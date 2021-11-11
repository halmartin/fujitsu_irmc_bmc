/*****************************************************************************
*
*                               NOTICE
*
*                COPYRIGHT 2012 - 2018 FUJITSU LIMITED
*                         ALL RIGHTS RESERVED
*
* Any reproduction of this program without the express written consent
* of Fujitsu Limited is a violation of the copyright laws and may
* subject you to civil liability and criminal prosecution.
*
* This computer program is CONFIDENTIAL and contains TRADE SECRETS of
* Fujitsu Technology Solutions and/or Fujitsu Limited which must not be disclosed.
* The receipt or possession of this program does not convey any rights to reproduce
* or disclose its contents, or to manufacture, use, or sell anything that it may
* describe, in whole or in part.
*
*****************************************************************************/
/*
 * Copyright (c) 2003 The OpenEvidence Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, the following disclaimer,
 *    and the original OpenSSL and SSLeay Licences below. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, the following disclaimer 
 *    and the original OpenSSL and SSLeay Licences below in
 *    the documentation and/or other materials provided with the
 *    distribution. 
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgments:
 *    "This product includes software developed by the Openevidence Project
 *    for use in the OpenEvidence Toolkit. (http://www.openevidence.org/)"
 *    This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *    This product includes cryptographic software written by Eric Young
 *    (eay@cryptsoft.com).  This product includes software written by Tim
 *    Hudson (tjh@cryptsoft.com)."
 *
 * 4. The names "OpenEvidence Toolkit" and "OpenEvidence Project" must not be
 *    used to endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openevidence-core@openevidence.org.
 *
 * 5. Products derived from this software may not be called "OpenEvidence"
 *    nor may "OpenEvidence" appear in their names without prior written
 *    permission of the OpenEvidence Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgments:
 *    "This product includes software developed by the OpenEvidence Project
 *    for use in the OpenEvidence Toolkit (http://www.openevidence.org/)
 *    This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *    This product includes cryptographic software written by Eric Young
 *    (eay@cryptsoft.com).  This product includes software written by Tim
 *    Hudson (tjh@cryptsoft.com)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenEvidence PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenEvidence PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes software developed by the OpenSSL Project
 * for use in the OpenSSL Toolkit (http://www.openssl.org/)
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */

/**
* @file:  fts_base32
* @brief: base32 encode/decode
* 
*/

#include <string.h>
#include <ctype.h>

/*
 * Note: Direct port of base32 firmware from iRMCS3 code base
 *
 * The following Base32 encoding/decoding routines are based on the Open Evidence source
 *
 */

static const char base32EncodeTable[32] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

static const unsigned char base32NumDecTable[10] = {
	0xFF, 0xFF, 26, 27, 28, 29, 30, 31, 0xFF, 0xFF
};

static int makeMask(int bit_count)
{
	int i;
	int ret = 0;
	
	for (i = 0; i < bit_count; i++) {
		ret <<= 1;
		ret |= 1;
	}

	return ret;
}

static void addBits(unsigned char *buf, int *bits_decoded, int bits)
{
	int bits_to_first_byte;
	int shift_count;
	int buf_idx;
	int selected_bits;

	if (bits < 0)
		return;
	
	bits_to_first_byte = 8 - *bits_decoded % 8;
	if (bits_to_first_byte > 5)
		bits_to_first_byte = 5;

	shift_count = 8 - bits_to_first_byte - *bits_decoded % 8;
	buf_idx = *bits_decoded / 8;
	selected_bits = (bits & (makeMask(bits_to_first_byte) << 
					(5 - bits_to_first_byte))) >> (5 - bits_to_first_byte);
	
	buf[buf_idx] |= selected_bits << shift_count;
	*bits_decoded += bits_to_first_byte;

	if (bits_to_first_byte < 5) {
		int bits_to_second_byte = 5 - bits_to_first_byte;
		
		shift_count = 8 - bits_to_second_byte;
		buf_idx++;
		selected_bits = bits & ((makeMask(bits_to_second_byte) << 
						(5 - bits_to_second_byte)) >> (5 - bits_to_second_byte));

		buf[buf_idx] |= selected_bits << shift_count;
		*bits_decoded += bits_to_second_byte;
	}
}

/* Returns -1 when EOF is encountered. */
static int readNextBits(const unsigned char *data, size_t data_len, int bits_read)
{
	int ret = 0;
	int first_byte_bits;
	size_t byte_to_read;
	int shift_count;
	
	byte_to_read = bits_read / 8;

	if (byte_to_read >= data_len)
		return -1;
	
	first_byte_bits = 8 - (bits_read - byte_to_read * 8);
	if (first_byte_bits > 5)
		first_byte_bits = 5;
	shift_count = 8 - bits_read % 8 - first_byte_bits;
	ret = (data[byte_to_read] & (makeMask(first_byte_bits) << shift_count)) >> 
			shift_count;

	byte_to_read++;
	if (first_byte_bits < 5) {
		int second_byte_bits = 5 - first_byte_bits;
		ret <<= second_byte_bits;
	
		if (byte_to_read < data_len) {
			shift_count = 8 - second_byte_bits;
			ret |= (data[byte_to_read] & (makeMask(second_byte_bits) << 
						shift_count)) >> shift_count;
		} 
	}

	return ret;
}

#define DASH_STEP   4 // Group every n Characters with a Dash...

char *base32Encode(const unsigned char *data, size_t data_len, char* data_out, int out_size)
{
	char *ret = data_out;
	int next_bits;
	size_t bits_read;
	int ret_len = 0;
	int dash_count = ((data_len *8) / (5 * DASH_STEP)); 

	if (!data || !data_len || !data_out || !out_size)
		return NULL;

	if (out_size < ((data_len / 5) * (8 + 1) + 8 + 1 + dash_count))
		return NULL;
	
	for (bits_read = 0; 
			(next_bits = readNextBits(data, data_len, bits_read)) != -1; 
			bits_read += 5) {
		ret[ret_len++] = base32EncodeTable[next_bits];

		// Add a dash '-' every n Chars to the output String ...
		if (ret_len % (DASH_STEP +1) == DASH_STEP && bits_read + 5 < data_len * 8)
			ret[ret_len++] = '-';
	}

#if 0 // not needed
	/* Pad output. (up to 5*5 Bit)*/
	while (bits_read % (DASH_STEP *5) != 0) {
		ret[ret_len++] = '=';
		bits_read += 5;
	}
#endif

	ret[ret_len++] = '\0';
	
	return ret;
	
}

unsigned char *base32Decode(const char *base32, size_t base32_len, unsigned char *data_out, size_t *ret_len)
{
	int bits_decoded = 0;
	unsigned char *ret = data_out;
	size_t i;

	if (!base32 || !data_out || !ret_len)
		return NULL;
		

	if (base32_len == 0)
		base32_len = strlen(base32);

	if (*ret_len < (base32_len * 5 / 8 + 2))
		return NULL; // Output Buffer to small
		

	// Clear out result
	memset(ret, 0, base32_len * 5 / 8 + 2);
	
	for (i = 0; i < base32_len; i++) {
		if (base32[i] == '=')
			break;
		if (isalpha(base32[i]))
			addBits(ret, &bits_decoded, toupper(base32[i]) - 'A');
		if (isdigit(base32[i]))
			addBits(ret, &bits_decoded, base32NumDecTable[base32[i] - '0']);
	}
	
	/* This operation also truncates extra bits from the end (when input
	 * bit count was not divisible by 5). */
	*ret_len = bits_decoded / 8;
	return ret;

}

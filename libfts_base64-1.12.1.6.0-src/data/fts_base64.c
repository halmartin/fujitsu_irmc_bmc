/*****************************************************************************
*
*                               NOTICE
*
*                COPYRIGHT 2005 - 2018 FUJITSU LIMITED
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>

#include "fts_base64.h"

// ----------------------------------------------------------------------------
#if 0 // ThreadX original version, which unfortunately has some problems encoding our test SSH key (last character is random)
// ============================================================================
STATIC const char b64enc[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// ============================================================================
size_t base64Encode( const unsigned char *input, char *output, const size_t len )
{
  int i;
  size_t inBytes = 0;
  size_t outBytes = 0;
  int done = 0;
  START_FUNC;

  while( !done ) {
    UCHAR igroup[3], ogroup[4];
    UCHAR c;
    int  n;

    igroup[0] = igroup[1] = igroup[2] = 0;

    /* Attempt to get 3 bytes at a time for encoding */
    for( n = 0; n < 3; n++ ) {
      c = input[inBytes];
      igroup[n] = c;

      if( inBytes++ == len ) {
        done = 1;
        break;
      }
    }

    /* Process input group of bytes into output group of bytes */
    if( n > 0 ) {
      ogroup[0] = b64enc[  igroup[0] >> 2];
      ogroup[1] = b64enc[( ( igroup[0] & 0x03 ) << 4 ) | ( igroup[1] >> 4 )];
      ogroup[2] = b64enc[( ( igroup[1] & 0x0F ) << 2 ) | ( igroup[2] >> 6 )];
      ogroup[3] = b64enc[  igroup[2] & 0x3F];

      /* Replace characters in output stream with "=" pad
         characters if fewer than three characters were
         read from the end of the input stream. */

      if( n < 3 ) {
        ogroup[3] = '=';

        if( n < 2 ) ogroup[2] = '=';
      }

      for( i = 0; i < 4; i++ )
        output[outBytes++] = ogroup[i];
    }
  }

  RETURN( outBytes );
}
#else 

// Version yoinked from OpenSSH source ...

/*	$OpenBSD: base64.c,v 1.5 2006/10/21 09:55:03 otto Exp $	*/

/*
 * Copyright (c) 1996 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * Portions Copyright (c) 1995 by International Business Machines, Inc.
 *
 * International Business Machines, Inc. (hereinafter called IBM) grants
 * permission under its copyrights to use, copy, modify, and distribute this
 * Software with or without fee, provided that the above copyright notice and
 * all paragraphs of this notice appear in all copies, and that the name of IBM
 * not be used in connection with the marketing of any product incorporating
 * the Software or modifications thereof, without specific, written prior
 * permission.
 *
 * To the extent it has a right to do so, IBM grants an immunity from suit
 * under its patents, if any, for the use, sale or manufacture of products to
 * the extent that such products are used for performing Domain Name System
 * dynamic updates in TCP/IP networks by means of the Software.  No immunity is
 * granted for any product per se or for any other function of any product.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", AND IBM DISCLAIMS ALL WARRANTIES,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL IBM BE LIABLE FOR ANY SPECIAL,
 * DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE, EVEN
 * IF IBM IS APPRISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */

/* OPENBSD ORIGINAL: lib/libc/net/base64.c */

static const char Base64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64 = '=';

/* (From RFC1521 and draft-ietf-dnssec-secext-03.txt)
   The following encoding technique is taken from RFC 1521 by Borenstein
   and Freed.  It is reproduced here in a slightly edited form for
   convenience.

   A 65-character subset of US-ASCII is used, enabling 6 bits to be
   represented per printable character. (The extra 65th character, "=",
   is used to signify a special processing function.)

   The encoding process represents 24-bit groups of input bits as output
   strings of 4 encoded characters. Proceeding from left to right, a
   24-bit input group is formed by concatenating 3 8-bit input groups.
   These 24 bits are then treated as 4 concatenated 6-bit groups, each
   of which is translated into a single digit in the base64 alphabet.

   Each 6-bit group is used as an index into an array of 64 printable
   characters. The character referenced by the index is placed in the
   output string.

                         Table 1: The Base64 Alphabet

      Value Encoding  Value Encoding  Value Encoding  Value Encoding
          0 A            17 R            34 i            51 z
          1 B            18 S            35 j            52 0
          2 C            19 T            36 k            53 1
          3 D            20 U            37 l            54 2
          4 E            21 V            38 m            55 3
          5 F            22 W            39 n            56 4
          6 G            23 X            40 o            57 5
          7 H            24 Y            41 p            58 6
          8 I            25 Z            42 q            59 7
          9 J            26 a            43 r            60 8
         10 K            27 b            44 s            61 9
         11 L            28 c            45 t            62 +
         12 M            29 d            46 u            63 /
         13 N            30 e            47 v
         14 O            31 f            48 w         (pad) =
         15 P            32 g            49 x
         16 Q            33 h            50 y

   Special processing is performed if fewer than 24 bits are available
   at the end of the data being encoded.  A full encoding quantum is
   always completed at the end of a quantity.  When fewer than 24 input
   bits are available in an input group, zero bits are added (on the
   right) to form an integral number of 6-bit groups.  Padding at the
   end of the data is performed using the '=' character.

   Since all base64 input is an integral number of octets, only the
         -------------------------------------------------                       
   following cases can arise:
   
       (1) the final quantum of encoding input is an integral
           multiple of 24 bits; here, the final unit of encoded
           output will be an integral multiple of 4 characters
           with no "=" padding,
       (2) the final quantum of encoding input is exactly 8 bits;
           here, the final unit of encoded output will be two
           characters followed by two "=" padding characters, or
       (3) the final quantum of encoding input is exactly 16 bits;
           here, the final unit of encoded output will be three
           characters followed by one "=" padding character.
   */

static int
b64_ntop(u_char const *src, size_t srclength, char *target, size_t targsize)
{
	size_t datalength = 0;
	u_char input[3];
	u_char output[4];
	u_int i;

	while (2 < srclength) {
		input[0] = *src++;
		input[1] = *src++;
		input[2] = *src++;
		srclength -= 3;

		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
		output[3] = input[2] & 0x3f;

		if (datalength + 4 > targsize)
			return (-1);
		target[datalength++] = Base64[output[0]];
		target[datalength++] = Base64[output[1]];
		target[datalength++] = Base64[output[2]];
		target[datalength++] = Base64[output[3]];
	}
    
	/* Now we worry about padding. */
	if (0 != srclength) {
		/* Get what's left. */
		input[0] = input[1] = input[2] = '\0';
		for (i = 0; i < srclength; i++)
			input[i] = *src++;
	
		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);

		if (datalength + 4 > targsize)
			return (-1);
		target[datalength++] = Base64[output[0]];
		target[datalength++] = Base64[output[1]];
		if (srclength == 1)
			target[datalength++] = Pad64;
		else
			target[datalength++] = Base64[output[2]];
		target[datalength++] = Pad64;
	}
	if (datalength >= targsize)
		return (-1);
	target[datalength] = '\0';	/* Returned value doesn't count \0. */
	return (datalength);
}

size_t base64Encode( const unsigned char *input, const size_t input_len, char *output, const size_t max_len )
{
  return b64_ntop(input, input_len, output, max_len);
}


#endif

#if 1 // ThreadX original version

// ----------------------------------------------------------------------------
static const unsigned char b64dec[128] = {
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x3E, 0x80, 0x80, 0x80, 0x3F,
  0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
  0x3C, 0x3D, 0x80, 0x80, 0x80, 0x00, 0x80, 0x80,
  0x80, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
  0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
  0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
  0x17, 0x18, 0x19, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x80, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
  0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
  0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
  0x31, 0x32, 0x33, 0x80, 0x80, 0x80, 0x80, 0x80
};

// ----------------------------------------------------------------------------
size_t base64Decode( const char *input, unsigned char *output, const size_t len )
{
  int i;
  size_t inBytes = 0;
  size_t outBytes = 0;
  int done = 0;

  while( !done ) {
    unsigned char a[4] = {0}, b[4] = {0}, o[3] = {0};
    int n;

    for( i = 0; i < 4; i++ ) {
      unsigned char c;
      c = input[inBytes++];

      if( b64dec[( int )c] & 0x80 ) {
        /* This is an error condition in the
         * base 64 encoded stream.
         * Ignore the invalid character and
         * try to continue processing the
         * stream.
         */
        /* HLiebig / SIRIUS Team:
         * break if the incorrect character is the last one
         * or else this will overwrite the caller's stack with garbled data
         */
        if( inBytes == len ) {
          /* No more characters exit for loop
           * regardless of value of i.
           * Might be an error condition if input
           * is not integral multiple of 4 bytes though.
           */
          done = 1;
          break;
        }

        i--;
        continue;
      }

      a[i] = c;
      b[i] = b64dec[( int )c];

      if( inBytes == len ) {
        /* No more characters exit for loop
         * regardless of value of i.
         * Might be an error condition if input
         * is not integral multiple of 4 bytes though.
         */
        done = 1;
        break;
      }
    }

    /* To attempt to gracefully handle decoding base 64 encoded streams
     * that are not an integral multiple of 4 in len for various reasons
     * above validate the value of i before performing each of these decode
     * bit munging assignments.
     */
    if( i > 0 )  o[0] = ( b[0] << 2 ) | ( b[1] >> 4 );

    if( i > 1 )  o[1] = ( b[1] << 4 ) | ( b[2] >> 2 );

    if( i > 2 )  o[2] = ( b[2] << 6 ) | b[3];

    i = a[2] == '=' ? 1 : ( a[3] == '=' ? 2 : 3 );

    /* Write the results to the output stream. */
    for( n = 0; n < i; n++ ) output[outBytes++] = o[n];

    if( i < 3 ) done = 1;
  }

  return( outBytes );
}

#else 

// OpenSSH version which would need additional work to import a SSHv2 key 

/* skips all whitespace anywhere.
   converts characters, four at a time, starting at (or after)
   src from base - 64 numbers into three 8 bit bytes in the target area.
   it returns the number of data bytes stored at the target, or -1 on error.
 */
static int
b64_pton(char const *src, u_char *target, size_t targsize)
{
	u_int tarindex, state;
	int ch;
	char *pos;

	state = 0;
	tarindex = 0;

	while ((ch = *src++) != '\0') {
		if (isspace(ch))	/* Skip whitespace anywhere. */
			continue;

		if (ch == Pad64)
			break;

		pos = strchr(Base64, ch);
		if (pos == 0) 		/* A non-base64 character. */
			return (-1);

		switch (state) {
		case 0:
			if (target) {
				if (tarindex >= targsize)
					return (-1);
				target[tarindex] = (pos - Base64) << 2;
			}
			state = 1;
			break;
		case 1:
			if (target) {
				if (tarindex + 1 >= targsize)
					return (-1);
				target[tarindex]   |=  (pos - Base64) >> 4;
				target[tarindex+1]  = ((pos - Base64) & 0x0f)
							<< 4 ;
			}
			tarindex++;
			state = 2;
			break;
		case 2:
			if (target) {
				if (tarindex + 1 >= targsize)
					return (-1);
				target[tarindex]   |=  (pos - Base64) >> 2;
				target[tarindex+1]  = ((pos - Base64) & 0x03)
							<< 6;
			}
			tarindex++;
			state = 3;
			break;
		case 3:
			if (target) {
				if (tarindex >= targsize)
					return (-1);
				target[tarindex] |= (pos - Base64);
			}
			tarindex++;
			state = 0;
			break;
		}
	}

	/*
	 * We are done decoding Base-64 chars.  Let's see if we ended
	 * on a byte boundary, and/or with erroneous trailing characters.
	 */

	if (ch == Pad64) {		/* We got a pad char. */
		ch = *src++;		/* Skip it, get next. */
		switch (state) {
		case 0:		/* Invalid = in first position */
		case 1:		/* Invalid = in second position */
			return (-1);

		case 2:		/* Valid, means one byte of info */
			/* Skip any number of spaces. */
			for (; ch != '\0'; ch = *src++)
				if (!isspace(ch))
					break;
			/* Make sure there is another trailing = sign. */
			if (ch != Pad64)
				return (-1);
			ch = *src++;		/* Skip the = */
			/* Fall through to "single trailing =" case. */
			/* FALLTHROUGH */

		case 3:		/* Valid, means two bytes of info */
			/*
			 * We know this char is an =.  Is there anything but
			 * whitespace after it?
			 */
			for (; ch != '\0'; ch = *src++)
				if (!isspace(ch))
					return (-1);

			/*
			 * Now make sure for cases 2 and 3 that the "extra"
			 * bits that slopped past the last full byte were
			 * zeros.  If we don't check them, they become a
			 * subliminal channel.
			 */
			if (target && target[tarindex] != 0)
				return (-1);
		}
	} else {
		/*
		 * We ended by seeing the end of the string.  Make sure we
		 * have no partial bytes lying around.
		 */
		if (state != 0)
			return (-1);
	}

	return (tarindex);
}
size_t base64Decode( const char *input, unsigned char *output, const size_t len )
{
  size_t targsize = B64DECODE_LEN (len);
  return b64_pton(input, output, targsize);
}

#endif


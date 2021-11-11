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
/******************************************************************************
* Filename: fts_base32.h
*
* Description:  base32 encode/decode
****************************************************************************/
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

#ifndef FTS_BASE32_H_
#define FTS_BASE32_H_

char *base32Encode(const unsigned char *data, size_t data_len, char* data_out, int out_size);
unsigned char *base32Decode(const char *base32, size_t base32_len, unsigned char *data_out, size_t *ret_len);

#endif /* FTS_BASE32_H_ */

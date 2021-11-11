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

#ifndef __FTS_BASE64_H
#define __FTS_BASE64_H

// ----------------------------------------------------------------------------
#define B64ENCODE_LEN(X) ( 4*((X+2)/3) )
#define B64DECODE_LEN(X) ( 3*((X+3)/4) )

#ifdef __cplusplus
extern "C" {
#endif

size_t  base64Decode( const char *input, unsigned char *output, const size_t len );
size_t  base64Encode( const unsigned char *input, const size_t input_len, char *output, const size_t max_len );

#ifdef __cplusplus
}
#endif

// ============================================================================
#endif

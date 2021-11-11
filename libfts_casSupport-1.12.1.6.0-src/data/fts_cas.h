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

/*
 *  Copyright (c) 2000-2003 Yale University. All rights reserved.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS," AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE EXPRESSLY
 *  DISCLAIMED. IN NO EVENT SHALL YALE UNIVERSITY OR ITS EMPLOYEES BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED, THE COSTS OF
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED IN ADVANCE OF THE POSSIBILITY OF SUCH
 *  DAMAGE.
 *
 *  Redistribution and use of this software in source or binary forms,
 *  with or without modification, are permitted, provided that the
 *  following conditions are met:
 *
 *  1. Any redistribution must include the above copyright notice and
 *  disclaimer and this list of conditions in any related documentation
 *  and, if feasible, in the redistributed software.
 *
 *  2. Any redistribution must include the acknowledgment, "This product
 *  includes software developed by Yale University," in any related
 *  documentation and, if feasible, in the redistributed software.
 *
 *  3. The names "Yale" and "Yale University" must not be used to endorse
 *  or promote products derived from this software.
 */
/*
 * 
 * Copyright (c) 2007, JA-SIG, Inc.
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 * 
 *     Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *     Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation 
 *     and/or other materials provided with the distribution.
 *     Neither the name of the JA-SIG, Inc. nor the names of its contributors may be used to endorse 
 *     or promote products derived from this software without specific prior written permission.
 *     
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *     A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *     CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *     EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *     PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *     PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *     LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *     NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *     SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
#ifndef __FTS_CAS_H_
#define __FTS_CAS_H_

/*
 * Macro Definitions
 */

/* CAS information */

/* Default CAS URI's and paths */
#define CAS_DEFAULT_LOGIN       "/cas/login"        /* /login URI */
#define CAS_DEFAULT_LOGOUT      "/cas/logout"       /* /logout URI */
#define CAS_DEFAULT_VALIDATE    "/cas/validate"     /* /validate [CAS 1.0] URI */

/* CAS default Server Definitions */
#define CAS_DEFAULT_HOST        "192.168.100.100"   /* Host name or Server IP address */
#define CAS_DEFAULT_PORT        3170                /* CAS Server port */

/* Ticket validation request definitions */
#define CAS_SCHEME              "http://"           /* Non-secure scheme */
#define CAS_SCHEME_S            "https://"          /* Secure scheme */
#define CAS_METHOD              "GET"
#define CAS_PROT                "HTTP/1.0"

// ============================================================================
//
// Central Authentication Service (CAS) Support
//
#define CA_CERT_FILE 4

// ============================================================================


/* CAS authentication status type definition */
typedef enum {
  CAS_ERROR = -1,             /* Error detected */
  CAS_OK,                     /* Authentication successful */
  CAS_REDIRECT                /* Service request re-directed to CAS for authentication */
} CAS_STATUS;

/* CAS configuration parameters type definition */
typedef enum {
  CAS_CFG_HOST,               /* CAS server name or IP address string */
  CAS_CFG_PORT,               /* CAS TCP port */
  CAS_CFG_SECURE,             /* CAS uses secure interface (http:// vs https://) */
  CAS_CFG_LOGIN,              /* CAS login URI and path */
  CAS_CFG_LOGOUT,             /* CAS logout URI and path */
  CAS_CFG_VALIDATE,           /* CAS validate URI and path */
  CAS_CFG_VALIDATE_CERT_EN,   /* CAS certificate validation enable */
  CAS_CFG_CA_CERT             /* CAS CA certificate file identifier */
} CAS_CFG_ID;

/* Ticket validation status definitions */
typedef enum {
  TVAL_OK,                /* Ticket is valid */
  TVAL_ERROR,             /* Ticket validation failed */
  TVAL_BAD_HOST,          /* HLiebig: Resolving Hostname failed */
  TVAL_NO_CONNECT,        /* HLiebig: Connect failed */
  TVAL_BAD_CERT           /* Invalid CAS server certificate */
} TVAL_STATE;

#define CAS_URLBUF_SIZE      4096    /* Maximum URL buffer size */


// ============================================================================
#include <libipmi_struct.h>

#ifdef __cplusplus
extern "C" {
#endif

TVAL_STATE casValidate( IPMI20_SESSION_T *ipmiSess, char *ticket, char *service, char *user, int userSize );
BOOL       casIsLoginActive( IPMI20_SESSION_T *ipmiSess );
BOOL       casConfigurationGet( CAS_CFG_ID cfgId, void *out );
char      *casGenerateUrl( char *urlBuf, int bufSize, char *casUri );


#ifdef __cplusplus
}
#endif

// ============================================================================
#endif

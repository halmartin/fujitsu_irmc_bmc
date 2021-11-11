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

#include "fts_appHelper.h"
#include "fts_cas.h"
#include "fts_lanHelper.h"
#include "fts_SimpleSignOn.h" // DEFAULT_HTTP_PORT DEFAULT_HTTPS_PORT


#include <openssl/ssl.h>
#include <openssl/err.h>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

// ============================================================================
/*
 * Static Datatypes
 */

static BOOL gCasEnable = FALSE;
static BOOL gCasUseSsl = TRUE;
static BOOL gCasVerifyServerCert = FALSE;
static BOOL gCasAlwaysDisplayLogin = TRUE;
static UINT16 gCasPort = CAS_DEFAULT_PORT;

static UINT8 gCasServer[ConfBmcCasServer_MaxSize +1];
static UINT8 gCasLoginUri[ConfBmcCasLoginUri_MaxSize +1];
static UINT8 gCasLogoutUri[ConfBmcCasLogoutUri_MaxSize +1];
static UINT8 gCasValidateUri[ConfBmcCasValidateUri_MaxSize +1];

// ============================================================================
/*
 * Static Function Declaration
 */

static int        casVerifyCallback( int ok, X509_STORE_CTX *ctx );

/******************************************************************************
 ** FUNCTION    casConfigurationGet()
 **
 ** INPUT       cfgId           CAS configuration variable identifier
 **             out             Pointer to configuration variable being read
 ** OUTPUT      BOOL            Configuration operation status
 **                             TRUE    CAS variable successfully returned
 **                             FALSE   CAS variable not returned
 **
 ** DESCRIPTION     Function to read a CAS configuration variable. Variable to
 **     be read is specified by cfgId. Function caller must ensure CAS
 **     configuration variables are read to a local variable with the correct
 **     type as specified below
 **
 **     Configuration ID            Type        Description
 **     CAS_CFG_HOST,               (char **)   server name or IP address string
 **     CAS_CFG_PORT,               (int *)     TCP port
 **     CAS_CFG_SECURE,             (BOOL *)    Secure interface (http:// vs https://)
 **     CAS_CFG_LOGIN,              (char **)   /login URI and path
 **     CAS_CFG_LOGOUT,             (char **)   /logout URI and path
 **     CAS_CFG_VALIDATE,           (char **)   /validate URI and path
 **     CAS_CFG_VALIDATE_CERT_EN    (BOOL *)    Certificate validation enable
 **     CAS_CFG_CA_CERT             (int *)     CA certificate file ID
 **
 *****************************************************************************/
BOOL casConfigurationGet( CAS_CFG_ID cfgId, void *out )
{
  BOOL ret = TRUE;

  if( out == NULL ) return FALSE;

  switch( cfgId ) {
  case CAS_CFG_HOST:
    *( char ** )out = ( char * )gCasServer;
    break;
  case CAS_CFG_PORT:
    *( int * )out = ( int )gCasPort;
    break;
  case CAS_CFG_SECURE:
    *( BOOL * )out = gCasUseSsl;
    break;
  case CAS_CFG_LOGIN:
    *( char ** )out = ( char * )gCasLoginUri;
    break;
  case CAS_CFG_LOGOUT:
    *( char ** )out = ( char * )gCasLogoutUri;
    break;
  case CAS_CFG_VALIDATE:
    *( char ** )out = ( char * )gCasValidateUri;
    break;
  case CAS_CFG_VALIDATE_CERT_EN:
    *( BOOL * )out = gCasVerifyServerCert;
    break;
  case CAS_CFG_CA_CERT:
    *( int * )out = ( int )CA_CERT_FILE;
    break;
  default:
    ret = FALSE;
    break;
  }

  return ret;
}

/*
 * Static Function Definition
 */

/******************************************************************************
 ** FUNCTION    casValidate()
 **
 ** INPUT       ticket          ticket provided by CAS
 **             service         Requested service
 **             user            Pointer to return user ID string
 **             userSize        Size (in bytes) of user ID buffer
 ** OUTPUT      TVAL_STATE      Ticket validation Status
 **                             TVAL_OK         Ticket is valid
 **                             TVAL_ERROR      Ticket validation failed
 **                             TVAL_BAD_CERT   Invalid CAS certificate
 **
 ** DESCRIPTION     Returns status of ticket by filling 'buf' with a user ID if
 *      the ticket is valid and buf is large enough and returning 1.  If not, 0 is
 *  returned.
 **
 *****************************************************************************/
TVAL_STATE casValidate( IPMI20_SESSION_T *ipmiSess, char *ticket, char *service, char *user, int userSize )
{
  TVAL_STATE rc = TVAL_ERROR;
  int b, total;
  SSL_CTX *ctx = NULL;
  SSL *ssl = NULL;
  char *validateRsp = NULL;
  char *validateReq = NULL, *str = NULL;
  char *casValidateUri = NULL;
  char *casHost = NULL;
  int casPort;
  int sockfd = INVALID_SOCKET;
  struct addrinfo *res;
  struct addrinfo *res0 = NULL;
  BOOL validateCasCert = TRUE;

  const IPMI_CHANNEL_NUM chanNum = etcGetDefaultLanChannelNumber( ipmiSess );  // BUGBUG: Currently Fixed

  BOOL bIpv4Enabled = lanIpv4Enabled(ipmiSess, chanNum);
  BOOL bIpv6Enabled = lanIpv6Enabled(ipmiSess, chanNum);

#define END(msg, err)       { if (msg != NULL){APP_INFO("%s", msg);} rc = (err); goto end; }
#define FAIL(msg)           END(msg, TVAL_ERROR)
#define FAIL_BAD_CERT(msg)  END(msg, TVAL_BAD_CERT)
#define SUCCEED(msg)        END(msg, TVAL_OK)

  if( !casConfigurationGet( CAS_CFG_VALIDATE, &casValidateUri ) ||
      !casConfigurationGet( CAS_CFG_HOST, &casHost )            ||
      !casConfigurationGet( CAS_CFG_PORT, &casPort ) )
    FAIL( "get configuration error" );

  APP_INFO( "casValidateUri=%s  casHost=%s  casPort=%d", casValidateUri, casHost, casPort );
  validateRsp = malloc( CAS_URLBUF_SIZE );

  if( validateRsp == NULL )
    FAIL( "Memory allocation error" );

  if( getaddrinfo( casHost, NULL, NULL, &res0 ) != 0 )
    END( "Bad host name", TVAL_BAD_HOST );

  /* loop over the list of results. break on successfull connect */
  for( res = res0; res; res = res->ai_next ) {
    // Prevent connections which we can resolve from DNS, but not connect to
    if( !bIpv6Enabled && res->ai_family == AF_INET6 )
      continue;

    if( !bIpv4Enabled && res->ai_family == AF_INET )
      continue;

    if( res->ai_family == AF_INET ) {
      struct sockaddr_in *addr = ( struct sockaddr_in * )( res->ai_addr );
      addr->sin_port = htons( casPort );

    } else if( res->ai_family == AF_INET6 ) {
      struct sockaddr_in6 *addr = ( struct sockaddr_in6 * )( res->ai_addr );
      addr->sin6_port = htons( casPort );
    }

    /* prevent sockets to 0.0.0.0 or 127.0.0.1 or ::1 etc. */
    if( !etcIsValidRemoteSocketAddress( res ) )
      continue;

    sockfd = socket( res->ai_family, SOCK_STREAM, 0 );

    if( sockfd == INVALID_SOCKET ) {
      APP_INFO( "CAS: socket() failed, errno=%d: %s", errno, strerror( errno ) );
      continue;
    }

    if( connect( sockfd, res->ai_addr, res->ai_addrlen ) == INVALID_SOCKET ) {
      APP_INFO( "CAS: connect() failed, errno=%d: %s", errno, strerror( errno ) );
      close( sockfd );
      sockfd = INVALID_SOCKET;
      rc = TVAL_NO_CONNECT;
      continue;
    }

    // We are successfull connected ...
    APP_INFO( "CAS: connected" );
    break;
  }

  freeaddrinfo( res0 );

  if( sockfd == INVALID_SOCKET )
    goto end;

  /* SSL connection */
  SSL_library_init();
  SSL_load_error_strings();

  if( ( ctx = SSL_CTX_new( (SSL_METHOD *) SSLv23_client_method() ) ) == NULL )
    FAIL( "CAS: SSL_CTX_new() error" );

  casConfigurationGet( CAS_CFG_VALIDATE_CERT_EN, &validateCasCert );

  if( validateCasCert ) {
    APP_INFO( "CAS: validate Cert against CA File=%s CA Path=%s", WEB_CA_FILE, WEB_CERT_DIR );

    if( ( !SSL_CTX_load_verify_locations( ctx, WEB_CA_FILE, WEB_CERT_DIR ) ) ||
        ( !SSL_CTX_set_default_verify_paths( ctx ) ) ) {
      APP_ERROR( "CAS: CA File=%s Path=%s: error=%s", WEB_CA_FILE, WEB_CERT_DIR, ERR_error_string( ERR_get_error(),NULL ) );
      FAIL( "CAS: certificate validation setup error" );

    } else {
      /* Set Verification flags to validate CAS server Certificate */
      SSL_CTX_set_verify( ctx, SSL_VERIFY_PEER, casVerifyCallback );
    }

  }

  if( !( ssl = SSL_new( ctx ) ) )
    FAIL( "SSL_new() error" );

  SSL_set_fd( ssl, sockfd );

  if( SSL_connect( ssl ) <= 0 ) {
    /* Check for CAS Certificate validation error, otherwise generic failure */
    if( validateCasCert && SSL_get_verify_result( ssl ) )
      FAIL_BAD_CERT( NULL );

    FAIL( "SSL_connect() error" );
  }

  validateReq = malloc( strlen( CAS_METHOD ) + strlen( " " )
                      + strlen( ( const char * )casValidateUri )
                      + strlen( "?ticket=" ) + strlen( ticket ) +
                      + strlen( "&service=" ) + strlen( service )
                      + strlen( " " )
                      + strlen( CAS_PROT ) + strlen( "\n\n" ) + 1 );

  if( validateReq == NULL )
    FAIL( "validateReq allocation error" );

  sprintf( validateReq, "%s %s?ticket=%s&service=%s %s\n\n",
           CAS_METHOD, casValidateUri, ticket, service, CAS_PROT );
  APP_INFO( "validateReq=%s", validateReq );

  if( !SSL_write( ssl, validateReq, strlen( validateReq ) ) )
    FAIL( "SSL_write() error" );

  total = 0;

  do {
    b = SSL_read( ssl, validateRsp + total, ( CAS_URLBUF_SIZE - 1 ) - total );
    total += b;
  } while( b > 0 );

  validateRsp[total] = '\0';

  if( b != 0 || total >= CAS_URLBUF_SIZE - 1 )
    FAIL( "SSL_read() or overflow error" ); /* unexpected read error or response too large */

  /*
  * The following segment of code is spelled out in excruciating detail
  * since it's worth the extra care.
  */
  str = strstr( validateRsp, "\r\n\r\n" ); /* find the end of the header */

  if( !str )
    FAIL( "response header error" );      /* no header */

  str += 4;                               /* safe because we just found "\r\n\r\n" */

  if( strncmp( str, "yes", 3 ) )
    FAIL( "CAS validation failure" );     /* no affirmative response */

  str = strchr( str, '\n' );              /* find the end of the current line */

  if( !str ) {
    str = strchr( str, '\r' );            /* possibly ended in CR (ok by RFC-2616) */

    if( !str )
      FAIL( "response body error" );      /* unexpectedly short body */
  }

  str++;                                  /* safe because we just found "\n" or "\r" */

  /* now, we've got the NetID and a bunch of crap after it.  Find the next
   * '[\r]\n' and end the string there
   */
  {
    char *str2 = strchr( str, '\r' );

    if( !str2 ) {
      str2 = strchr( str, '\n' );

      if( !str2 )
        FAIL( "response body error" ); /* unexpectedly short body */
    }

    *str2 = '\0';                       /* remove [\r]\n */
  }

  /*
  * without enough space, fail entirely, since a partial NetID could
  * be dangerous
  */
  if( userSize < strlen( str ) + 1 )
    FAIL( "user size error" );

  strcpy( user, str );
  APP_INFO( "user=%s",user );

  SUCCEED( NULL );

  /* cleanup and return */

end:

  if( ssl != NULL ) {
    if( SSL_shutdown( ssl ) == 0 ) {
      sleep( 3 );
      SSL_shutdown( ssl );
    }
  }

  if( sockfd != INVALID_SOCKET ) close( sockfd );

  if( ssl != NULL ) SSL_free( ssl );

  if( ctx != NULL ) SSL_CTX_free( ctx );

  if (validateReq) free ( validateReq );
  if (validateRsp) free ( validateRsp );

  return( rc );
}


/******************************************************************************
 ** FUNCTION    casGenerateUrl()
 **
 ** INPUT       urlBuf      Pointer to buffer for CAS URL
 **             bufSize     Size (in bytes) of CAS URL buffer
 **             casUri      CAS URI to be utilize when generating URL
 ** OUTPUT      char        Return pointer to CAS URL. NULL if URL is not
 **                         generated
 **
 ** DESCRIPTION     Function to generate a URL for redirecting to a CAS URI.
 **     The CAS URL if formed using configuration parameters describing the
 **     CAS servers HTTP scheme, name or IP address and port number. Appended
 **     to this is a specified CAS URI (including path)
 **
 *****************************************************************************/
char *casGenerateUrl( char *urlBuf, int bufSize, char *casUri )
{
  char *casUrl = NULL;
  char *casHost = NULL;
  int   casPort;

  if( urlBuf != NULL && casConfigurationGet( CAS_CFG_HOST, &casHost ) ) {
    struct in6_addr in6;

    BOOL casSecure = TRUE;
    BOOL bIsIpv6 = FALSE;

    memset( &in6, 0, sizeof(in6) );

    if( inet_pton( AF_INET6, casHost, &in6 ) ==  1 ) {
      bIsIpv6 = TRUE;
    }

    casUrl = urlBuf;
    memset( casUrl, 0, bufSize );

#ifdef ENABLE_NON_SECURE_CAS
    casConfigurationGet( CAS_CFG_SECURE, &casSecure );
#endif

    if( !casConfigurationGet( CAS_CFG_PORT, &casPort ) ) {
//    casPort = ( casSecure ) ? DEFAULT_HTTPS_PORT : DEFAULT_HTTP_PORT;
      casPort = CAS_DEFAULT_PORT;
    }

    if( casPort != ( ( casSecure ) ? DEFAULT_HTTPS_PORT : DEFAULT_HTTP_PORT ) ) {
      if( bIsIpv6 )
        snprintf( casUrl, bufSize, "%s[%s]:%d%s", ( casSecure ) ? CAS_SCHEME_S : CAS_SCHEME, casHost, casPort, casUri );

      else
        snprintf( casUrl, bufSize, "%s%s:%d%s", ( casSecure ) ? CAS_SCHEME_S : CAS_SCHEME, casHost, casPort, casUri );

    } else {
      if( bIsIpv6 )
        snprintf( casUrl, bufSize, "%s[%s]%s", ( casSecure ) ? CAS_SCHEME_S : CAS_SCHEME, casHost, casUri );

      else
        snprintf( casUrl, bufSize, "%s%s%s", ( casSecure ) ? CAS_SCHEME_S : CAS_SCHEME, casHost, casUri );
    }
  }

  APP_INFO( "casUrl=%s", casUrl );

  return( casUrl );
}


/******************************************************************************
 ** FUNCTION    casVerifyCallback()
 **
 ** INPUT       ok          Certificate validation status from OpenSSL
 **             ctx         X509 certificate store structure
 ** OUTPUT      int         Returned Certificate validation status
 **
 ** DESCRIPTION     Callback to evaluate the response of Server certificate
 **     validation.
 **
 *****************************************************************************/
static int casVerifyCallback( int ok, X509_STORE_CTX *ctx )
{
  switch( ctx->error ) {
  case X509_V_OK:
    APP_INFO( "X509_V_OK" );
    break;
  case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT)" );
    break;
  case X509_V_ERR_UNABLE_TO_GET_CRL:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_UNABLE_TO_GET_CRL)" );
    break;
  case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE)" );
    break;
  case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE)" );
    break;
  case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY)" );
    break;
  case X509_V_ERR_CERT_SIGNATURE_FAILURE:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_CERT_SIGNATURE_FAILURE)" );
    break;
  case X509_V_ERR_CRL_SIGNATURE_FAILURE:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_CRL_SIGNATURE_FAILURE)" );
    break;
  case X509_V_ERR_CERT_NOT_YET_VALID:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_CERT_NOT_YET_VALID)" );
    break;
  case X509_V_ERR_CERT_HAS_EXPIRED:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_CERT_HAS_EXPIRED)" );
    break;
  case X509_V_ERR_CRL_NOT_YET_VALID:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_CRL_NOT_YET_VALID)" );
    break;
  case X509_V_ERR_CRL_HAS_EXPIRED:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_CRL_HAS_EXPIRED)" );
    break;
  case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD)" );
    break;
  case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD)" );
    break;
  case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD)" );
    break;
  case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD)" );
    break;
  case X509_V_ERR_OUT_OF_MEM:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_OUT_OF_MEM)" );
    break;
  case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)" );
    break;
  case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN)" );
    break;
  case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY)" );
    break;
  case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE)" );
    break;
  case X509_V_ERR_CERT_CHAIN_TOO_LONG:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_CERT_CHAIN_TOO_LONG)" );
    break;
  case X509_V_ERR_CERT_REVOKED:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_CERT_REVOKED)" );
    break;
  case X509_V_ERR_INVALID_CA:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_INVALID_CA)" );
    break;
  case X509_V_ERR_PATH_LENGTH_EXCEEDED:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_PATH_LENGTH_EXCEEDED)" );
    break;
  case X509_V_ERR_INVALID_PURPOSE:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_INVALID_PURPOSE)" );
    break;
  case X509_V_ERR_CERT_UNTRUSTED:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_CERT_UNTRUSTED)" );
    break;
  case X509_V_ERR_CERT_REJECTED:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_CERT_REJECTED)" );
    break;
  case X509_V_ERR_SUBJECT_ISSUER_MISMATCH:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_SUBJECT_ISSUER_MISMATCH)" );
    break;
  case X509_V_ERR_AKID_SKID_MISMATCH:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_AKID_SKID_MISMATCH)" );
    break;
  case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH)" );
    break;
  case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
    APP_INFO( "CAS: Cert Error (X509_V_ERR_KEYUSAGE_NO_CERTSIGN)" );
    break;
  default:
    APP_INFO( "CAS: Cert Error (error code: %d)", ctx->error );
    break;
  }

  return ok;
}

// FSCCHG_HLiebig: 
// Wrapper function to check if CAS is active,
// by testing for enabled and valid IP Address
BOOL casIsLoginActive( IPMI20_SESSION_T *ipmiSess )
{
  // Update all CAS Runtime Variables
  gCasEnable = configSpaceReadByteWithOid( ipmiSess, ConfBmcCasEnable, DEFAULT_OID ) ? TRUE : FALSE;
#ifdef ENABLE_NON_SECURE_CAS
  gCasUseSsl = configSpaceReadByteWithOid( ipmiSess, ConfBmcCasUseHttps, DEFAULT_OID ) ? TRUE : FALSE;
#endif
  gCasVerifyServerCert = configSpaceReadByteWithOid( ipmiSess, ConfBmcCasVerifyServerCert, DEFAULT_OID ) ? TRUE : FALSE;
  gCasAlwaysDisplayLogin = configSpaceReadByteWithOid( ipmiSess, ConfBmcCasAlwaysDisplayLogin, DEFAULT_OID ) ? TRUE : FALSE;
  gCasPort = configSpaceReadWordWithOid( ipmiSess, ConfBmcCasPort, DEFAULT_OID );

  if( gCasPort == 0 || gCasPort == 0xFFFF ) {
    APP_ERROR( "Invalid CAS network port %d - defaulting to ", gCasPort, CAS_DEFAULT_PORT );
    gCasPort = CAS_DEFAULT_PORT;
  }

  ZERO_MEMORY( gCasServer );
  ZERO_MEMORY( gCasLoginUri );
  ZERO_MEMORY( gCasLogoutUri );
  ZERO_MEMORY( gCasValidateUri );

  configSpaceRead( ipmiSess, ConfBmcCasServer, DEFAULT_OID, gCasServer, sizeof( gCasServer ) );
  configSpaceRead( ipmiSess, ConfBmcCasLoginUri, DEFAULT_OID, gCasLoginUri, sizeof( gCasLoginUri ) );
  configSpaceRead( ipmiSess, ConfBmcCasLogoutUri, DEFAULT_OID, gCasLogoutUri, sizeof( gCasLogoutUri ) );
  configSpaceRead( ipmiSess, ConfBmcCasValidateUri, DEFAULT_OID, gCasValidateUri, sizeof( gCasValidateUri ) );

  APP_INFO( "gCasEnable=%s  gCasServer=%s gCasPort=%d", gCasEnable?"TRUE":"FALSE", gCasServer, gCasPort );

  if( ( gCasEnable == TRUE ) &&  strlen( ( const char * )gCasServer ) > 0 ) {
    struct addrinfo *res0 = NULL;
    struct addrinfo *res;
    int  error;

    error = getaddrinfo( ( const char * )gCasServer, NULL, NULL, &res0 );

    if( error != 0 ) {
      APP_INFO( "error in getaddrinfo: %s\n", gai_strerror( error ) );
      return( FALSE );
    }

    if( res0 == NULL ) { // TODO ?????
      // Simple Lookup failed, assume the Server Name to be an DNS Name
      return( TRUE );
    }

    for( res = res0; res != NULL; res = res->ai_next ) {
      if( etcIsValidRemoteSocketAddress( res ) ) {
        freeaddrinfo( res0 );
        return( TRUE );
      }
    }

    freeaddrinfo( res0 );

  }

  return( FALSE );
}

// ===========================================================================

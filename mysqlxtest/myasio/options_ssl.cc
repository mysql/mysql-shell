/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "openssl/ssl.h"

#if defined(HAVE_YASSL)
using namespace yaSSL;
#endif // defined(HAVE_YASSL)

#include "myasio/options_ssl.h"
#include "ngs/memory.h"

using namespace ngs;

#ifdef HAVE_YASSL

static char *my_asn1_time_to_string(ASN1_TIME *time, char *buf, size_t len)
{
  return yaSSL_ASN1_TIME_to_string(time, buf, len);
}

#else /* openssl */

static char *my_asn1_time_to_string(ASN1_TIME *time, char *buf, size_t len)
{
  int n_read;
  char *res= NULL;
  BIO *bio= BIO_new(BIO_s_mem());

  if (bio == NULL)
    return NULL;

  if (!ASN1_TIME_print(bio, time))
    goto end;

  n_read= BIO_read(bio, buf, (int) (len - 1));

  if (n_read > 0)
  {
    buf[n_read]= 0;
    res= buf;
  }

end:
  BIO_free(bio);
  return res;
}

#endif

std::string Options_session_ssl::ssl_cipher()
{
  return SSL_get_cipher(m_ssl);
}

std::string Options_session_ssl::ssl_version()
{
  return SSL_get_version(m_ssl);
}

std::vector<std::string> Options_session_ssl::ssl_cipher_list()
{
  std::vector<std::string> result;
  const char *cipher = NULL;
  int         index = 0;

  for(;;)
  {
     cipher = SSL_get_cipher_list(m_ssl, index++);

     if (NULL == cipher)
       break;

     result.push_back(cipher);
  }

  return result;
}

long Options_session_ssl::ssl_verify_depth()
{
  return SSL_get_verify_depth(m_ssl);
}

long Options_session_ssl::ssl_verify_mode()
{
  return SSL_get_verify_mode(m_ssl);
}

long Options_session_ssl::ssl_sessions_reused()
{
  return SSL_session_reused(m_ssl);
}

long Options_session_ssl::ssl_get_verify_result_and_cert()
{
  long result = 0;

  if (X509_V_OK != (result = SSL_get_verify_result(m_ssl)))
    return result;

  X509 *cert = NULL;
  if (!(cert= SSL_get_peer_certificate(m_ssl)))
    return -1;

  X509_free(cert);

  return X509_V_OK;
}

std::string Options_session_ssl::ssl_get_peer_certificate_issuer()
{
  X509 *cert = NULL;
  if (!(cert= SSL_get_peer_certificate(m_ssl)))
    return "";

  std::string result = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);;
  X509_free(cert);

  return result;
}

std::string Options_session_ssl::ssl_get_peer_certificate_subject()
{
  X509 *cert = NULL;
  if (!(cert= SSL_get_peer_certificate(m_ssl)))
    return "";

  std::string result = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);;
  X509_free(cert);

  return result;
}


long Options_context_ssl::ssl_ctx_verify_depth()
{
  return SSL_CTX_get_verify_depth(m_ctx);
}

long Options_context_ssl::ssl_ctx_verify_mode()
{
  return SSL_CTX_get_verify_mode(m_ctx);
}

std::string Options_context_ssl::ssl_server_not_after()
{
  char       buff[100];
  Custom_allocator<SSL>::Unique_ptr ssl(SSL_new(m_ctx), SSL_free);

  X509      *cert = SSL_get_certificate(ssl.get());
  ASN1_TIME *not_after = X509_get_notAfter(cert);

  const char *result = my_asn1_time_to_string(not_after, buff,
                                              sizeof(buff));
  if (!result)
    return "";

  return result;
}

std::string Options_context_ssl::ssl_server_not_before()
{
  char       buff[100];
  Custom_allocator<SSL>::Unique_ptr ssl(SSL_new(m_ctx), SSL_free);

  X509       *cert = SSL_get_certificate(ssl.get());
  ASN1_TIME  *not_before = X509_get_notBefore(cert);
  const char *result = my_asn1_time_to_string(not_before, buff,
                                              sizeof(buff));
  if (!result)
    return "";

  return result;
}

long Options_context_ssl::ssl_sess_accept_good()
{
  return SSL_CTX_sess_accept_good(m_ctx);
}

long Options_context_ssl::ssl_sess_accept()
{
  return SSL_CTX_sess_accept(m_ctx);
}

long Options_context_ssl::ssl_accept_renegotiates()
{
  return SSL_CTX_sess_accept_renegotiate(m_ctx);
}

long Options_context_ssl::ssl_session_cache_hits()
{
  return SSL_CTX_sess_hits(m_ctx);
}

long Options_context_ssl::ssl_session_cache_misses()
{
  return SSL_CTX_sess_misses(m_ctx);
}

std::string Options_context_ssl::ssl_session_cache_mode()
{
  switch (SSL_CTX_get_session_cache_mode(m_ctx))
  {
  case SSL_SESS_CACHE_OFF:
    return "OFF";

  case SSL_SESS_CACHE_CLIENT:
    return "CLIENT";

  case SSL_SESS_CACHE_SERVER:
    return "SERVER";

  case SSL_SESS_CACHE_BOTH:
    return "BOTH";

  case SSL_SESS_CACHE_NO_AUTO_CLEAR:
    return "NO_AUTO_CLEAR";

  case SSL_SESS_CACHE_NO_INTERNAL_LOOKUP:
    return "NO_INTERNAL_LOOKUP";

  default:
    return "Unknown";
  }
}

long Options_context_ssl::ssl_session_cache_overflows()
{
  return SSL_CTX_sess_cache_full(m_ctx);
}

long Options_context_ssl::ssl_session_cache_size()
{
  return SSL_CTX_sess_get_cache_size(m_ctx);
}

long Options_context_ssl::ssl_session_cache_timeouts()
{
  return SSL_CTX_sess_timeouts(m_ctx);
}

long Options_context_ssl::ssl_used_session_cache_entries()
{
  return SSL_CTX_sess_number(m_ctx);
}

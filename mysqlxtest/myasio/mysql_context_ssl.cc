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

#include <string>
#include <stdexcept>
#include "openssl/ssl.h"

#if defined(HAVE_YASSL)
using namespace yaSSL;
#endif // defined(HAVE_YASSL)


namespace mysqld
{

// file is copy of vio/viosslfactories.c
// converted from C to C++. Base file should be refactore and
// export outside server to be avail for plugins

/*
  Diffie-Hellman key.
  Generated using: >openssl dhparam -5 -C 2048

  -----BEGIN DH PARAMETERS-----
  MIIBCAKCAQEAil36wGZ2TmH6ysA3V1xtP4MKofXx5n88xq/aiybmGnReZMviCPEJ
  46+7VCktl/RZ5iaDH1XNG1dVQmznt9pu2G3usU+k1/VB4bQL4ZgW4u0Wzxh9PyXD
  glm99I9Xyj4Z5PVE4MyAsxCRGA1kWQpD9/zKAegUBPLNqSo886Uqg9hmn8ksyU9E
  BV5eAEciCuawh6V0O+Sj/C3cSfLhgA0GcXp3OqlmcDu6jS5gWjn3LdP1U0duVxMB
  h/neTSCSvtce4CAMYMjKNVh9P1nu+2d9ZH2Od2xhRIqMTfAS1KTqF3VmSWzPFCjG
  mjxx/bg6bOOjpgZapvB6ABWlWmRmAAWFtwIBBQ==
  -----END DH PARAMETERS-----
 */
static unsigned char dh2048_p[]=
{
  0x8A, 0x5D, 0xFA, 0xC0, 0x66, 0x76, 0x4E, 0x61, 0xFA, 0xCA, 0xC0, 0x37,
  0x57, 0x5C, 0x6D, 0x3F, 0x83, 0x0A, 0xA1, 0xF5, 0xF1, 0xE6, 0x7F, 0x3C,
  0xC6, 0xAF, 0xDA, 0x8B, 0x26, 0xE6, 0x1A, 0x74, 0x5E, 0x64, 0xCB, 0xE2,
  0x08, 0xF1, 0x09, 0xE3, 0xAF, 0xBB, 0x54, 0x29, 0x2D, 0x97, 0xF4, 0x59,
  0xE6, 0x26, 0x83, 0x1F, 0x55, 0xCD, 0x1B, 0x57, 0x55, 0x42, 0x6C, 0xE7,
  0xB7, 0xDA, 0x6E, 0xD8, 0x6D, 0xEE, 0xB1, 0x4F, 0xA4, 0xD7, 0xF5, 0x41,
  0xE1, 0xB4, 0x0B, 0xE1, 0x98, 0x16, 0xE2, 0xED, 0x16, 0xCF, 0x18, 0x7D,
  0x3F, 0x25, 0xC3, 0x82, 0x59, 0xBD, 0xF4, 0x8F, 0x57, 0xCA, 0x3E, 0x19,
  0xE4, 0xF5, 0x44, 0xE0, 0xCC, 0x80, 0xB3, 0x10, 0x91, 0x18, 0x0D, 0x64,
  0x59, 0x0A, 0x43, 0xF7, 0xFC, 0xCA, 0x01, 0xE8, 0x14, 0x04, 0xF2, 0xCD,
  0xA9, 0x2A, 0x3C, 0xF3, 0xA5, 0x2A, 0x83, 0xD8, 0x66, 0x9F, 0xC9, 0x2C,
  0xC9, 0x4F, 0x44, 0x05, 0x5E, 0x5E, 0x00, 0x47, 0x22, 0x0A, 0xE6, 0xB0,
  0x87, 0xA5, 0x74, 0x3B, 0xE4, 0xA3, 0xFC, 0x2D, 0xDC, 0x49, 0xF2, 0xE1,
  0x80, 0x0D, 0x06, 0x71, 0x7A, 0x77, 0x3A, 0xA9, 0x66, 0x70, 0x3B, 0xBA,
  0x8D, 0x2E, 0x60, 0x5A, 0x39, 0xF7, 0x2D, 0xD3, 0xF5, 0x53, 0x47, 0x6E,
  0x57, 0x13, 0x01, 0x87, 0xF9, 0xDE, 0x4D, 0x20, 0x92, 0xBE, 0xD7, 0x1E,
  0xE0, 0x20, 0x0C, 0x60, 0xC8, 0xCA, 0x35, 0x58, 0x7D, 0x3F, 0x59, 0xEE,
  0xFB, 0x67, 0x7D, 0x64, 0x7D, 0x8E, 0x77, 0x6C, 0x61, 0x44, 0x8A, 0x8C,
  0x4D, 0xF0, 0x12, 0xD4, 0xA4, 0xEA, 0x17, 0x75, 0x66, 0x49, 0x6C, 0xCF,
  0x14, 0x28, 0xC6, 0x9A, 0x3C, 0x71, 0xFD, 0xB8, 0x3A, 0x6C, 0xE3, 0xA3,
  0xA6, 0x06, 0x5A, 0xA6, 0xF0, 0x7A, 0x00, 0x15, 0xA5, 0x5A, 0x64, 0x66,
  0x00, 0x05, 0x85, 0xB7,
};

static unsigned char dh2048_g[]={
  0x05,
};


static DH *get_dh2048(void)
{
  DH *dh;
  if ((dh=DH_new()))
  {
    dh->p=BN_bin2bn(dh2048_p,sizeof(dh2048_p),NULL);
    dh->g=BN_bin2bn(dh2048_g,sizeof(dh2048_g),NULL);
    if (! dh->p || ! dh->g)
    {
      DH_free(dh);
      dh=0;
    }
  }
  return(dh);
}


std::string sslGetErrString(const std::string &base_message)
{
  //todo: add ssl error messgae (not avail in yassl)
  return base_message;
}

std::string not_empty_string(const std::string &input)
{
  return input.empty()? "NULL" : input;
}

void set_context_cert(SSL_CTX *ctx, std::string cert_file, std::string key_file)
{
  if (cert_file.empty() &&  key_file.size())
    cert_file = key_file;

  if (key_file.empty() &&  cert_file.size())
    key_file = cert_file;

  if (cert_file.size() &&
      0 >= SSL_CTX_use_certificate_file(ctx, cert_file.c_str(), SSL_FILETYPE_PEM))
  {
    throw std::runtime_error(std::string("Unable to get certificate: ") + cert_file);
  }

  if (key_file.size() &&
      SSL_CTX_use_PrivateKey_file(ctx, key_file.c_str(), SSL_FILETYPE_PEM) <= 0)
  {
    throw std::runtime_error(std::string("Unable to get private key: ") + key_file);
  }

  /*
    If we are using DSA, we can copy the parameters from the private key
    Now we know that a key and cert have been set against the SSL context
  */
  if (cert_file.size() && !SSL_CTX_check_private_key(ctx))
  {
    throw std::runtime_error("Private key does not match the certificate public key");
  }
}

void set_context(SSL_CTX* ssl_context, const bool is_client, const std::string &ssl_key,
    const std::string &ssl_cert,    const std::string &ssl_ca,
    const std::string &ssl_ca_path, const std::string &ssl_cipher,
    const std::string &ssl_crl,     const std::string &ssl_crl_path)
{
  //log_info("key_file: '%s'  cert_file: '%s'  ca_file: '%s'  ca_path: '%s'  "
  //         "cipher: '%s' crl_file: '%s' crl_path: '%s' ",
  //         not_empty_string(ssl_key).c_str(),
  //         not_empty_string(ssl_cert).c_str(),
  //         not_empty_string(ssl_ca).c_str(),
  //         not_empty_string(ssl_ca_path).c_str(),
  //         not_empty_string(ssl_cipher).c_str(),
  //         not_empty_string(ssl_crl).c_str(),
  //         not_empty_string(ssl_crl_path).c_str());

  SSL_CTX_set_options(ssl_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

  /*
    Set the ciphers that can be used
    NOTE: SSL_CTX_set_cipher_list will return 0 if
    none of the provided ciphers could be selected
  */
  if (ssl_cipher.length() &&
      0 == SSL_CTX_set_cipher_list(ssl_context, ssl_cipher.c_str()))
  {
    throw std::runtime_error("Failed to set ciphers to use");
  }

  /* Load certs from the trusted ca */
  if (0 == SSL_CTX_load_verify_locations(ssl_context,
                                         ssl_ca.empty() ? NULL : ssl_ca.c_str(),
                                         ssl_ca_path.empty() ? NULL : ssl_ca_path.c_str()))
  {
    //log_warning("SSL_CTX_load_verify_locations failed");

    if (ssl_ca.size() || ssl_ca_path.size())
    {
      /* fail only if ca file or ca path were supplied and looking into
         them fails. */
      throw std::runtime_error(sslGetErrString("SSL_CTX_load_verify_locations failed"));
    }

    /* otherwise go use the defaults */
    if (0 == SSL_CTX_set_default_verify_paths(ssl_context))
    {
      throw std::runtime_error(sslGetErrString("SSL_CTX_set_default_verify_paths failed"));
    }
  }

  if (ssl_crl.size() || ssl_crl_path.size())
  {
#ifdef HAVE_YASSL
    //log_warning("yaSSL doesn't support CRL");
#else
    X509_STORE *store= SSL_CTX_get_cert_store(ssl_context);
    /* Load crls from the trusted ca */
    if (0 == X509_STORE_load_locations(store, ssl_crl.c_str(), ssl_crl_path.c_str()) ||
        0 == X509_STORE_set_flags(store,
                                  X509_V_FLAG_CRL_CHECK |
                                  X509_V_FLAG_CRL_CHECK_ALL))
    {
      throw std::runtime_error(sslGetErrString("SSL_CTX_get_cert_store"));
    }
#endif
  }

  set_context_cert(ssl_context, ssl_cert, ssl_key);

  /* Server specific check : Must have certificate and key file */
  if (!is_client && ssl_key.empty() && ssl_cert.empty())
  {
    throw std::runtime_error(sslGetErrString("SSL context is not usable without certificate and private key"));
  }
  else
  {
    SSL_CTX_set_verify(ssl_context, SSL_VERIFY_NONE, NULL);
  }

  /* DH stuff */
  DH *dh = get_dh2048();

  if (SSL_CTX_set_tmp_dh(ssl_context, dh) == 0)
  {
    DH_free(dh);
    throw std::runtime_error(sslGetErrString("SSL_CTX_set_tmp_dh failed"));
  }
  DH_free(dh);
}

} // namespace ngs

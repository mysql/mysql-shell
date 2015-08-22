/* Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#ifndef _NGS_ASIO_WRAPPER_YASSL_H_
#define _NGS_ASIO_WRAPPER_YASSL_H_

#include "openssl/ssl.h"

using namespace yaSSL;

#include <boost/system/error_code.hpp>

#include "yassl_types.hpp"
#include "yassl_int.hpp"
#include "myasio/wrapper_ssl.h"
#include "myasio/options_ssl.h"
#include "myasio/memory.h"


namespace ngs
{
  class Wrapper_yassl_error : public boost::system::error_category
  {
  public:
    const char* name() const BOOST_SYSTEM_NOEXCEPT
    {
      return "asio.yassl";
    }

    std::string message(int value) const
    {
      switch(value)
      {
      case range_error:
        return "Range error";

      case realloc_error:
        return "Realloc error";

      case factory_error:
        return "Factory error";

      case unknown_cipher:
        return "Unknown cipher";

      case prefix_error:
        return "Prefix error";

      case record_layer:
        return "Record error";

      case handshake_layer:
        return "Handshake error";

      case out_of_order:
        return "Data out of order";

      case bad_input:
        return "Bad input error";

      case match_error:
        return "SSL verification failed";

      case no_key_file:
        return "Key file not set";

      case verify_error:
        return "SSL verification failed";

      case send_error:
        return "Error while sending data";

      case receive_error:
        return "Error while receiving data";

      case certificate_error:
        return "Certificate error";

      case privateKey_error:
        return "Private key error";

      case badVersion_error:
        return "SSL bad version";

      case compress_error:
        return "Compression error";

      case decompress_error:
        return "Decompression error";

      case pms_version_error:
        return "PMS version error";

      case sanityCipher_error:
        return "Sanity cipher error";

      default:
        return "Unknown error";
      }
    }
  };


  class Wrapper_yassl : public Wrapper_ssl
  {
  public:
    Wrapper_yassl(boost::shared_ptr<SSL_CTX> context)
    : ssl_ctxt(context),
      ssl(Custom_allocator<SSL>::Unique_ptr(SSL_new(ssl_ctxt.get()), SSL_free))
    {
    }

    virtual boost::shared_ptr<Options_session> get_ssl_options()
    {
      return boost::make_shared<Options_session_ssl>(ssl.get());
    }

    virtual void ssl_set_error_want_read()
    {
      ssl_set_error(SSL_ERROR_WANT_READ);
    }

    virtual bool ssl_is_no_fatal_error()
    {
      const int result = ssl_get_error();

      return no_error == result;
    }

    virtual bool ssl_is_error_would_block()
    {
      const int result = ssl_get_error();

      return SSL_ERROR_WANT_READ == result ||
             SSL_ERROR_WANT_WRITE == result;
    }

    virtual void ssl_set_error_none()
    {
      ssl_set_error(no_error);
    }

    virtual bool ssl_handshake()
    {
      if (yaSSL::server_end == ssl_ctxt->getMethod()->getSide())
        return SSL_SUCCESS == SSL_accept(ssl.get());

      return SSL_SUCCESS == SSL_connect(ssl.get());
    }

    virtual int ssl_read(void* buffer, int sz)
    {
      int result = SSL_read(ssl.get(), buffer, sz);

      if (SSL_WOULD_BLOCK == result)
        return 0;

      return result;
    }

    virtual int ssl_write(const void* buffer, int sz )
    {
      return SSL_write(ssl.get(), buffer, sz);
    }

    virtual void ssl_set_fd(int file_descriptor)
    {
      SSL_set_fd(ssl.get(), file_descriptor);
    }

    virtual void ssl_set_error(int error)
    {
      ssl->SetError(static_cast<YasslError>(error));
    }

    virtual void ssl_set_socket_error(int error)
    {
      ssl->useSocket().set_lastError(error);
    }

    virtual void ssl_set_transport_recv(Socket_recv socket_recv)
    {
      ssl->useSocket().set_transport_recv_function(socket_recv);
    }

    virtual void ssl_set_transport_send(Socket_send socket_send)
    {
      ssl->useSocket().set_transport_send_function(socket_send);
    }

    virtual void ssl_set_transport_data(void *error)
    {
      ssl->useSocket().set_transport_ptr(error);
    }

    virtual boost::system::error_code get_boost_error()
    {
      int error_code = ssl_get_error();
      static Wrapper_yassl_error yassl_error_category;

      if (no_error == error_code)
        return boost::system::error_code();

      return boost::system::error_code(static_cast<int>(error_code), yassl_error_category);
    }

  private:
    boost::shared_ptr<SSL_CTX>        ssl_ctxt;
    Custom_allocator<SSL>::Unique_ptr ssl;

    int ssl_get_error()
    {
      return SSL_get_error(ssl.get(), 0);
    }
  };

} // namespace ngs

#endif // _NGS_ASIO_WRAPPER_YASSL_H_

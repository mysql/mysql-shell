/* Copyright (c) 2015, 2016 Oracle and/or its affiliates. All rights reserved.

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

#if defined(HAVE_YASSL)
#include "myasio/wrapper_yassl.h"
#include "yassl_types.hpp"
#include "yassl_int.hpp"

using namespace yaSSL;

#include "myasio/options_ssl.h"
#include "ngs/memory.h"

#include <boost/move/move.hpp>

namespace ngs
{


  const char* Wrapper_yassl_error::name() const BOOST_SYSTEM_NOEXCEPT
  {
    return "asio.yassl";
  }

  std::string Wrapper_yassl_error::message(int value) const
  {
    switch (value)
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


  Wrapper_yassl::Wrapper_yassl(boost::shared_ptr<SSL_CTX> context)
  : ssl_ctxt(context)
  {
  }

  void Wrapper_yassl::ssl_initialize()
  {
    if (!ssl)
    {
      Custom_allocator<SSL>::Unique_ptr alloc_ssl(SSL_new(ssl_ctxt.get()), SSL_free);
      ssl = boost::move(alloc_ssl);
    }
  }

  boost::shared_ptr<IOptions_session> Wrapper_yassl::get_ssl_options()
  {
    return boost::make_shared<Options_session_ssl>(ssl.get());
  }

  void Wrapper_yassl::ssl_set_error_want_read()
  {
    ssl_set_error(SSL_ERROR_WANT_READ);
  }

  bool Wrapper_yassl::ssl_is_no_fatal_error()
  {
    const int result = ssl_get_error();

    return no_error == result;
  }

  bool Wrapper_yassl::ssl_is_error_would_block()
  {
    const int result = ssl_get_error();

    return SSL_ERROR_WANT_READ == result ||
        SSL_ERROR_WANT_WRITE == result;
  }

  void Wrapper_yassl::ssl_set_error_none()
  {
    ssl_set_error(no_error);
  }

  bool Wrapper_yassl::ssl_handshake()
  {
    if (yaSSL::server_end == ssl_ctxt->getMethod()->getSide())
    return SSL_SUCCESS == SSL_accept(ssl.get());

    return SSL_SUCCESS == SSL_connect(ssl.get());
  }

  int Wrapper_yassl::ssl_read(void* buffer, int sz)
  {
    int result = SSL_read(ssl.get(), buffer, sz);

    if (SSL_WOULD_BLOCK == result)
    return 0;

    return result;
  }

  int Wrapper_yassl::ssl_write(const void* buffer, int sz)
  {
    return SSL_write(ssl.get(), buffer, sz);
  }

  void Wrapper_yassl::ssl_set_fd(int file_descriptor)
  {
    SSL_set_fd(ssl.get(), file_descriptor);
  }

  void Wrapper_yassl::ssl_set_error(int error)
  {
    ssl->SetError(static_cast<YasslError>(error));
  }

  void Wrapper_yassl::ssl_set_socket_error(int error)
  {
    ssl->useSocket().set_lastError(error);
  }

  void Wrapper_yassl::ssl_set_transport_recv(Socket_recv socket_recv)
  {
    ssl->useSocket().set_transport_recv_function(socket_recv);
  }

  void Wrapper_yassl::ssl_set_transport_send(Socket_send socket_send)
  {
    ssl->useSocket().set_transport_send_function(socket_send);
  }

  void Wrapper_yassl::ssl_set_transport_data(void *error)
  {
    ssl->useSocket().set_transport_ptr(error);
  }

  boost::system::error_code Wrapper_yassl::get_boost_error()
  {
    int error_code = ssl_get_error();
    static Wrapper_yassl_error yassl_error_category;

    if (no_error == error_code)
    return boost::system::error_code();

    return boost::system::error_code(static_cast<int>(error_code), yassl_error_category);
  }

  int Wrapper_yassl::ssl_get_error()
  {
    return SSL_get_error(ssl.get(), 0);
  }

} // namespace ngs

#endif // HAVE_YASSL

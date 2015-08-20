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

#ifndef _NGS_ASIO_WRAPPER_SSL_
#define _NGS_ASIO_WRAPPER_SSL_

#include <boost/system/error_code.hpp>
#include <boost/shared_ptr.hpp>


namespace ngs
{

  typedef long (*Socket_recv)(void *, void *, std::size_t );
  typedef long (*Socket_send)(void *, const void *, std::size_t );

  class Options_session;

  // Separate caller from ssl functions, macros, consts etc.
  // Unit test from Connection_yassl can be run without real ssl
  // in both ssl configuration system & bundle
  class Wrapper_ssl
  {
  public:
    virtual ~Wrapper_ssl() {}

    virtual boost::system::error_code get_boost_error() = 0;
    virtual void ssl_set_error_none() = 0;
    virtual void ssl_set_error_want_read() = 0;
    virtual bool ssl_is_no_fatal_error() = 0;
    virtual bool ssl_is_error_would_block() = 0;

    virtual void ssl_set_socket_error(int error) = 0;

    virtual boost::shared_ptr<Options_session> get_ssl_options() = 0;

    virtual bool ssl_handshake() = 0;
    virtual int ssl_read(void* buffer, int sz) = 0;
    virtual int ssl_write(const void* buffer, int sz ) = 0;
    virtual void ssl_set_fd(int file_descriptor) = 0;

    virtual void ssl_set_transport_recv(Socket_recv socket_recv) = 0;
    virtual void ssl_set_transport_send(Socket_send socket_send) = 0;
    virtual void ssl_set_transport_data(void *error) = 0;
  };

} // namespace ngs

#endif // _NGS_ASIO_WRAPPER_SSL_

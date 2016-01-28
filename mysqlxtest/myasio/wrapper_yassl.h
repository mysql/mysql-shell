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

#ifndef _NGS_ASIO_WRAPPER_YASSL_H_
#define _NGS_ASIO_WRAPPER_YASSL_H_

#include "openssl/ssl.h"
#include <boost/system/error_code.hpp>

#include "myasio/wrapper_ssl.h"
#include "ngs/memory.h"

namespace ngs
{
  class IOptions_session;

  class Wrapper_yassl_error : public boost::system::error_category
  {
  public:
    const char* name() const;
    std::string message(int value) const;
  };

  class Wrapper_yassl : public IWrapper_ssl
  {
  public:
    Wrapper_yassl(boost::shared_ptr<yaSSL::SSL_CTX> context);

    virtual void ssl_initialize();

    virtual boost::shared_ptr<IOptions_session> get_ssl_options();

    virtual void ssl_set_error_want_read();

    virtual bool ssl_is_no_fatal_error();

    virtual bool ssl_is_error_would_block();

    virtual void ssl_set_error_none();

    virtual bool ssl_handshake();

    virtual int ssl_read(void* buffer, int sz);

    virtual int ssl_write(const void* buffer, int sz);

    virtual void ssl_set_fd(int file_descriptor);

    virtual void ssl_set_error(int error);

    virtual void ssl_set_socket_error(int error);

    virtual void ssl_set_transport_recv(Socket_recv socket_recv);

    virtual void ssl_set_transport_send(Socket_send socket_send);

    virtual void ssl_set_transport_data(void *error);

    virtual boost::system::error_code get_boost_error();

  private:
    boost::shared_ptr<yaSSL::SSL_CTX>        ssl_ctxt;
    Custom_allocator<yaSSL::SSL>::Unique_ptr ssl;

    int ssl_get_error();
  };
} // namespace ngs

#endif // _NGS_ASIO_WRAPPER_YASSL_H_

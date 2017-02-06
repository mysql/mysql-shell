/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db


#ifndef _MYSQLX_SYNC_CONNECTION_H_
#define _MYSQLX_SYNC_CONNECTION_H_


#include <boost/system/error_code.hpp>
#include "myasio/connection.h"
#include "myasio/connection_factory.h"


namespace mysqlx
{

class Mysqlx_sync_connection
{
public:
  Mysqlx_sync_connection(boost::asio::io_service &service, const char *ssl_key = NULL,
                         const char *ssl_ca = NULL, const char *ssl_ca_path = NULL,
                         const char *ssl_cert = NULL, const char *ssl_cipher = NULL,
                         const char *ssl_crl = NULL, const char *ssl_crl_path = NULL,
                         const char *ssl_tls_version = NULL, int ssl_mode = 2 /*PREFERRED*/,
                         const std::size_t timeout = 0l);

  boost::system::error_code connect(const ngs::Endpoint &);
  boost::system::error_code accept(const ngs::Endpoint &);
  boost::system::error_code activate_tls();
  boost::system::error_code shutdown(boost::asio::socket_base::shutdown_type how_to_shutdown);

  boost::system::error_code write(const void *data, const std::size_t data_length);
  boost::system::error_code read(void *data, const std::size_t data_length);
  boost::system::error_code read_with_timeout(void *data, std::size_t &data_length, const std::size_t deadline_miliseconds);

  void close();

  bool supports_ssl();

private:

  static bool is_set(const char *string);
  ngs::Connection_factory_ptr get_async_connection_factory(const char *ssl_key,  const char *ssl_ca, const char *ssl_ca_path,
                                                           const char *ssl_cert, const char *ssl_cipher, const char *ssl_crl, 
                                                           const char *ssl_crl_path, const char *ssl_tls_version,
                                                           int ssl_mode);

  boost::asio::io_service    &m_service;
  ngs::Connection_factory_ptr m_async_factory;
  ngs::IConnection_ptr         m_async_connection;
  const std::size_t           m_timeout;
};


} // namespace mysqlx


#endif // _MYSQLX_SYNC_CONNECTION_H_

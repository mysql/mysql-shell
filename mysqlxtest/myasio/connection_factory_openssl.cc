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

#if !defined(HAVE_YASSL)

#include <boost/asio/ssl.hpp>
#include <boost/make_shared.hpp>

#include "myasio/connection_factory_openssl.h"
#include "myasio/connection_openssl.h"
#include "myasio/options_ssl.h"
#include "myasio/mysql_context_ssl.h"
#include "myasio/connection_dynamic_tls.h"


using namespace ngs;

Connection_openssl_factory::Connection_openssl_factory(const std::string &ssl_key,
                                                       const std::string &ssl_cert,    const std::string &ssl_ca,
                                                       const std::string &ssl_ca_path, const std::string &ssl_cipher,
                                                       const std::string &ssl_crl,     const std::string &ssl_crl_path,
                                                       const bool is_client)
: m_context(new boost::asio::ssl::context(is_client ?
                                          boost::asio::ssl::context::tlsv11_client :
                                          boost::asio::ssl::context::tlsv11_server)),
  m_is_client(is_client)
{
  try
  {
    m_context->set_options(boost::asio::ssl::context::default_workarounds |
                           boost::asio::ssl::context::no_sslv2 |
                           boost::asio::ssl::context::single_dh_use);

    mysqld::set_context(m_context->native_handle(), is_client, ssl_key, ssl_cert, ssl_ca,
                        ssl_ca_path, ssl_cipher, ssl_crl, ssl_crl_path);
  }
  catch (const std::exception &e)
  {
    delete m_context;
    m_context = NULL;
    throw;
  }
}

Connection_openssl_factory::~Connection_openssl_factory()
{
  delete m_context;
}

Connection_unique_ptr Connection_openssl_factory::create_connection(boost::asio::io_service &io_service)
{
  Connection_unique_ptr connection(new Connection_openssl(io_service, boost::ref(*m_context), m_is_client));

  return Connection_unique_ptr(new Connection_dynamic_tls(boost::move(connection)));
}


Options_context_ptr Connection_openssl_factory::create_ssl_context_options()
{
  return boost::make_shared<Options_context_ssl>(m_context->native_handle());
}

#endif // !defined(HAVE_YASSL)

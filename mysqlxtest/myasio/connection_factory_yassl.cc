/*
 * Copyright (c) 2015, 2016 Oracle and/or its affiliates. All rights reserved.
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

#if defined(HAVE_YASSL)

#include "myasio/connection_factory_raw.h"
#include "myasio/wrapper_yassl.h"

#include "myasio/connection_factory_yassl.h"
#include "myasio/connection_yassl.h"
#include "myasio/connection_dynamic_tls.h"

#include <boost/make_shared.hpp>
#include <boost/array.hpp>

using namespace yaSSL;
#include "myasio/mysql_context_ssl.h"
#include "myasio/options_ssl.h"

namespace ngs
{
  Connection_yassl_factory::Connection_yassl_factory(const std::string &ssl_key,
                                                     const std::string &ssl_cert, const std::string &ssl_ca,
                                                     const std::string &ssl_ca_path, const std::string &ssl_cipher,
                                                     const std::string &ssl_crl, const std::string &ssl_crl_path,
                                                     const bool is_client)
  : m_is_client(is_client)
  {
    SSL_METHOD *method = m_is_client ? TLSv1_1_client_method() : TLSv1_1_server_method();

    ssl_ctxt = boost::shared_ptr<SSL_CTX>(SSL_CTX_new(method), SSL_CTX_free);

    mysqld::set_context(ssl_ctxt.get(), m_is_client, ssl_key, ssl_cert, ssl_ca,
                        ssl_ca_path, ssl_cipher, ssl_crl, ssl_crl_path);
  }

  IConnection_unique_ptr Connection_yassl_factory::create_connection(boost::asio::io_service &io_service)
  {
    Wrapper_ssl_ptr        wrapper_ssl(new Wrapper_yassl(ssl_ctxt));
    IConnection_unique_ptr connection(Connection_raw_factory().create_connection(io_service));

    Vector_states_yassl  handlers;

    if (m_is_client)
      handlers.push_back(boost::make_shared<State_handshake_client_yassl>(boost::ref(*wrapper_ssl)));
    else
      handlers.push_back(boost::make_shared<State_handshake_server_yassl>(boost::ref(*wrapper_ssl)));

    handlers.push_back(boost::make_shared<State_running_yassl>(boost::ref(*wrapper_ssl)));
    handlers.push_back(boost::make_shared<State_stop_yassl>(boost::ref(*wrapper_ssl)));

    IConnection_unique_ptr connection_ssl(new Connection_yassl(boost::move(connection), boost::move(wrapper_ssl), handlers));

    return IConnection_unique_ptr(new Connection_dynamic_tls(boost::move(connection_ssl)));
  }

  IOptions_context_ptr Connection_yassl_factory::create_ssl_context_options()
  {
    return IOptions_context_ptr(new Options_context_ssl(ssl_ctxt.get()));
  }
} // namespace ngs

#endif // defined(HAVE_YASSL)
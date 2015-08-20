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

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include "myasio/connection_dynamic_tls.h"


namespace ngs
{

class Connection_dynamic_tls::Options_dynamic : public Options_session
{
public:
  Options_dynamic(const Options_session_ptr options)
  : m_options(options)
  {}

  bool supports_tls() { return true; }
  bool active_tls() { return m_options->active_tls(); }

  std::vector<std::string> ssl_cipher_list() { return m_options->ssl_cipher_list(); }

  std::string ssl_cipher() { return m_options->ssl_cipher();  }
  std::string ssl_version() { return m_options->ssl_version(); }

  long ssl_verify_depth() { return m_options->ssl_verify_depth(); }
  long ssl_verify_mode() { return m_options->ssl_verify_mode(); }

  long ssl_sessions_reused() { return m_options->ssl_sessions_reused(); }

private:
  Options_session_ptr m_options;
};


Connection_dynamic_tls::Connection_dynamic_tls(Connection_unique_ptr connection)
: m_connection(boost::move(connection)),
  m_operational_mode(m_connection->get_lowest_layer())
{
  //assert(m_connection->is_option_set(Option_support_tls));
}

Connection_dynamic_tls::~Connection_dynamic_tls()
{
}

Endpoint Connection_dynamic_tls::get_remote_endpoint() const
{
  return m_operational_mode->get_remote_endpoint();
}

int Connection_dynamic_tls::get_socket_id()
{
  return m_operational_mode->get_socket_id();
}

Options_session_ptr Connection_dynamic_tls::options()
{
  return boost::make_shared<Options_dynamic>(m_operational_mode->options());
}

Connection_unique_ptr Connection_dynamic_tls::get_lowest_layer()
{
  return m_connection->get_lowest_layer();
}

void Connection_dynamic_tls::async_connect(const Endpoint &endpoint,
                                           const On_asio_status_callback &on_connect_callback,
                                           const On_asio_status_callback &on_ready_callback)
{
  m_operational_mode->async_connect(endpoint, on_connect_callback, on_ready_callback);
}

void Connection_dynamic_tls::async_accept(boost::asio::ip::tcp::acceptor &acceptor,
                                          const On_asio_status_callback &on_accept_callback,
                                          const On_asio_status_callback &on_ready_callback)
{
  m_operational_mode->async_accept(acceptor, on_accept_callback, on_ready_callback);
}

void Connection_dynamic_tls::async_write(const Const_buffer_sequence &data, const On_asio_data_callback &on_write_callback)
{
  m_operational_mode->async_write(data, on_write_callback);
}

void Connection_dynamic_tls::async_read(const Mutable_buffer_sequence &data, const On_asio_data_callback &on_read_callback)
{
  m_operational_mode->async_read(data, on_read_callback);
}

void Connection_dynamic_tls::async_activate_tls(const On_asio_status_callback on_status)
{
  m_operational_mode = Connection_unique_ptr(m_connection.get(), boost::none);

  m_operational_mode->async_activate_tls(on_status);
}

void Connection_dynamic_tls::shutdown(boost::asio::socket_base::shutdown_type how_to_shutdown, boost::system::error_code &ec)
{
  m_operational_mode->shutdown(how_to_shutdown, ec);
}

void Connection_dynamic_tls::post(const boost::function<void ()> &calee)
{
  m_operational_mode->post(calee);
}

bool Connection_dynamic_tls::thread_in_connection_strand()
{
  return m_operational_mode->thread_in_connection_strand();
}

void Connection_dynamic_tls::close()
{
  m_connection->close();
}

}  // namespace ngs

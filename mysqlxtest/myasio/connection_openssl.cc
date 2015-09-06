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


//boost ssl is going to fail when using bundled ssl
#if !defined(HAVE_YASSL)

#include <boost/bind.hpp>

#include "myasio/connection_openssl.h"
#include "myasio/connection_raw.h"
#include "myasio/callback.h"
#include "myasio/options_ssl.h"


using namespace ngs;


Connection_openssl::Connection_openssl(boost::asio::io_service &service, boost::asio::ssl::context &context, const bool is_client)
: m_handshake_type(is_client ? boost::asio::ssl::stream_base::client : boost::asio::ssl::stream_base::server),
  m_asio_socket(service, context),
  m_asio_strand(service),
  m_state(State_handshake)
{
}

Connection_openssl::~Connection_openssl()
{
  close();
}

Endpoint Connection_openssl::get_remote_endpoint() const
{
  return m_asio_socket.lowest_layer().remote_endpoint();
}

int Connection_openssl::get_socket_id()
{
  return m_asio_socket.lowest_layer().native_handle();
}

Options_session_ptr Connection_openssl::options()
{
  return boost::make_shared<Options_session_ssl>(m_asio_socket.native_handle());
}

Connection_unique_ptr Connection_openssl::get_lowest_layer()
{
  return Connection_unique_ptr(new Connection_raw<boost::asio::ip::tcp::socket&>(m_asio_socket.next_layer()));
}

void Connection_openssl::async_connect(const Endpoint &endpoint,
                                       const On_asio_status_callback &on_connect_callback,
                                       const On_asio_status_callback &on_ready_callback)
{
  m_accept_callback = on_connect_callback;
  m_ready_callback  = on_ready_callback;

  m_asio_socket.lowest_layer().async_connect(endpoint,
                                             m_asio_strand.wrap(boost::bind(&Connection_openssl::on_accept_try_handshake, this, boost::asio::placeholders::error)));
}

void Connection_openssl::async_accept(boost::asio::ip::tcp::acceptor & acceptor,
                                      const On_asio_status_callback &on_accept_callback,
                                      const On_asio_status_callback &on_ready_callback)
{
  m_accept_callback = on_accept_callback;
  m_ready_callback  = on_ready_callback;

  acceptor.async_accept(m_asio_socket.lowest_layer(), m_asio_strand.wrap(boost::bind(&Connection_openssl::on_accept_try_handshake, this, boost::asio::placeholders::error)));
}

void Connection_openssl::async_write(const Const_buffer_sequence &data, const On_asio_data_callback &on_write_callback)
{
  if (!thread_in_connection_strand())
  {
    m_asio_strand.post(boost::bind(&Connection_openssl::async_write, this, data, on_write_callback));
    return;
  }

  if (State_running != m_state)
  {
    on_write_callback(boost::system::errc::make_error_code(boost::system::errc::state_not_recoverable), 0);
    return;
  }

  m_asio_socket.async_write_some(data, m_asio_strand.wrap(on_write_callback));
}

void Connection_openssl::async_read(const Mutable_buffer_sequence &data, const On_asio_data_callback &on_read_callback)
{
  if (!thread_in_connection_strand())
  {
    m_asio_strand.post(boost::bind(&Connection_openssl::async_read, this, data, on_read_callback));
    return;
  }

  if (State_running != m_state)
  {
    on_read_callback(boost::system::errc::make_error_code(boost::system::errc::state_not_recoverable), 0);
    return;
  }

  m_asio_socket.async_read_some(data, m_asio_strand.wrap(on_read_callback));
}

void Connection_openssl::async_activate_tls(const On_asio_status_callback on_status)
{
  if (on_status)
  {
    m_ready_callback = on_status;
  }

  m_asio_socket.async_handshake(m_handshake_type, m_asio_strand.wrap(boost::bind(&Connection_openssl::on_handshake, this, boost::asio::placeholders::error)));
}

void Connection_openssl::shutdown(boost::asio::socket_base::shutdown_type how_to_shutdown, boost::system::error_code &ec)
{
  m_asio_socket.lowest_layer().shutdown(how_to_shutdown, ec);
}

void Connection_openssl::post(const boost::function<void ()> &calee)
{
  m_asio_strand.post(calee);
}

bool Connection_openssl::thread_in_connection_strand()
{
  return m_asio_strand.running_in_this_thread();
}

void Connection_openssl::close()
{
  if (m_asio_socket.lowest_layer().is_open())
  {
    boost::system::error_code ec;

    m_asio_socket.lowest_layer().close(ec);
  }
}

void Connection_openssl::on_accept_try_handshake(const boost::system::error_code &ec)
{
  Callback_direct().call_status_function(m_accept_callback, ec);

  if (ec)
  {
    return;
  }

  set_socket_option(m_asio_socket.lowest_layer(), boost::asio::ip::tcp::no_delay(true), false);

  async_activate_tls(On_asio_status_callback());
}

void Connection_openssl::on_handshake(const boost::system::error_code &error)
{
  m_state = error ? State_stop :
                    State_running;


  Callback_direct().call_status_function(m_ready_callback, error);
}

#endif // !defined(HAVE_YASSL)

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


#ifndef _NGS_ASIO_CONNECTION_RAW_H_
#define _NGS_ASIO_CONNECTION_RAW_H_


#include "myasio/connection.h"
#include "myasio/callback.h"

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/remove_reference.hpp>


namespace ngs
{

class Options_default : public Options_session
{
public:
  bool supports_tls() { return false; };
  bool active_tls() { return false; };
  std::string ssl_cipher() { return ""; };
  std::vector<std::string> ssl_cipher_list() { return std::vector<std::string>(); };
  std::string ssl_version() { return ""; };

  long ssl_ctx_verify_depth() { return 0; }
  long ssl_ctx_verify_mode() { return 0; }
  long ssl_verify_depth() { return 0; }
  long ssl_verify_mode() { return 0; }

  std::string ssl_server_not_after() { return ""; }
  std::string ssl_server_not_before() { return ""; }
  long ssl_sessions_reused() { return 0; }
};

template <typename Socket_type>
class Connection_raw : public Connection
{
public:
  template<typename Initialize_type>
  Connection_raw(Initialize_type &socket);

  virtual Endpoint    get_remote_endpoint() const;
  virtual int         get_socket_id();
  virtual Options_session_ptr options();

  virtual void async_connect(const Endpoint &endpoint, const On_asio_status_callback &on_connect_callback, const On_asio_status_callback &on_ready_callback);
  virtual void async_accept(boost::asio::ip::tcp::acceptor &, const On_asio_status_callback &on_accept_callback,
                            const On_asio_status_callback &on_ready_callback);
  virtual void async_write(const Const_buffer_sequence &data, const On_asio_data_callback &on_write_callback);
  virtual void async_read(const Mutable_buffer_sequence &data, const On_asio_data_callback &on_read_callback);
  virtual void async_activate_tls(const On_asio_status_callback on_status);

  virtual Connection_unique_ptr get_lowest_layer();

  virtual void post(const boost::function<void ()> &calee);
  virtual bool thread_in_connection_strand();

  virtual void shutdown(boost::asio::socket_base::shutdown_type how_to_shutdown, boost::system::error_code &ec);
  virtual void close();

private:
  boost::asio::io_service &get_service(boost::asio::io_service &service) { return service; };
  boost::asio::io_service &get_service(boost::asio::ip::tcp::socket &service) { return service.get_io_service(); };

  void on_accept(const boost::system::error_code &ec);

  Socket_type          m_asio_socket;
  boost::asio::strand  m_asio_strand;

  On_asio_status_callback m_on_accept_callback;
  On_asio_status_callback m_on_ready_callback;
};


template <typename Socket_type>
template <typename Initialize_type>
Connection_raw<Socket_type>::Connection_raw(Initialize_type &socket)
: m_asio_socket(socket),
  m_asio_strand(get_service(socket))
{
}


template <typename Socket_type>
int Connection_raw<Socket_type>::get_socket_id()
{
  return static_cast<int>(m_asio_socket.native());
}


template <typename Socket_type>
Endpoint Connection_raw<Socket_type>::get_remote_endpoint() const
{
  return m_asio_socket.remote_endpoint();
}


template <typename Socket_type>
Options_session_ptr Connection_raw<Socket_type>::options()
{
  return boost::make_shared<Options_default>();
}


template <typename Socket_type>
Connection_unique_ptr Connection_raw<Socket_type>::get_lowest_layer()
{
  typedef typename boost::remove_reference<Socket_type>::type &Socket_ref_type;

  return Connection_unique_ptr(new Connection_raw<Socket_ref_type>(m_asio_socket));
}


template <typename Socket_type>
void Connection_raw<Socket_type>::async_connect(const Endpoint &endpoint,
                                       const On_asio_status_callback &on_connect_callback,
                                       const On_asio_status_callback &on_ready_callback)
{
  m_on_accept_callback = on_connect_callback;
  m_on_ready_callback = on_ready_callback;
  m_asio_socket.async_connect(endpoint, m_asio_strand.wrap(boost::bind(&Connection_raw<Socket_type>::on_accept, this, boost::asio::placeholders::error)));
}


template <typename Socket_type>
void Connection_raw<Socket_type>::async_accept(boost::asio::ip::tcp::acceptor &acceptor,
                                      const On_asio_status_callback &on_accept_callback,
                                      const On_asio_status_callback &on_ready_callback)
{
  m_on_accept_callback = on_accept_callback;
  m_on_ready_callback = on_ready_callback;
  acceptor.async_accept(m_asio_socket, m_asio_strand.wrap(boost::bind(&Connection_raw<Socket_type>::on_accept, this, boost::asio::placeholders::error)));
}


template <typename Socket_type>
void Connection_raw<Socket_type>::async_write(const Const_buffer_sequence &data, const On_asio_data_callback &on_write_callback)
{
  m_asio_socket.async_write_some(data, m_asio_strand.wrap(on_write_callback));
}


template <typename Socket_type>
void Connection_raw<Socket_type>::async_read(const Mutable_buffer_sequence &data, const On_asio_data_callback &on_read_callback)
{
  m_asio_socket.async_read_some(data, m_asio_strand.wrap(on_read_callback));
}


template <typename Socket_type>
void Connection_raw<Socket_type>::async_activate_tls(const On_asio_status_callback on_status)
{
  on_status(boost::system::errc::make_error_code(boost::system::errc::not_supported));
}


template <typename Socket_type>
void Connection_raw<Socket_type>::post(const boost::function<void ()> &calee)
{
  m_asio_strand.post(m_asio_strand.wrap(calee));
}


template <typename Socket_type>
bool Connection_raw<Socket_type>::thread_in_connection_strand()
{
  return m_asio_strand.running_in_this_thread();
}


template <typename Socket_type>
void Connection_raw<Socket_type>::shutdown(boost::asio::socket_base::shutdown_type how_to_shutdown, boost::system::error_code &ec)
{
  m_asio_socket.shutdown(how_to_shutdown, ec);
}


template <typename Socket_type>
void Connection_raw<Socket_type>::close()
{
  if (m_asio_socket.is_open())
  {
    boost::system::error_code ec;

    m_asio_socket.close(ec);
  }
}


template <typename Socket_type>
void Connection_raw<Socket_type>::on_accept(const boost::system::error_code &ec)
{
  Callback_post callback(boost::bind(&Connection_raw<Socket_type>::post, this, _1));

  callback.call_status_function(m_on_accept_callback, ec);

  if (!ec)
  {
    callback.call_status_function(m_on_ready_callback, ec);
  }

  m_on_ready_callback.clear();
}


}  // namespace ngs


#endif // _NGS_ASIO_CONNECTION_RAW_H_

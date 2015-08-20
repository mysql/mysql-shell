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


#ifndef _NGS_ASIO_CONNECTION_YASSL_H_
#define _NGS_ASIO_CONNECTION_YASSL_H_

#include "myasio/connection.h"
#include "myasio/connection_state_yassl.h"
#include "myasio/wrapper_ssl.h"

#include <list>
#include <boost/scoped_ptr.hpp>


namespace ngs
{


struct Buffer_page;
typedef Memory_new<Wrapper_ssl>::Unique_ptr Wrapper_ssl_ptr;


class Connection_yassl : public Connection
{
public:
  Connection_yassl(Connection_unique_ptr connection_raw, Wrapper_ssl_ptr ssl, Vector_states_yassl &handlers);
  virtual ~Connection_yassl();

  virtual Endpoint    get_remote_endpoint() const;
  virtual int         get_socket_id();
  virtual Options_session_ptr options();

  virtual void post(const boost::function<void ()> &calee);
  virtual bool thread_in_connection_strand();

  // Read, write SDU (service data unit)
  virtual void async_connect(const Endpoint &endpoint, const On_asio_status_callback &on_connect_callback, const On_asio_status_callback &on_ready_callback);
  virtual void async_accept(boost::asio::ip::tcp::acceptor &, const On_asio_status_callback &on_accept_callback, const On_asio_status_callback &on_ready_callback);
  virtual void async_write(const Const_buffer_sequence &data, const On_asio_data_callback &on_write_callback);
  virtual void async_read(const Mutable_buffer_sequence &data, const On_asio_data_callback &on_read_callback);
  virtual void async_activate_tls(const On_asio_status_callback on_status);

  virtual Connection_unique_ptr get_lowest_layer();

  virtual void shutdown(boost::asio::socket_base::shutdown_type how_to_shutdown, boost::system::error_code &ec);
  virtual void close();

private:
  void change_state(const State state, const bool first_state = false);

  void request_asio_read_into_pdu_ibuffer();

  void on_asio_accept(const boost::system::error_code &ec);
  void on_asio_read_into_pdu_ibuffer(const boost::system::error_code &error, std::size_t count);
  void on_asio_write_of_pdu_completed(const boost::system::error_code &error,
                                      std::size_t count,
                                      boost::shared_ptr<uint8_t[]> last_data = boost::shared_ptr<uint8_t[]>());

  static long ssl_pdu_socket_recv_from_ibuffer(void *ptr, void *buf, std::size_t count);
  static long ssl_pdu_socket_send_to_asio(void *ptr, const void *buf, std::size_t count);

  bool insert_sdu(const Const_buffer_sequence& data, const On_asio_data_callback& on_write_callback);
  void handle_sdu(); // from AppLayer
  void handle_pdu(); // from TcpLayer

  Mutable_buffer_sequence create_asio_read_buffer(void* buf_ptr, const std::size_t buf_size);
  Const_buffer_sequence   create_asio_write_buffer(const void* buf_ptr, const std::size_t buf_size);

  typedef std::list<const_buffer_with_callback> Const_buffer_list;

  Connection_unique_ptr    m_connection;
  Wrapper_ssl_ptr         m_ssl;
  Callback_ptr            m_callback;

  Buffer_page            *m_ssl_pdu_input_buffer;
  Const_buffer_list       m_ssl_sdu_output_buffer;
  Mutable_buffer_sequence m_ssl_sdu_input_buffer;

  On_asio_status_callback m_accept_callback;
  On_asio_status_callback m_ready_callback;
  On_asio_data_callback   m_on_sdu_recv_user_callback;
  std::size_t             m_number_of_onging_ssl_writes;
  bool                    m_asio_read_panding;

  State_yassl            *m_current_handler;
  Vector_states_yassl     m_handlers;
};


}  // namespace ngs


#endif // _NGS_ASIO_CONNECTION_YASSL_H_

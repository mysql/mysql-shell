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

#include "myasio/callback.h"
#include "ngs/memory.h"
#include "ngs/protocol/buffer.h"
#include "myasio/options.h"
#include "myasio/connection_raw.h"
#include "myasio/connection_yassl.h"

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/asio/error.hpp>
#include <stdint.h>
#include <iterator>
#include <algorithm>

#define LOG_DOMAIN "ngs.protocol"
#include "ngs/log.h"


#if defined(_WIN32)
#define SOCKET_EWOULDBLOCK WSAEWOULDBLOCK
#else /* Unix */
#define SOCKET_EWOULDBLOCK EWOULDBLOCK
#endif

namespace ngs
{

  // First parameter separates UT from asio, second from SSL
  Connection_yassl::Connection_yassl(IConnection_unique_ptr connection_raw, Wrapper_ssl_ptr ssl, Vector_states_yassl &handlers)
  : m_connection(boost::move(connection_raw)),
    m_ssl(boost::move(ssl)),
    m_callback(boost::make_shared<Callback_post>(boost::bind(&IConnection::post, m_connection.get(), _1))),
    m_ssl_pdu_input_buffer(new Page()),
    m_number_of_onging_ssl_writes(0),
    m_asio_read_panding(false),
    m_current_handler(NULL)
  {
    m_handlers.swap(handlers);

    change_state(State_handshake, true);
  }

  Connection_yassl::~Connection_yassl()
  {
    close();
    delete m_ssl_pdu_input_buffer;
  }

  Endpoint Connection_yassl::get_remote_endpoint() const
  {
    return m_connection->get_remote_endpoint();
  }

  int Connection_yassl::get_socket_id()
  {
    return m_connection->get_socket_id();
  }

  IOptions_session_ptr Connection_yassl::options()
  {
    return m_ssl->get_ssl_options();
  }

  IConnection_ptr Connection_yassl::get_lowest_layer()
  {
    return m_connection->get_lowest_layer();
  }

  void Connection_yassl::post(const boost::function<void()> &calee)
  {
    m_connection->post(calee);
  }

  bool Connection_yassl::thread_in_connection_strand()
  {
    return m_connection->thread_in_connection_strand();
  }

  void Connection_yassl::async_connect(const Endpoint &endpoint, const On_asio_status_callback &on_connect_callback, const On_asio_status_callback &on_ready_callback)
  {
    m_accept_callback = on_connect_callback;
    m_ready_callback = on_ready_callback;

    m_connection->async_connect(endpoint, boost::bind(&Connection_yassl::on_asio_accept, this, boost::asio::placeholders::error), On_asio_status_callback());
  }

  void Connection_yassl::async_accept(boost::asio::ip::tcp::acceptor & acceptor, const On_asio_status_callback &on_accept_callback, const On_asio_status_callback &on_ready_callback)
  {
    m_accept_callback = on_accept_callback;
    m_ready_callback = on_ready_callback;

    m_connection->async_accept(acceptor, boost::bind(&Connection_yassl::on_asio_accept, this, boost::asio::placeholders::error), On_asio_status_callback());
  }

  bool Connection_yassl::insert_sdu(const Const_buffer_sequence& data, const On_asio_data_callback& on_write_callback)
  {
    const_buffer_with_callback buffer_with_callback = { data, on_write_callback, 0 };

    m_ssl_sdu_output_buffer.push_back(buffer_with_callback);

    return  1 == m_ssl_sdu_output_buffer.size();
  }

  void Connection_yassl::async_write(const Const_buffer_sequence &data, const On_asio_data_callback &on_write_callback)
  {
    if (!m_connection->thread_in_connection_strand())
    {
      m_connection->post(boost::bind(&Connection_yassl::async_write, this, data, on_write_callback));
      return;
    }

    const bool is_first_sdu = insert_sdu(data, on_write_callback);

    if (is_first_sdu)
    {
      handle_sdu();
    }
  }

  void Connection_yassl::async_read(const Mutable_buffer_sequence &data, const On_asio_data_callback &on_read_callback)
  {
    if (!m_connection->thread_in_connection_strand())
    {
      m_connection->post(boost::bind(&Connection_yassl::async_read, this, data, on_read_callback));
      return;
    }

    if (m_on_sdu_recv_user_callback)
    {
      log_error("%i:User callback already installed", m_connection->get_socket_id());
      return;
    }

    if (data.empty())
    {
      boost::system::error_code no_error;
      size_t no_data = 0;
      m_connection->post(boost::bind(on_read_callback, no_error, no_data));
      return;
    }

    m_ssl_sdu_input_buffer.assign(data.begin(), data.end());
    m_on_sdu_recv_user_callback.clear();
    m_on_sdu_recv_user_callback = on_read_callback;

    m_connection->post(boost::bind(&Connection_yassl::handle_pdu, this));
  }

  void Connection_yassl::async_activate_tls(const On_asio_status_callback on_status)
  {
    ssl_initialize();

    m_ready_callback = on_status;
    on_asio_accept(boost::system::error_code());
  }

  void Connection_yassl::shutdown(boost::asio::socket_base::shutdown_type how_to_shutdown, boost::system::error_code &ec)
  {
    m_connection->shutdown(how_to_shutdown, ec);
  }

  void Connection_yassl::cancel()
  {
    m_connection->cancel();
  }

  void Connection_yassl::close()
  {
    m_connection->close();
  }

  void Connection_yassl::change_state(const State state, const bool first_state)
  {
    Vector_states_yassl::iterator i = m_handlers.begin();

    while (i != m_handlers.end())
    {
      if (state == (*i)->get_state_id())
      {
        m_current_handler = (*i).get();
        m_current_handler->set_callback(m_callback);

        if (!first_state)
        {
          log_debug("%i: Changed state to %i", m_connection->get_socket_id(), static_cast<int>(state));
        }

        return;
      }
      ++i;
    }

    log_error("%i: State %i not found", m_connection->get_socket_id(), static_cast<int>(state));
  }

  void Connection_yassl::on_asio_accept(const boost::system::error_code &ec)
  {
    log_debug("%i: long Connection_yassl::on_asio_accept(error = %i)", m_connection->get_socket_id(), (int)(bool)ec);

    ssl_initialize();

    if (!ec)
    {
      m_ssl->ssl_set_fd(m_connection->get_socket_id());
    }

    m_callback->call_status_function(m_accept_callback, ec);

    if (!ec)
    {
      handle_pdu();
    }
  }

  long Connection_yassl::ssl_pdu_socket_recv_from_ibuffer(void *object_ptr, void *buf, std::size_t count)
  {
    Connection_yassl &this_obj = *reinterpret_cast<Connection_yassl*>(object_ptr);

    if (0 == this_obj.m_ssl_pdu_input_buffer->length)
    {
      this_obj.m_ssl->ssl_set_error_want_read();
      this_obj.m_ssl->ssl_set_socket_error(SOCKET_EWOULDBLOCK);

      return -1;
    }

    const std::size_t bytes_to_read = std::min(static_cast<std::size_t>(this_obj.m_ssl_pdu_input_buffer->length), count);
    const std::size_t bytes_in_buffer = this_obj.m_ssl_pdu_input_buffer->length;
    char*             asio_recv_buffer_ptr = this_obj.m_ssl_pdu_input_buffer->data;

    memcpy(buf, asio_recv_buffer_ptr, bytes_to_read);

    if (bytes_to_read < bytes_in_buffer)
    {
      const std::size_t bytes_left_in_buffer = bytes_in_buffer - bytes_to_read;

      memmove(asio_recv_buffer_ptr,
              asio_recv_buffer_ptr + bytes_to_read,
              bytes_left_in_buffer);
    }

    this_obj.m_ssl_pdu_input_buffer->length -= bytes_to_read;

    return bytes_to_read;
  }

  long Connection_yassl::ssl_pdu_socket_send_to_asio(void *object_ptr, const void *buf, std::size_t count)
  {
    Connection_yassl &this_obj = *reinterpret_cast<Connection_yassl*>(object_ptr);
    boost::shared_ptr<uint8_t[]> data_copy = boost::make_shared<uint8_t[]>(count);

    memcpy(data_copy.get(), buf, count);

    ++this_obj.m_number_of_onging_ssl_writes;

    this_obj.m_connection->async_write(this_obj.create_asio_write_buffer(data_copy.get(), count),
                                       boost::bind(&Connection_yassl::on_asio_write_of_pdu_completed,
                                                   &this_obj,
                                                   boost::asio::placeholders::error,
                                                   boost::asio::placeholders::bytes_transferred,
                                                   data_copy));

    return count;
  }

  void Connection_yassl::handle_sdu()
  {
    State_yassl::Result result = State_yassl::Result_continue;
    bool should_remove_processed_item = false;

    while (State_yassl::Result_continue == result)
    {
      boost::optional<State> next_state;

      if (should_remove_processed_item)
      {
        m_ssl_sdu_output_buffer.pop_front();
      }

      if (m_ssl_sdu_output_buffer.empty())
      {
        return;
      }

      const_buffer_with_callback& next_buffer_callback = m_ssl_sdu_output_buffer.front();
      bool is_pdu_write_ongoing = m_number_of_onging_ssl_writes != 0;

      result = m_current_handler->handle_sdu(next_state, next_buffer_callback, is_pdu_write_ongoing);

      if (next_state)
      {
        change_state(*next_state);
      }

      should_remove_processed_item = true;
    }
  }

  void Connection_yassl::on_asio_write_of_pdu_completed(const boost::system::error_code &error, std::size_t count, boost::shared_ptr<uint8_t[]> last_data)
  {
    --m_number_of_onging_ssl_writes;

    if (m_ssl_sdu_output_buffer.empty())
    {
      return;
    }

    const_buffer_with_callback &buffer_callback = m_ssl_sdu_output_buffer.front();

    if (error)
    {
      m_callback->call_data_function(buffer_callback.on_sdu_send_user_callback, error);
      return;
    }

    if (buffer_callback.buffer.empty())
    {
      m_callback->call_data_function(buffer_callback.on_sdu_send_user_callback, buffer_callback.write_user_data_size);

      m_ssl_sdu_output_buffer.pop_front();

      if (m_ssl_sdu_output_buffer.empty())
      {
        return;
      }
    }

    handle_sdu();
  }

  void Connection_yassl::handle_pdu()
  {
    State_yassl::Result result = State_yassl::Result_continue;

    while (State_yassl::Result_continue == result)
    {
      boost::optional<State> next_state;

      if (0 != m_ssl_pdu_input_buffer->length || m_current_handler->can_process_empty_pdu())
        result = m_current_handler->handle_pdu(next_state, m_ssl_sdu_input_buffer, m_on_sdu_recv_user_callback, m_ready_callback);

      if (next_state)
      {
        change_state(*next_state);
      }

      if (0 == m_ssl_pdu_input_buffer->length && State_yassl::Result_continue == result)
      {
        request_asio_read_into_pdu_ibuffer();

        break;
      }
    }
  }

  void Connection_yassl::on_asio_read_into_pdu_ibuffer(const boost::system::error_code &error, std::size_t count)
  {
    m_asio_read_panding = false;
    m_ssl_pdu_input_buffer->length += count;

    if (error)
    {
      change_state(State_stop);

      if (m_accept_callback)
        m_callback->call_status_function(m_accept_callback, error);
      else if (m_ready_callback)
        m_callback->call_status_function(m_ready_callback, error);
      else
        m_callback->call_data_function(m_on_sdu_recv_user_callback, error);

      return;
    }

    handle_pdu();
  }

  void Connection_yassl::request_asio_read_into_pdu_ibuffer()
  {
    if (m_asio_read_panding)
    {
      return;
    }

    m_asio_read_panding = true;
    m_connection->async_read(create_asio_read_buffer(m_ssl_pdu_input_buffer->get_free_ptr(), m_ssl_pdu_input_buffer->get_free_bytes()),
                             boost::bind(&Connection_yassl::on_asio_read_into_pdu_ibuffer,
                                         this,
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred));
  }

  Mutable_buffer_sequence Connection_yassl::create_asio_read_buffer(void* buf_ptr, const std::size_t buf_size)
  {
    Mutable_buffer_sequence result;

    result.push_back(boost::asio::buffer(buf_ptr, buf_size));

    return result;
  }

  Const_buffer_sequence Connection_yassl::create_asio_write_buffer(const void* buf_ptr, const std::size_t buf_size)
  {
    Const_buffer_sequence result;

    result.push_back(boost::asio::buffer(buf_ptr, buf_size));

    return result;
  }

  void Connection_yassl::ssl_initialize()
  {
    m_ssl->ssl_initialize();

    m_ssl->ssl_set_transport_recv(&Connection_yassl::ssl_pdu_socket_recv_from_ibuffer);
    m_ssl->ssl_set_transport_send(&Connection_yassl::ssl_pdu_socket_send_to_asio);
    m_ssl->ssl_set_transport_data(this);
  }

} // namespace ngs

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


#include "myasio/connection_state_yassl.h"
#include "myasio/callback.h"


#define LOG_DOMAIN "ngs.protocol"
#include "ngs/log.h"


namespace ngs
{


State_yassl::State_yassl(IWrapper_ssl &ssl, const State state)
: m_state(state),
  m_ssl(ssl)
{

}


State_handshake_server_yassl::State_handshake_server_yassl(IWrapper_ssl &ssl)
: State_yassl(ssl, State_handshake)
{

}


State_yassl::Result State_handshake_server_yassl::handle_sdu(boost::optional<State> &next_state, const_buffer_with_callback& next_buffer_callback, bool is_ongoing_pdu_write)
{
  return Result_done;
}


State_yassl::Result State_handshake_server_yassl::handle_pdu(boost::optional<State> &next_state, Mutable_buffer_sequence &pdu_buffer,
                                                      On_asio_data_callback &read_user_callback, On_asio_status_callback &ready_callback)
{
  bool accept_result = m_ssl.ssl_handshake();

  if (accept_result)
  {
    next_state = State_running;
    m_callback->call_status_function(ready_callback, boost::system::error_code());
    return Result_done;
  }

  if (m_ssl.ssl_is_error_would_block() ||
      m_ssl.ssl_is_no_fatal_error())
  {
    m_ssl.ssl_set_error_none();
    return Result_continue;
  }

  next_state = State_stop;
  m_callback->call_status_function(ready_callback, m_ssl.get_boost_error());

  return Result_done;
}


State_handshake_client_yassl::State_handshake_client_yassl(IWrapper_ssl &ssl)
: State_handshake_server_yassl(ssl)
{
}


State_running_yassl::State_running_yassl(IWrapper_ssl &ssl)
: State_yassl(ssl, State_running)
{

}


State_yassl::Result State_running_yassl::handle_sdu(boost::optional<State> &next_state, const_buffer_with_callback& next_buffer_callback, bool is_ongoing_pdu_write)
{
  boost::asio::const_buffer   const_buffer = next_buffer_callback.buffer.at(0);

  const void*       data_ptr  = boost::asio::detail::buffer_cast_helper(const_buffer);
  const std::size_t data_size = boost::asio::detail::buffer_size_helper(const_buffer);

  next_buffer_callback.buffer.erase(next_buffer_callback.buffer.begin());

  if (data_size != (std::size_t) m_ssl.ssl_write(data_ptr, data_size))
  {
    m_callback->call_data_function(next_buffer_callback.on_sdu_send_user_callback, boost::system::errc::make_error_code(boost::system::errc::no_buffer_space));
    next_state = State_stop;

    return Result_continue;
  }

  next_buffer_callback.write_user_data_size += data_size;

  return Result_done;
}


State_yassl::Result State_running_yassl::handle_pdu(boost::optional<State> &next_state, Mutable_buffer_sequence &pdu_buffer,
                                                    On_asio_data_callback &read_user_callback, On_asio_status_callback &ready_callback)
{
  int num_of_bytes = 0;
  int page_offset = 0;
  Mutable_buffer_sequence::iterator page = pdu_buffer.begin();

  while (pdu_buffer.end() != page)
  {
    void* free_ptr = (char*)boost::asio::detail::buffer_cast_helper(*page) + page_offset;
    std::size_t buffer_size = boost::asio::detail::buffer_size_helper(*page);
    std::size_t free_size   = buffer_size - page_offset;

    int result = m_ssl.ssl_read(free_ptr, free_size);

    if (result > 0)
    {
      num_of_bytes += result;
      page_offset  += result;

      if (free_size - result == 0)
      {
        page_offset = 0;
        page++;
      }

      continue;
    }

    if (m_ssl.ssl_is_error_would_block())
    {
      if (m_ssl.ssl_is_error_would_block())
      {
        m_ssl.ssl_set_error_none();
      }

      break;
    }

    boost::system::error_code error = m_ssl.get_boost_error();

    if (0 == result && !error)
    {
      error = boost::system::errc::make_error_code(boost::system::errc::connection_reset);
    }

    next_state = State_stop;
    m_callback->call_data_function(read_user_callback, error);

    return Result_done;
  }

  if (0 < num_of_bytes)
  {
    m_callback->call_data_function(read_user_callback, num_of_bytes);
    return Result_done;
  }

  if (pdu_buffer.empty())
  {
    log_debug("Waiting for user request");
    return Result_done;
  }

  return Result_continue;
}


State_stop_yassl::State_stop_yassl(IWrapper_ssl &ssl)
: State_yassl(ssl, State_stop)
{

}



State_yassl::Result State_stop_yassl::handle_sdu(boost::optional<State> &next_state, const_buffer_with_callback& next_buffer_callback, bool is_ongoing_pdu_write)
{
  if (is_ongoing_pdu_write)
  {
    log_debug("Waiting for next notification");
    return Result_done;
  }

  m_callback->call_data_function(next_buffer_callback.on_sdu_send_user_callback, boost::system::errc::make_error_code(boost::system::errc::state_not_recoverable));

  return Result_continue;
}


State_yassl::Result State_stop_yassl::handle_pdu(boost::optional<State> &next_state, Mutable_buffer_sequence &pdu_buffer,
                                                 On_asio_data_callback &read_user_callback, On_asio_status_callback &ready_callback)
{
  m_callback->call_data_function(read_user_callback, boost::system::errc::make_error_code(boost::system::errc::state_not_recoverable));
  return Result_done;
}


} // namespace ngs

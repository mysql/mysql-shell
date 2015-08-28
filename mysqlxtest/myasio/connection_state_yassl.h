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


#ifndef _NGS_ASIO_CONNECTION_STATES_YASSL_H_
#define _NGS_ASIO_CONNECTION_STATES_YASSL_H_


#include "myasio/types.h"
#include "myasio/callback.h"
#include "myasio/wrapper_ssl.h"

#include <boost/function.hpp>
#include <boost/optional.hpp>

#include <vector>


namespace ngs
{


enum State {State_handshake, State_running, State_stop};


class State_yassl
{
public:
  enum Result {Result_done, Result_continue};

  State_yassl(Wrapper_ssl &ssl, const State state);
  virtual ~State_yassl() {}

  virtual Result handle_sdu(boost::optional<State> &next_state,
                            const_buffer_with_callback& next_buffer_callback,
                            bool is_ongoing_pdu_write) = 0;
  virtual Result handle_pdu(boost::optional<State> &next_state,
                            Mutable_buffer_sequence &pdu_buffer,
                            On_asio_data_callback &read_user_callback,
                            On_asio_status_callback &ready_callback) = 0;

  virtual bool can_process_empty_pdu() { return false; }
  virtual State get_state_id() { return m_state; }

  void set_callback(const Callback_ptr &callback) { m_callback = callback; }

protected:
  Callback_ptr  m_callback;
  const State   m_state;
  Wrapper_ssl  &m_ssl;
};


class State_handshake_server_yassl : public State_yassl
{
public:
  State_handshake_server_yassl(Wrapper_ssl &ssl);

  virtual Result handle_sdu(boost::optional<State> &next_state,
                            const_buffer_with_callback& next_buffer_callback,
                            bool is_ongoing_pdu_write);
  virtual Result handle_pdu(boost::optional<State> &next_state,
                            Mutable_buffer_sequence &pdu_buffer,
                            On_asio_data_callback &read_user_callback,
                            On_asio_status_callback &ready_callback);
};

class State_handshake_client_yassl : public State_handshake_server_yassl
{
public:
  State_handshake_client_yassl(Wrapper_ssl &ssl);

  virtual bool can_process_empty_pdu() { return true; }
};


class State_running_yassl : public State_yassl
{
public:
  State_running_yassl(Wrapper_ssl &ssl);

  virtual Result handle_sdu(boost::optional<State> &next_state,
                            const_buffer_with_callback& next_buffer_callback,
                            bool is_ongoing_pdu_write);
  virtual Result handle_pdu(boost::optional<State> &next_state,
                            Mutable_buffer_sequence &pdu_buffer,
                            On_asio_data_callback &read_user_callback,
                            On_asio_status_callback &ready_callback);

  virtual bool can_process_empty_pdu() { return true; }
};


class State_stop_yassl : public State_yassl
{
public:
  State_stop_yassl(Wrapper_ssl &ssl);

  virtual Result handle_sdu(boost::optional<State> &next_state,
                            const_buffer_with_callback& next_buffer_callback,
                            bool is_ongoing_pdu_write);
  virtual Result handle_pdu(boost::optional<State> &next_state,
                            Mutable_buffer_sequence &pdu_buffer,
                            On_asio_data_callback &read_user_callback,
                            On_asio_status_callback &ready_callback);
};


//little overkill but unique ptr dosen't work with std containers !
typedef boost::shared_ptr<State_yassl> State_yassl_ptr;
typedef std::vector<State_yassl_ptr>   Vector_states_yassl;


}  // namespace ngs


#endif // _NGS_ASIO_CONNECTION_STATES_YASSL_H_

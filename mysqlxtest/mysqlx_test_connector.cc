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

#include "mysqlx_test_connector.h"

#include <iostream>

#include <boost/asio.hpp>
#include <string>
#include <iostream>

namespace mysqlx
{
  Mysqlx_test_connector::Mysqlx_test_connector(const std::string &host, const std::string &port)
    : _socket(_ios)
  {
    tcp::resolver resolver(_ios);
    tcp::resolver::query query(host, port);

#if BOOST_VERSION >= 104700
    boost::asio::connect(_socket, resolver.resolve(query));
#else
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;
    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end)
    {
      _socket.close();
      _socket.connect(*endpoint_iterator++, error);
    }
    if (error)
      throw boost::system::system_error(error);
#endif
  }

  void Mysqlx_test_connector::send_message(int mid, Message *msg)
  {
    char buf[5];
    *(int32_t*)buf = htonl(msg->ByteSize() + 5);
    buf[4] = mid;
    _socket.write_some(boost::asio::buffer(buf, 5));
    std::string mbuf;
    msg->SerializeToString(&mbuf);
    _socket.write_some(boost::asio::buffer(mbuf.data(), mbuf.length()));
    flush();
  }

  void Mysqlx_test_connector::send_message(int mid, const std::string &mdata)
  {
    char buf[5];
    *(int32_t*)buf = htonl(mdata.size() + 5);
    buf[4] = mid;
    _socket.write_some(boost::asio::buffer(buf, 5));
    _socket.write_some(boost::asio::buffer(mdata.data(), mdata.length()));
    flush();
  }

  Message *Mysqlx_test_connector::read_response(int &mid)
  {
    char buf[5];
    Message* ret_val = NULL;
    boost::system::error_code error;

    std::size_t length = boost::asio::read(_socket, boost::asio::buffer(buf, 5), boost::asio::transfer_all(), error);

    if (!error)
    {
      int msglen = ntohl(*(int32_t*)buf) - 5;
      mid = buf[4];
      char *mbuf = new char[msglen];

      std::size_t length = boost::asio::read(_socket, boost::asio::buffer(mbuf, msglen), boost::asio::transfer_all(), error);

      if (!error)
      {
        switch (mid)
        {
          case Mysqlx::ServerMessages::OK:
            ret_val = new Mysqlx::Ok();
            break;
          case Mysqlx::ServerMessages::ERROR:
            ret_val = new Mysqlx::Error();
            break;
          case Mysqlx::ServerMessages::NOTICE:
            ret_val = new Mysqlx::Notice();
            break;
          case Mysqlx::ServerMessages::PARAMETER_CHANGED_NOTIFICATION:
            ret_val = new Mysqlx::ParameterChangedNotification();
            break;
          case Mysqlx::ServerMessages::CONN_CAPABILITIES:
            ret_val = new Mysqlx::Connection::Capabilities();
            break;
          case Mysqlx::ServerMessages::SESS_AUTHENTICATE_CONTINUE:
            ret_val = new Mysqlx::Session::AuthenticateContinue();
            break;
          case Mysqlx::ServerMessages::SESS_AUTHENTICATE_OK:
            ret_val = new Mysqlx::Session::AuthenticateOk();
            break;
          case Mysqlx::ServerMessages::SQL_PREPARE_STMT_OK:
            ret_val = new Mysqlx::Sql::PrepareStmtOk();
            break;
          case Mysqlx::ServerMessages::SQL_PREPARED_STMT_EXECUTE_OK:
            ret_val = new Mysqlx::Sql::PreparedStmtExecuteOk();
            break;
          case Mysqlx::ServerMessages::SQL_COLUMN_META_DATA:
            ret_val = new Mysqlx::Sql::ColumnMetaData();
            break;
          case Mysqlx::ServerMessages::SQL_ROW:
            ret_val = new Mysqlx::Sql::Row();
            break;
          case Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE:
            ret_val = new Mysqlx::Sql::CursorFetchDone();
            break;
          case Mysqlx::ServerMessages::SQL_CURSOR_FETCH_SUSPENDED:
            ret_val = new Mysqlx::Sql::CursorFetchSuspended();
            break;
          case Mysqlx::ServerMessages::SQL_CURSORS_POLL:
            ret_val = new Mysqlx::Sql::CursorsPoll();
            break;
          case Mysqlx::ServerMessages::SQL_CURSOR_CLOSE_OK:
            ret_val = new Mysqlx::Sql::CursorCloseOk();
            break;
          case Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE_MORE_RESULTSETS:
            ret_val = new Mysqlx::Sql::CursorFetchDoneMoreResultsets();
            break;
          case  Mysqlx::ServerMessages::SESS_AUTHENTICATE_FAIL:
            ret_val = new Mysqlx::Session::AuthenticateFail();
            break;
          case Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK:
            ret_val = new Mysqlx::Sql::StmtExecuteOk();
            break;
          default:
            delete[] mbuf;

            std::stringstream ss;
            ss << "Unknown message received from server: ";
            ss << mid;
            throw std::logic_error(ss.str());
            break;
        }

        // Parses the received message
        ret_val->ParseFromString(std::string(mbuf, msglen));

        if (!ret_val->IsInitialized())
        {
          std::string error("Message is not properly initialized : ");
          error += ret_val->InitializationErrorString();
          throw std::runtime_error(error);
        }
      }

      delete[] mbuf;
    }

    // Throws an exception in case of error
    if (error)
      throw std::runtime_error(error.message());

    return ret_val;
  }
  /*
   * send_receive_message
   *
   * Handles a happy path of communication with the server:
   * - Sends a message
   * - Receives response and validates it
   *
   * If any of the expected responses is not received then an exception is trown
   */
  Message *Mysqlx_test_connector::send_receive_message(int& mid, Message *msg, std::set<int> responses, const std::string& info)
  {
    send_message(mid, msg);

    Message *response = read_response(mid);

    if (responses.find(mid) == responses.end())
      handle_wrong_response(mid, response, info);

    return response;
  }

  /*
   * send_receive_message
   *
   * Handles a happy path of communication with the server:
   * - Sends a message
   * - Receives response and validates it
   *
   * If the expected response is not received then an exception is trown
   */
  Message *Mysqlx_test_connector::send_receive_message(int& mid, Message *msg, int response_id, const std::string& info)
  {
    send_message(mid, msg);

    Message *response = read_response(mid);

    if (mid != response_id)
      handle_wrong_response(mid, response, info);

    return response;
  }

  /*
   * handle_wrong_response
   * Throws the proper exception based on the received response on the
   * send_receive_message methods above
   */
  void Mysqlx_test_connector::handle_wrong_response(int mid, Message *msg, const std::string& info)
  {
    if (mid == Mysqlx::ServerMessages::ERROR)
    {
      Mysqlx::Error *error = dynamic_cast<Mysqlx::Error *>(msg);
      std::string error_message(error->msg());
      delete error;
      throw std::runtime_error(error_message);
    }
    else
    {
      std::string message;
      if (msg)
        delete msg;

      std::stringstream ss;
      ss << "Unexpected response [" << mid << "], " << info;

      throw std::logic_error(ss.str());
    }
  }

  void Mysqlx_test_connector::flush()
  {
    //_socket.flush();
  }

  void Mysqlx_test_connector::close()
  {
    // This should be logged, for now commenting to
    // avoid having unneeded output on the script mode
    // shcore::print("disconnect\n");
    _socket.close();
    /*Mysqlx::Connection::close close;
    std::string data;
    close.SerializeToString(&data);
    send_message(Mysqlx::ClientMessages::kMsgConClose, data);*/
  }

  Mysqlx_test_connector::~Mysqlx_test_connector()
  {
    close();
  }
}
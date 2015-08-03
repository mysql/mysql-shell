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

// Avoid warnings from includes of other project and protobuf
#if defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4018 4996)
#endif

#include <google/protobuf/text_format.h>
#include <google/protobuf/message.h>
#include <boost/bind.hpp>
#include "mysqlx.h"
#include "mysqlx_connection.h"
#include "mysqlx_crud.h"
#include "mysqlx_row.h"

#include "my_config.h"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#elif defined _MSC_VER
#pragma warning (pop)
#endif

#include <iostream>

#include <boost/asio.hpp>
#include <string>
#include <iostream>
#include <limits>
#include "compilerutils.h"
#include "xdecimal.h"

#ifdef WIN32
#  define snprintf _snprintf
#  pragma push_macro("ERROR")
#  undef ERROR
#endif

using namespace mysqlx;

bool mysqlx::parse_mysql_connstring(const std::string &connstring,
                                    std::string &protocol, std::string &user, std::string &password,
                                    std::string &host, int &port, std::string &sock,
                                    std::string &db, int &pwd_found)
{
  // format is [protocol://][user[:pass]]@host[:port][/db] or user[:pass]@::socket[/db], like what cmdline utilities use
  pwd_found = 0;
  std::string remaining = connstring;

  std::string::size_type p;
  p = remaining.find("://");
  if (p != std::string::npos)
  {
    protocol = connstring.substr(0, p);
    remaining = remaining.substr(p + 3);
  }

  std::string s = remaining;
  p = remaining.find('/');
  if (p != std::string::npos)
  {
    db = remaining.substr(p + 1);
    s = remaining.substr(0, p);
  }
  p = s.rfind('@');
  std::string user_part;
  std::string server_part = (p == std::string::npos) ? s : s.substr(p + 1);

  if (p == std::string::npos)
  {
    // by default, connect using the current OS username
#ifdef _WIN32
    //XXX find out current username here
#else
    const char *tmp = getenv("USER");
    user_part = tmp ? tmp : "";
#endif
  }
  else
    user_part = s.substr(0, p);

  if ((p = user_part.find(':')) != std::string::npos)
  {
    user = user_part.substr(0, p);
    password = user_part.substr(p + 1);
    pwd_found = 1;
  }
  else
    user = user_part;

  p = server_part.find(':');
  if (p != std::string::npos)
  {
    host = server_part.substr(0, p);
    server_part = server_part.substr(p + 1);
    p = server_part.find(':');
    if (p != std::string::npos)
      sock = server_part.substr(p + 1);
    else
    if (!sscanf(server_part.substr(0, p).c_str(), "%i", &port))
      return false;
  }
  else
    host = server_part;
  return true;
}

using namespace mysqlx;

Error::Error(int err, const std::string &message)
: std::runtime_error(message), _message(message), _error(err)
{
}

Error::~Error() BOOST_NOEXCEPT_OR_NOTHROW
{
}


static void throw_server_error(const Mysqlx::Error &error)
{
  throw Error(error.code(), error.msg());
}

Session::Session()
{
  m_connection = new Connection();
}

Session::~Session()
{
  delete m_connection;
}

boost::shared_ptr<Session> mysqlx::openSession(const std::string &uri, const std::string &pass)
{
  boost::shared_ptr<Session> session(new Session());
  session->connection()->connect(uri, pass);
  return session;
}

boost::shared_ptr<Session> mysqlx::openSession(const std::string &host, int port, const std::string &schema,
                                               const std::string &user, const std::string &pass)
{
  boost::shared_ptr<Session> session(new Session());
  session->connection()->connect(host, port, schema, user, pass);
  return session;
}

Connection::Connection()
: m_socket(m_ios), m_deadline(m_ios), m_trace_packets(false), m_closed(true)
{
  if (getenv("MYSQLX_TRACE_CONNECTION"))
    m_trace_packets = true;
}

Connection::~Connection()
{
  try
  {
    close();
  }
  catch (Error &e)
  {
    // ignore close errors
  }
}

void Connection::connect(const std::string &uri, const std::string &pass)
{
  std::string protocol, host, schema, user, password;
  std::string sock;
  int pwd_found = 0;
  int port = 33060;

  if (!parse_mysql_connstring(uri, protocol, user, password, host, port, sock, schema, pwd_found))
    throw Error(CR_WRONG_HOST_INFO, "Unable to parse connection string");

  if (protocol != "mysqlx" && !protocol.empty())
    throw Error(CR_WRONG_HOST_INFO, "Unsupported protocol " + protocol);

  if (!pass.empty())
    password = pass;

  connect(host, port, schema, user, pass.empty() ? password : pass);
}

void Connection::connect(const std::string &host, int port)
{
  tcp::resolver resolver(m_ios);
  char ports[8];
  snprintf(ports, sizeof(ports), "%i", port);
  tcp::resolver::query query(host, ports);

  boost::system::error_code error;
  tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, error);
  tcp::resolver::iterator end;

  if (error)
    throw Error(CR_UNKNOWN_HOST, error.message());

  error = boost::asio::error::fault;

  while (error && endpoint_iterator != end)
  {
    m_socket.close();
    m_socket.connect(*endpoint_iterator++, error);
  }

  if (error)
    throw Error(CR_CONNECTION_ERROR, error.message() + " connecting to " + host + ":" + ports);

  m_closed = false;
}

void Connection::connect(const std::string &host, int port, const std::string &schema,
                         const std::string &user, const std::string &pass)
{
  connect(host, port);

  authenticate_plain(user, pass, schema);
  //authenticate_mysql41(user, pass, schema);
}

void Connection::set_closed()
{
  m_closed = true;
}

void Connection::close()
{
  if (!m_closed)
  {
    send(Mysqlx::Session::Close());
    m_closed = true;

    int mid;
    try
    {
      std::auto_ptr<Message> message(recv_raw(mid));
      if (mid != Mysqlx::ServerMessages::OK)
        throw Error(CR_COMMANDS_OUT_OF_SYNC, "Unexpected message received in response to Session.Close");
      m_socket.close();
    }
    catch (...)
    {
      m_socket.close();
      throw;
    }
  }
}

Result *Connection::recv_result()
{
  return new Result(this, true);
}

Result *Connection::execute_sql(const std::string &sql)
{
  {
    Mysqlx::Sql::StmtExecute exec;
    exec.set_namespace_("sql");
    exec.set_stmt(sql);
    send(exec);
  }
  return new Result(this, true);
}

Result *Connection::execute_stmt(const std::string &ns, const std::string &sql, const std::vector<ArgumentValue> &args)
{
  {
    Mysqlx::Sql::StmtExecute exec;
    exec.set_namespace_(ns);
    exec.set_stmt(sql);

    for (std::vector<ArgumentValue>::const_iterator iter = args.begin();
         iter != args.end(); ++iter)
    {
      Mysqlx::Datatypes::Any *any = exec.mutable_args()->Add();

      any->set_type(Mysqlx::Datatypes::Any::SCALAR);
      switch (iter->type())
      {
        case ArgumentValue::TInteger:
          any->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_SINT);
          any->mutable_scalar()->set_v_signed_int(*iter);
          break;
        case ArgumentValue::TUInteger:
          any->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_UINT);
          any->mutable_scalar()->set_v_unsigned_int(*iter);
          break;
        case ArgumentValue::TNull:
          any->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_NULL);
          break;
        case ArgumentValue::TDouble:
          any->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_DOUBLE);
          any->mutable_scalar()->set_v_double(*iter);
          break;
        case ArgumentValue::TFloat:
          any->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_FLOAT);
          any->mutable_scalar()->set_v_float(*iter);
          break;
        case ArgumentValue::TBool:
          any->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_BOOL);
          any->mutable_scalar()->set_v_bool(*iter);
          break;
        case ArgumentValue::TString:
          any->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_STRING);
          any->mutable_scalar()->mutable_v_string()->set_value(*iter);
          break;
        case ArgumentValue::TOctets:
          any->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_OCTETS);
          any->mutable_scalar()->set_v_opaque(*iter);
          break;
      }
    }
    send(exec);
  }
  return new Result(this, true);
}

Result *Connection::execute_find(const Mysqlx::Crud::Find &m)
{
  send(m);

  return new Result(this, true);
}

Result *Connection::execute_update(const Mysqlx::Crud::Update &m)
{
  send(m);

  return new Result(this, false);
}

Result *Connection::execute_insert(const Mysqlx::Crud::Insert &m)
{
  send(m);

  return new Result(this, false);
}

Result *Connection::execute_delete(const Mysqlx::Crud::Delete &m)
{
  send(m);

  return new Result(this, false);
}

void Connection::authenticate_mysql41(const std::string &user, const std::string &pass, const std::string &db)
{
  /*  {
      Mysqlx::Session::AuthenticateStart auth;

      auth.set_mech_name("MYSQL41");

      send(Mysqlx::ClientMessages::SESS_AUTHENTICATE_START, auth);
      }

      {
      int mid;
      std::auto_ptr<Message> message(recv_raw(mid));
      switch (mid)
      {
      case Mysqlx::ServerMessages::SESS_AUTHENTICATE_CONTINUE:
      {
      Mysqlx::Session::AuthenticateContinue &auth_continue = *static_cast<Mysqlx::Session::AuthenticateContinue*>(message.get());

      std::string data;

      if (!auth_continue.has_auth_data())
          throw Error(CR_MALFORMED_PACKET, "Missing authentication data");

      std::string password_hash;

      if (pass.length())
      {
      password_hash = ngs::Password_hasher::scramble(auth_continue.auth_data().c_str(), pass.c_str());
      password_hash = ngs::Password_hasher::get_password_from_salt(password_hash);
      }

      data.append(db).push_back('\0'); // authz
      data.append(user).push_back('\0'); // authc
      data.append(password_hash); // pass

      Mysqlx::Session::AuthenticateContinue auth_continue_responce;

      auth_continue_responce.set_auth_data(data);

      send(Mysqlx::ClientMessages::SESS_AUTHENTICATE_CONTINUE, auth_continue_responce);
      }
      break;

      case Mysqlx::ServerMessages::NOTICE:
        dispatch_notice(static_cast<Mysqlx::Notice::Frame*>(message.get()));
        break;

      case Mysqlx::ServerMessages::ERROR:
      throw_server_error(*static_cast<Mysqlx::Error*>(message.get()));

      default:
      throw Error(CR_MALFORMED_PACKET, "Unexpected message received from server");
      break;
      }
      }

  bool done = false;
  while (!done)
      {
      int mid;
      std::auto_ptr<Message> message(recv_raw(mid));
      switch (mid)
      {
      case Mysqlx::ServerMessages::SESS_AUTHENTICATE_OK:
        done = true;
      break;

      case Mysqlx::ServerMessages::ERROR:
      throw_server_error(*static_cast<Mysqlx::Error*>(message.get()));

      case Mysqlx::ServerMessages::NOTICE:
        dispatch_notice(static_cast<Mysqlx::Notice::Frame*>(message.get()));
        break;

      default:
      throw Error(CR_MALFORMED_PACKET, "Unexpected message received from server");
      break;
      }
      }*/
}

void Connection::authenticate_plain(const std::string &user, const std::string &pass, const std::string &UNUSED(db))
{
  {
    Mysqlx::Session::AuthenticateStart auth;

    auth.set_mech_name("PLAIN");
    std::string data;

    data.push_back('\0'); // authz
    data.append(user).push_back('\0'); // authc
    data.append(pass); // pass

    //TODO pass db

    auth.set_auth_data(data);
    send(Mysqlx::ClientMessages::SESS_AUTHENTICATE_START, auth);
  }

  {
  int mid;
  std::auto_ptr<Message> message(recv_raw(mid));
  switch (mid)
  {
    case Mysqlx::ServerMessages::SESS_AUTHENTICATE_OK:
      break;

    case Mysqlx::ServerMessages::ERROR:
      throw_server_error(*static_cast<Mysqlx::Error*>(message.get()));

      case Mysqlx::ServerMessages::NOTICE:
        dispatch_notice(static_cast<Mysqlx::Notice::Frame*>(message.get()));
        break;

    default:
      throw Error(CR_MALFORMED_PACKET, "Unexpected message received from server");
      break;
  }
}
}

void Connection::send(int mid, const Message &msg)
{
  boost::system::error_code error;
  uint8_t buf[5];
  *(uint32_t*)buf = msg.ByteSize() + 1;
#ifdef WORDS_BIGENDIAN
  std::swap(buf[0], buf[3]);
  std::swap(buf[1], buf[2]);
#endif
  buf[4] = mid;

  if (m_trace_packets)
  {
    std::string out;
    google::protobuf::TextFormat::Printer p;
    p.SetInitialIndentLevel(1);
    p.PrintToString(msg, &out);
    std::cout << ">>>> SEND " << msg.ByteSize() + 1 << " " << msg.GetDescriptor()->full_name() << "\n" << out << "\n";
  }

  m_socket.write_some(boost::asio::buffer(buf, 5), error);
  if (!error)
  {
    std::string mbuf;
    msg.SerializeToString(&mbuf);
    m_socket.write_some(boost::asio::buffer(mbuf.data(), mbuf.length()), error);
  }

  if (error)
  {
    switch (error.value())
    {
      case boost::asio::error::connection_reset:
      case boost::asio::error::connection_aborted:
      case boost::asio::error::broken_pipe:
        throw Error(CR_SERVER_GONE_ERROR, error.message());

      default:
        throw Error(CR_UNKNOWN_ERROR, error.message());
    }
  }
}


void Connection::push_local_notice_handler(Local_notice_handler handler)
{
  m_local_notice_handlers.push_back(handler);
}


void Connection::pop_local_notice_handler()
  {
  m_local_notice_handlers.pop_back();
}


void Connection::dispatch_notice(Mysqlx::Notice::Frame *frame)
    {
      if (frame->scope() == Mysqlx::Notice::Frame::LOCAL)
      {
    if (!m_local_notice_handlers.empty())
    {
      for (std::list<Local_notice_handler>::iterator iter = m_local_notice_handlers.begin();
           iter != m_local_notice_handlers.end(); ++iter)
        if ((*iter)(frame->type(), frame->payload())) // handler returns true if the notice was handled
          break;
    }
        else
    {
      if (frame->type() == 3)
      {
        Mysqlx::Notice::SessionStateChanged change;
        change.ParseFromString(frame->payload());
        if (!change.IsInitialized())
          std::cerr << "Invalid notice received from server " << change.InitializationErrorString() << "\n";
        else
        {
          if (change.param() == Mysqlx::Notice::SessionStateChanged::ACCOUNT_EXPIRED)
          {
            std::cout << "NOTICE: Account password expired\n";
            return;
          }
        }
      }
          std::cout << "Unhandled local notice\n";
      }
  }
      else
      {
        std::cout << "Unhandled global notice\n";
      }
    }


Message *Connection::recv_next(int &mid)
{
  for (;;)
  {
    Message *msg = recv_raw(mid);
    if (mid == Mysqlx::ServerMessages::NOTICE)
    {
      dispatch_notice(static_cast<Mysqlx::Notice::Frame*>(msg));
    }
    else
      return msg;
  }
}

void Connection::handle_async_deadline_timeout(const boost::system::error_code &ec, bool &finished)
{
  finished = true;

  if (boost::asio::error::operation_aborted == ec)
    return;

  boost::system::error_code error;

  m_socket.cancel(error);
}

void Connection::handle_async_read(const boost::system::error_code &ec, std::size_t data_size, std::size_t &received_data)
{
  received_data = std::numeric_limits<std::size_t>::max();

  if (boost::asio::error::operation_aborted != ec)
  {
    m_deadline.cancel();

    if (ec)
      throw ec;
  }

  received_data = data_size;
}

Message *Connection::recv_raw_with_deadline(int &mid, const std::size_t deadline_miliseconds)
{
  bool deadline_occured = false;
  char header_buffer[5];
  std::size_t header_received = 0;
  boost::system::error_code remember_error;

  m_deadline.expires_from_now(boost::posix_time::milliseconds(deadline_miliseconds));

  m_socket.async_read_some(boost::asio::buffer(header_buffer, sizeof(header_buffer)), boost::bind(&Connection::handle_async_read, this, _1, _2, boost::ref(header_received)));
  m_deadline.async_wait(boost::bind(&Connection::handle_async_deadline_timeout, this, _1, boost::ref(deadline_occured)));

  while (!deadline_occured && 0 == header_received)
  {
    try
    {
      m_ios.run_one();
    }
    catch (const boost::system::error_code &ec)
    {
      if (ec)
        remember_error = ec;
    }
  }

  throw_mysqlx_error(remember_error);

  if (std::numeric_limits<std::size_t>::max() != header_received && header_received > 0)
    return recv_message_with_header(mid, header_buffer, header_received);

  return NULL;
}

Message *Connection::recv_payload(const int mid, const std::size_t msglen)
{
  boost::system::error_code error;
  Message* ret_val = NULL;
  char *mbuf = new char[msglen];

  boost::asio::read(m_socket, boost::asio::buffer(mbuf, msglen), boost::asio::transfer_all(), error);

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
        ret_val = new Mysqlx::Notice::Frame();
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
      case Mysqlx::ServerMessages::RESULTSET_COLUMN_META_DATA:
        ret_val = new Mysqlx::Resultset::ColumnMetaData();
        break;
      case Mysqlx::ServerMessages::RESULTSET_ROW:
        ret_val = new Mysqlx::Resultset::Row();
        break;
      case Mysqlx::ServerMessages::RESULTSET_FETCH_DONE:
        ret_val = new Mysqlx::Resultset::FetchDone();
        break;
      case Mysqlx::ServerMessages::RESULTSET_FETCH_DONE_MORE_RESULTSETS:
        ret_val = new Mysqlx::Resultset::FetchDoneMoreResultsets();
        break;
      case Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK:
        ret_val = new Mysqlx::Sql::StmtExecuteOk();
        break;
    }

    if (!ret_val)
    {
      delete[] mbuf;
      std::stringstream ss;
      ss << "Unknown message received from server ";
      ss << mid;
      throw Error(CR_MALFORMED_PACKET, ss.str());
    }

    // Parses the received message
    ret_val->ParseFromString(std::string(mbuf, msglen));

    if (m_trace_packets)
    {
      std::string out;
      google::protobuf::TextFormat::Printer p;
      p.SetInitialIndentLevel(1);
      p.PrintToString(*ret_val, &out);
      std::cout << "<<<< RECEIVE " << ret_val->GetDescriptor()->full_name() << "\n" << out << "\n";
    }

    if (!ret_val->IsInitialized())
    {
      delete[] mbuf;
      delete ret_val;
      std::string err("Message is not properly initialized: ");
      err += ret_val->InitializationErrorString();
      throw Error(CR_MALFORMED_PACKET, err);
    }
  }

  delete[] mbuf;

  return ret_val;
}

Message *Connection::recv_raw(int &mid)
{
  char buf[5];

  return recv_message_with_header(mid, buf, 0);
}

Message *Connection::recv_message_with_header(int &mid, char(&header_buffer)[5], const std::size_t header_offset)
{
  Message* ret_val = NULL;
  boost::system::error_code error;

  boost::asio::read(m_socket, boost::asio::buffer(header_buffer + header_offset,
                                                  5 - header_offset),
                    boost::asio::transfer_all(), error);

#ifdef WORDS_BIGENDIAN
  std::swap(header_buffer[0], header_buffer[3]);
  std::swap(header_buffer[1], header_buffer[2]);
#endif

  if (!error)
  {
    uint32_t msglen = *(uint32_t*)header_buffer - 1;
    mid = header_buffer[4];

    ret_val = recv_payload(mid, msglen);
  }
  else
  {
    throw_mysqlx_error(error);
  }

  return ret_val;
}

void Connection::throw_mysqlx_error(const boost::system::error_code &error)
{
  if (!error)
    return;

  switch (error.value())
  {
    case boost::asio::error::eof:
    case boost::asio::error::connection_reset:
    case boost::asio::error::connection_aborted:
    case boost::asio::error::broken_pipe:
      throw Error(CR_SERVER_GONE_ERROR, error.message());

    default:
      throw Error(CR_UNKNOWN_ERROR, error.message());
  }
}

boost::shared_ptr<Schema> Session::getSchema(const std::string &name)
{
  std::map<std::string, boost::shared_ptr<Schema> >::const_iterator iter = m_schemas.find(name);
  if (iter != m_schemas.end())
    return iter->second;

  return m_schemas[name] = boost::shared_ptr<Schema>(new Schema(shared_from_this(), name));
}

Result *Session::executeSql(const std::string &sql)
{
  return m_connection->execute_sql(sql);
}

Result *Session::executeStmt(const std::string &ns, const std::string &stmt,
                             const std::vector<ArgumentValue> &args)
{
  return m_connection->execute_stmt(ns, stmt, args);
}

Document::Document(const std::string &doc)
: m_data(new std::string(doc))
{
}

Document::Document(const Document &doc)
: m_data(doc.m_data)
{
}

Result::Result(Connection *owner, bool expect_data)
: current_message(NULL), m_owner(owner), m_last_insert_id(-1), m_affected_rows(-1),
  m_state(expect_data ? ReadMetadataI : ReadStmtOkI)
{
}

Result::Result()
: current_message(NULL)
{
}

Result::~Result()
{
  // flush the resultset from the pipe
  while (m_state != ReadError && m_state != ReadDone)
    nextDataSet();

  delete current_message;
}

boost::shared_ptr<std::vector<ColumnMetadata> > Result::columnMetadata()
{
  if (m_state == ReadMetadataI)
    read_metadata();
  return m_columns;
}

bool Result::ready()
{
  // if we've read something (ie not on initial state), then we're ready
  return m_state != ReadMetadataI && m_state != ReadStmtOkI;
}

void Result::wait()
{
  if (m_state == ReadMetadataI)
    read_metadata();
  if (m_state == ReadStmtOkI)
    read_stmt_ok();
}


bool Result::handle_notice(int32_t type, const std::string &data)
{
  switch (type)
{
    case 1: // warning
  {
    Mysqlx::Notice::Warning warning;
    warning.ParseFromString(data);
    if (!warning.IsInitialized())
      std::cerr << "Invalid notice received from server " << warning.InitializationErrorString() << "\n";
    else
    {
      Warning w;
      w.code = warning.code();
      w.text = warning.msg();
      w.is_note = warning.level() == Mysqlx::Notice::Warning::NOTE;
      m_warnings.push_back(w);
    }
      return true;
  }

    case 2: // session variable changed
      break;

    case 3: //session state changed
    {
      Mysqlx::Notice::SessionStateChanged change;
      change.ParseFromString(data);
      if (!change.IsInitialized())
        std::cerr << "Invalid notice received from server " << change.InitializationErrorString() << "\n";
      else
      {
        switch (change.param())
        {
          case Mysqlx::Notice::SessionStateChanged::GENERATED_INSERT_ID:
            if (change.value().type() == Mysqlx::Datatypes::Scalar::V_UINT)
              m_last_insert_id = change.value().v_unsigned_int();
            else
              std::cerr << "Invalid notice value received from server: " << data << "\n";
            break;

          case Mysqlx::Notice::SessionStateChanged::ROWS_AFFECTED:
            if (change.value().type() == Mysqlx::Datatypes::Scalar::V_UINT)
              m_affected_rows = change.value().v_unsigned_int();
  else
              std::cerr << "Invalid notice value received from server: " << data << "\n";
            break;

          case Mysqlx::Notice::SessionStateChanged::PRODUCED_MESSAGE:
            if (change.value().type() == Mysqlx::Datatypes::Scalar::V_STRING)
              m_info_message = change.value().v_string().value();
            else
              std::cerr << "Invalid notice value received from server: " << data << "\n";
            break;

          default:
            return false;
        }
      }
      return true;
    }
    default:
    std::cerr << "Unexpected notice type received " << type << "\n";
      return false;
}
  return false;
}


int Result::get_message_id()
{
  if (NULL != current_message)
  {
    return current_message_id;
  }

  m_owner->push_local_notice_handler(boost::bind(&Result::handle_notice, this, _1, _2));

  try
  {
  current_message = m_owner->recv_next(current_message_id);
  }
  catch (...)
  {
    m_owner->pop_local_notice_handler();
    throw;
  }

  m_owner->pop_local_notice_handler();

  // error messages that can be received in any state
  if (current_message_id == Mysqlx::ServerMessages::ERROR)
  {
    m_state = ReadError;
    throw_server_error(static_cast<const Mysqlx::Error&>(*current_message));
  }

  switch (m_state)
  {
    case ReadMetadataI:
    {
                        switch (current_message_id)
                        {
                          case Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK:
                            m_state = ReadDone;
                            return current_message_id;

                          case Mysqlx::ServerMessages::RESULTSET_COLUMN_META_DATA:
                            m_state = ReadMetadata;
                            return current_message_id;
                        }
                        break;
    }
    case ReadMetadata:
    {
                       // while reading metadata, we can either get more metadata
                       // start getting rows (which also signals end of metadata)
                       // or EORows, which signals end of metadata AND empty resultset
                       switch (current_message_id)
                       {
                         case Mysqlx::ServerMessages::RESULTSET_COLUMN_META_DATA:
                           m_state = ReadMetadata;
                           return current_message_id;

                         case Mysqlx::ServerMessages::RESULTSET_ROW:
                           m_state = ReadRows;
                           return current_message_id;

                         case Mysqlx::ServerMessages::RESULTSET_FETCH_DONE:
                           // empty resultset
                           m_state = ReadStmtOk;
                           return current_message_id;
                       }
                       break;
    }
    case ReadRows:
    {
                   switch (current_message_id)
                   {
                     case Mysqlx::ServerMessages::RESULTSET_ROW:
                       return current_message_id;

                     case Mysqlx::ServerMessages::RESULTSET_FETCH_DONE:
                       m_state = ReadStmtOk;
                       return current_message_id;

                     case Mysqlx::ServerMessages::RESULTSET_FETCH_DONE_MORE_RESULTSETS:
                       m_state = ReadMetadata;
                       return current_message_id;
                   }
                   break;
    }
    case ReadStmtOkI:
    case ReadStmtOk:
    {
                     switch (current_message_id)
                     {
                       case Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK:
                         m_state = ReadDone;
                         return current_message_id;
                     }
                     break;
    }
    case ReadError:
    case ReadDone:
      // not supposed to reach here
      throw std::logic_error("attempt to read data at wrong time");
  }

  m_state = ReadError;
  throw Error(CR_COMMANDS_OUT_OF_SYNC, "Unexpected message received from server");
}

mysqlx::Message* Result::pop_message()
{
  mysqlx::Message *result = current_message;

  current_message = NULL;

  return result;
}

static ColumnMetadata unwrap_column_metadata(const Mysqlx::Resultset::ColumnMetaData &column_data)
{
  ColumnMetadata column;

  switch (column_data.type())
  {
    case Mysqlx::Resultset::ColumnMetaData::SINT:
      column.type = mysqlx::SINT;
      break;
    case Mysqlx::Resultset::ColumnMetaData::UINT:
      column.type = mysqlx::UINT;
      break;
    case Mysqlx::Resultset::ColumnMetaData::DOUBLE:
      column.type = mysqlx::DOUBLE;
      break;
    case Mysqlx::Resultset::ColumnMetaData::FLOAT:
      column.type = mysqlx::FLOAT;
      break;
    case Mysqlx::Resultset::ColumnMetaData::BYTES:
      column.type = mysqlx::BYTES;
      break;
    case Mysqlx::Resultset::ColumnMetaData::TIME:
      column.type = mysqlx::TIME;
      break;
    case Mysqlx::Resultset::ColumnMetaData::DATETIME:
      column.type = mysqlx::DATETIME;
      break;
    case Mysqlx::Resultset::ColumnMetaData::SET:
      column.type = mysqlx::SET;
      break;
    case Mysqlx::Resultset::ColumnMetaData::ENUM:
      column.type = mysqlx::ENUM;
      break;
    case Mysqlx::Resultset::ColumnMetaData::BIT:
      column.type = mysqlx::BIT;
      break;
    case Mysqlx::Resultset::ColumnMetaData::DECIMAL:
      column.type = mysqlx::DECIMAL;
      break;
  }
  column.name = column_data.name();
  column.original_name = column_data.original_name();

  column.table = column_data.table();
  column.original_table = column_data.original_table();

  column.schema = column_data.schema();
  column.catalog = column_data.catalog();

  column.collation = column_data.has_collation() ? column_data.collation() : 0;

  column.fractional_digits = column_data.fractional_digits();

  column.length = column_data.length();

  column.flags = column_data.flags();
  column.content_type = column_data.content_type();
  return column;
}

void Result::read_metadata()
{
  if (m_state != ReadMetadata && m_state != ReadMetadataI)
    throw std::logic_error("read_metadata() called at wrong time");

  // msgs we can get in this state:
  // CURSOR_OK
  // META_DATA

  int msgid = -1;
  m_columns.reset(new std::vector<ColumnMetadata>());
  while (m_state == ReadMetadata || m_state == ReadMetadataI)
  {
    if (-1 != msgid)
    {
      delete pop_message();
    }

    msgid = get_message_id();

    if (msgid == Mysqlx::ServerMessages::RESULTSET_COLUMN_META_DATA)
    {
      msgid = -1;
      std::auto_ptr<Mysqlx::Resultset::ColumnMetaData> column_data(static_cast<Mysqlx::Resultset::ColumnMetaData*>(pop_message()));

      m_columns->push_back(unwrap_column_metadata(*column_data));
    }
  }
}

std::auto_ptr<Row> Result::read_row()
{
  if (m_state != ReadRows)
    throw std::logic_error("read_row() called at wrong time");

  // msgs we can get in this state:
  // RESULTSET_ROW
  // RESULTSET_FETCH_DONE
  // RESULTSET_FETCH_DONE_MORE_RESULTSETS
  int mid = get_message_id();

  if (mid == Mysqlx::ServerMessages::RESULTSET_ROW)
    return std::auto_ptr<Row>(new Row(m_columns, static_cast<Mysqlx::Resultset::Row*>(pop_message())));

  return std::auto_ptr<Row>();
}

void Result::read_stmt_ok()
{
  if (m_state != ReadStmtOk && m_state != ReadStmtOkI)
    throw std::logic_error("read_stmt_ok() called at wrong time");

  // msgs we can get in this state:
  // STMT_EXEC_OK

  if (Mysqlx::ServerMessages::RESULTSET_FETCH_DONE == get_message_id())
    delete pop_message();

  if (Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK != get_message_id())
    throw std::runtime_error("Unexpected message id");

  std::auto_ptr<mysqlx::Message> msg(pop_message());
}

bool Result::nextDataSet()
{
  // flush left over rows
  while (m_state == ReadRows)
    read_row();

  if (m_state == ReadMetadata)
  {
    read_metadata();
    if (m_state == ReadRows)
      return true;
  }
  if (m_state == ReadStmtOk)
    read_stmt_ok();
  return false;
}

Row *Result::next()
{
  if (!ready())
    wait();

  if (m_state == ReadStmtOk)
  {
    read_stmt_ok();
    return NULL;
  }
  if (m_state == ReadDone)
    return NULL;

  std::auto_ptr<Row> row(read_row());

  if (m_state == ReadStmtOk)
    read_stmt_ok();

  return row.release();
}

void Result::discardData()
{
  // Flushes the leftover data
  wait();
  while (nextDataSet());
}

Row::Row(boost::shared_ptr<std::vector<ColumnMetadata> > columns, Mysqlx::Resultset::Row *data)
: m_columns(columns), m_data(data)
{
}

Row::~Row()
{
  delete m_data;
}

void Row::check_field(int field, FieldType type) const
{
  if (field < 0 || field >= (int)m_columns->size())
    throw std::range_error("invalid field index");

  if (m_columns->at(field).type != type)
    throw std::range_error("invalid field type");
}

bool Row::isNullField(int field) const
{
  if (field < 0 || field >= (int)m_columns->size())
    throw std::range_error("invalid field index");

  if (m_data->field(field).empty())
    return true;
  return false;
}

int32_t Row::sIntField(int field) const
{
  int64_t t = sInt64Field(field);
  if (t > std::numeric_limits<int32_t>::max() || t < std::numeric_limits<int32_t>::min())
    throw std::invalid_argument("field of wrong type");

  return (int32_t)t;
}

uint32_t Row::uIntField(int field) const
{
  uint64_t t = uInt64Field(field);
  if (t > std::numeric_limits<uint32_t>::max())
    throw std::invalid_argument("field of wrong type");

  return (uint32_t)t;
}

int64_t Row::sInt64Field(int field) const
{
  check_field(field, SINT);
  const std::string& field_val = m_data->field(field);

  return Row_decoder::s64_from_buffer(field_val);
}

uint64_t Row::uInt64Field(int field) const
{
  check_field(field, UINT);
  const std::string& field_val = m_data->field(field);

  return Row_decoder::u64_from_buffer(field_val);
}

uint64_t Row::bitField(int field) const
{
  check_field(field, BIT);
  const std::string& field_val = m_data->field(field);

  return Row_decoder::u64_from_buffer(field_val);
}

std::string Row::stringField(int field) const
{
  size_t length;
  check_field(field, BYTES);

  const std::string& field_val = m_data->field(field);

  const char* res = Row_decoder::string_from_buffer(field_val, length);
  return std::string(res, length);
}

std::string Row::decimalField(int field) const
{
  check_field(field, DECIMAL);

  const std::string& field_val = m_data->field(field);

  mysqlx::Decimal decimal = Row_decoder::decimal_from_buffer(field_val);

  return std::string(decimal.str());
}

std::string Row::setFieldStr(int field) const
{
  check_field(field, SET);

  const std::string& field_val = m_data->field(field);

  return Row_decoder::set_from_buffer_as_str(field_val);
}

std::set<std::string> Row::setField(int field) const
{
  std::set<std::string> result;
  check_field(field, SET);

  const std::string& field_val = m_data->field(field);
  Row_decoder::set_from_buffer(field_val, result);

  return result;
}

std::string Row::enumField(int field) const
{
  size_t length;
  check_field(field, ENUM);

  const std::string& field_val = m_data->field(field);

  const char* res = Row_decoder::string_from_buffer(field_val, length);
  return std::string(res, length);
}

const char *Row::stringField(int field, size_t &rlength) const
{
  check_field(field, BYTES);

  const std::string& field_val = m_data->field(field);

  return Row_decoder::string_from_buffer(field_val, rlength);
}

float Row::floatField(int field) const
{
  check_field(field, FLOAT);

  const std::string& field_val = m_data->field(field);

  return Row_decoder::float_from_buffer(field_val);
}

double Row::doubleField(int field) const
{
  check_field(field, DOUBLE);

  const std::string& field_val = m_data->field(field);

  return Row_decoder::double_from_buffer(field_val);
}

DateTime Row::dateTimeField(int field) const
{
  check_field(field, DATETIME);

  const std::string& field_val = m_data->field(field);

  return Row_decoder::datetime_from_buffer(field_val);
}

Time Row::timeField(int field) const
{
  check_field(field, TIME);

  const std::string& field_val = m_data->field(field);

  return Row_decoder::time_from_buffer(field_val);
}

int Row::numFields() const
{
  return m_data->field_size();
}

#ifdef WIN32
#  pragma pop_macro("ERROR")
#endif
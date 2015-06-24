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
#include "mysqlx.h"
#include "mysqlx_connection.h"
#include "mysqlx_crud.h"

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
#include <mysql/service_my_snprintf.h>
#include "compilerutils.h"

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


AuthError::~AuthError() BOOST_NOEXCEPT_OR_NOTHROW
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
: m_socket(m_ios), m_trace_packets(false)
{
  if (getenv("MYSQLX_TRACE_CONNECTION"))
    m_trace_packets = true;
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
    throw Error(CR_WRONG_HOST_INFO, "Unsupported protocol "+protocol);

  if (!pass.empty())
    password = pass;

  connect(host, port, schema, user, pass.empty() ? password : pass);
}

void Connection::connect(const std::string &host, int port)
{
  tcp::resolver resolver(m_ios);
  char ports[8];
  my_snprintf(ports, sizeof(ports), "%i", port);
  tcp::resolver::query query(host, ports);

  tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
  tcp::resolver::iterator end;
  boost::system::error_code error = boost::asio::error::host_not_found;
  while (error && endpoint_iterator != end)
  {
    m_socket.close();
    m_socket.connect(*endpoint_iterator++, error);
  }

  if (error)
  {
    switch (error.value())
    {
      case boost::asio::error::host_not_found:
        throw Error(CR_UNKNOWN_HOST, error.message());

      default:
        throw Error(CR_CONNECTION_ERROR, error.message()+" connecting to "+host+":"+ports);
    }
  }
}


void Connection::connect(const std::string &host, int port, const std::string &schema,
                         const std::string &user, const std::string &pass)
{
  connect(host, port);

  authenticate_plain(user, pass, schema);
}


void Connection::close()
{
  m_socket.close();
}


Result *Connection::execute_sql(const std::string &sql)
{
  {
    Mysqlx::Sql::StmtExecute exec;
    exec.set_stmt(sql);
    send(exec);
  }
  return new Result(this, true, true);
}


Result *Connection::execute_find(const Mysqlx::Crud::Find &m)
{
  send(m);

  return new Result(this, false, true);
}


Result *Connection::execute_update(const Mysqlx::Crud::Update &m)
{
  send(m);

  return new Result(this, false, false);
}


Result *Connection::execute_insert(const Mysqlx::Crud::Insert &m)
{
  send(m);

  return new Result(this, false, false);
}


Result *Connection::execute_delete(const Mysqlx::Crud::Delete &m)
{
  send(m);

  return new Result(this, false, false);
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
    std::auto_ptr<Message> message(recv(mid));
    switch (mid)
    {
      case Mysqlx::ServerMessages::SESS_AUTHENTICATE_OK:
        break;

      case Mysqlx::ServerMessages::SESS_AUTHENTICATE_FAIL:
      {
        Mysqlx::Session::AuthenticateFail *fail(static_cast<Mysqlx::Session::AuthenticateFail*>(message.get()));
        throw AuthError(fail->msg());
      }

      case Mysqlx::ServerMessages::ERROR:
        throw_server_error(*static_cast<Mysqlx::Error*>(message.get()));

      default:
        throw Error(CR_MALFORMED_PACKET, "Unexpected message received from server");
        break;
    }
  }
}


void Connection::send(int mid, const Message &msg)
{
  boost::system::error_code error;
  char buf[5];
  *(int32_t*)buf = htonl(msg.ByteSize() + 5);
  buf[4] = mid;

  if (m_trace_packets)
  {
    std::string out;
    google::protobuf::TextFormat::Printer p;
    p.SetInitialIndentLevel(1);
    p.PrintToString(msg, &out);
    std::cout << ">>>> SEND " << msg.GetDescriptor()->full_name() << "\n" << out << "\n";
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


Message *Connection::recv(int &mid)
{
  char buf[5];
  Message* ret_val = NULL;
  boost::system::error_code error;

  /*std::size_t length = */boost::asio::read(m_socket, boost::asio::buffer(buf, 5), boost::asio::transfer_all(), error);

  if (!error)
  {
    int msglen = ntohl(*(int32_t*)buf) - 5;
    mid = buf[4];
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
        std::cout << "<<<< RECEIVE " << mid << "\n" << out << "\n";
      }

      if (!ret_val->IsInitialized())
      {
        delete[] mbuf;
        delete ret_val;
        std::string err("Message is not properly initialized : ");
        err += ret_val->InitializationErrorString();
        throw Error(CR_MALFORMED_PACKET, err);
      }
    }
    delete[] mbuf;
  }
  else
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
  return ret_val;
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


Document::Document(const std::string &doc)
: m_data(new std::string(doc))
{
}


Document::Document(const Document &doc)
: m_data(doc.m_data)
{
}


Result::Result(Connection *owner, bool needs_stmt_ok, bool expect_data)
: m_owner(owner), m_last_insert_id(-1), m_affected_rows(-1), m_needs_stmt_ok(needs_stmt_ok),
  m_state(expect_data ? ReadMetadataI : ReadStmtOkI)
{
}


Result::~Result()
{
  // flush the resultset from the pipe
  while (m_state != ReadError && m_state != ReadDone)
    nextResult();
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


int Result::get_message_id()
{
  if (NULL != current_message.get())
  {
    return current_message_id;
  }

  current_message.reset(m_owner->recv(current_message_id));

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

        case Mysqlx::ServerMessages::SQL_COLUMN_META_DATA:
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
        case Mysqlx::ServerMessages::SQL_COLUMN_META_DATA:
          m_state = ReadMetadata;
          return current_message_id;

        case Mysqlx::ServerMessages::SQL_ROW:
          m_state = ReadRows;
          return current_message_id;

        case Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE:
          // empty resultset
          if (m_needs_stmt_ok)
            m_state = ReadStmtOk;
          else
            m_state = ReadDone;
          return current_message_id;
      }
      break;
    }
    case ReadRows:
    {
      switch (current_message_id)
      {
        case Mysqlx::ServerMessages::SQL_ROW:
          return current_message_id;

        case Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE:
          if (m_needs_stmt_ok)
            m_state = ReadStmtOk;
          else
            m_state = ReadDone;
          return current_message_id;

        case Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE_MORE_RESULTSETS:
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


static ColumnMetadata unwrap_column_metadata(Mysqlx::Sql::ColumnMetaData *column_data)
{
  ColumnMetadata column;

  switch (column_data->type())
  {
    case Mysqlx::Sql::ColumnMetaData::SINT:
      column.type = mysqlx::SINT;
      break;
    case Mysqlx::Sql::ColumnMetaData::UINT:
      column.type = mysqlx::UINT;
      break;
    case Mysqlx::Sql::ColumnMetaData::DOUBLE:
      column.type = mysqlx::DOUBLE;
      break;
    case Mysqlx::Sql::ColumnMetaData::FLOAT:
      column.type = mysqlx::FLOAT;
      break;
    case Mysqlx::Sql::ColumnMetaData::BYTES:
      column.type = mysqlx::BYTES;
      break;
    case Mysqlx::Sql::ColumnMetaData::TIME:
      column.type = mysqlx::TIME;
      break;
    case Mysqlx::Sql::ColumnMetaData::DATETIME:
      column.type = mysqlx::DATETIME;
      break;
    case Mysqlx::Sql::ColumnMetaData::SET:
      column.type = mysqlx::SET;
      break;
    case Mysqlx::Sql::ColumnMetaData::ENUM:
      column.type = mysqlx::ENUM;
      break;
    case Mysqlx::Sql::ColumnMetaData::BIT:
      column.type = mysqlx::BIT;
      break;
    case Mysqlx::Sql::ColumnMetaData::DECIMAL:
      column.type = mysqlx::DECIMAL;
      break;
  }
  column.name = column_data->name();
  column.original_name = column_data->original_name();

  column.table = column_data->table();
  column.original_table = column_data->original_table();

  column.schema = column_data->schema();
  column.catalog = column_data->catalog();

  column.charset = column_data->charset();

  column.fractional_digits = column_data->fractional_digits();

  column.length = column_data->length();

  column.flags = column_data->flags();
  column.content_type = column_data->content_type();
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
  std::auto_ptr<mysqlx::Message> msg;
  m_columns.reset(new std::vector<ColumnMetadata>());
  while (m_state == ReadMetadata || m_state == ReadMetadataI)
  {
    if (-1 != msgid)
    {
      pop_message();
    }

    msgid = get_message_id();

    if (msgid == Mysqlx::ServerMessages::SQL_COLUMN_META_DATA)
    {
      msgid = -1;
      m_columns->push_back(unwrap_column_metadata(static_cast<Mysqlx::Sql::ColumnMetaData*>(pop_message().release())));
    }
  }
}


std::auto_ptr<Row> Result::read_row()
{
  if (m_state != ReadRows)
    throw std::logic_error("read_row() called at wrong time");

  // msgs we can get in this state:
  // SQL_ROW
  // SQL_CURSOR_FETCH_DONE
  // SQL_CURSOR_FETCH_DONE_MORE_RESULTSETS
  int mid = get_message_id();

  if (mid == Mysqlx::ServerMessages::SQL_ROW)
    return std::auto_ptr<Row>(new Row(m_columns, std::auto_ptr<Mysqlx::Sql::Row>(static_cast<Mysqlx::Sql::Row*>(pop_message().release()))));

  return std::auto_ptr<Row>();
}


void Result::read_stmt_ok()
{
  if (m_state != ReadStmtOk && m_state != ReadStmtOkI)
    throw std::logic_error("read_stmt_ok() called at wrong time");

  // msgs we can get in this state:
  // STMT_EXEC_OK

  if (Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE == get_message_id())
    pop_message();

  if (Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK != get_message_id())
    throw std::runtime_error("Unexpected message id");

  std::auto_ptr<mysqlx::Message> msg(pop_message());
  Mysqlx::Sql::StmtExecuteOk *ok(static_cast<Mysqlx::Sql::StmtExecuteOk*>(msg.get()));

  if (ok->has_rows_affected())
    m_affected_rows = ok->rows_affected();
  if (ok->has_last_insert_id())
    m_last_insert_id = ok->last_insert_id();
}


bool Result::nextResult()
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

  return read_row().release();
}

void Result::discardData()
{
  // Flushes the leftover data
  while (nextResult());
}



Row::Row(boost::shared_ptr<std::vector<ColumnMetadata> > columns, std::auto_ptr<Mysqlx::Sql::Row> data)
: m_columns(columns), m_data(data)
{
}


void Row::check_field(int field, FieldType type) const
{
  if (field < 0 || field >= (int)m_columns->size())
    throw std::range_error("invalid field index");

  if (m_columns->at(field).type != type)
    throw std::range_error("invalid field type");

  if (!m_data->field(field).has_scalar())
    throw Error(CR_MALFORMED_PACKET, "Unexpected data received from server");
}


bool Row::isNullField(int field) const
{
  if (field < 0 || field >= (int)m_columns->size())
    throw std::range_error("invalid field index");

  if (!m_data->field(field).has_scalar() || m_data->field(field).scalar().type() == Mysqlx::Datatypes::Scalar::V_NULL)
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

  Mysqlx::Datatypes::Scalar::Type stype = m_data->field(field).scalar().type();
  if (stype == Mysqlx::Datatypes::Scalar::V_SINT && m_data->field(field).scalar().has_v_signed_int())
    return m_data->field(field).scalar().v_signed_int();
  else if (stype == Mysqlx::Datatypes::Scalar::V_UINT && m_data->field(field).scalar().has_v_unsigned_int())
  {
    int64_t t = (int64_t)m_data->field(field).scalar().v_unsigned_int();
    if (t >= 0)
      return t;
  }
  throw std::invalid_argument("field of wrong type");
}


uint64_t Row::uInt64Field(int field) const
{
  check_field(field, UINT);

  Mysqlx::Datatypes::Scalar::Type stype = m_data->field(field).scalar().type();
  if (stype == Mysqlx::Datatypes::Scalar::V_UINT && m_data->field(field).scalar().has_v_unsigned_int())
    return m_data->field(field).scalar().v_unsigned_int();
  else if (stype == Mysqlx::Datatypes::Scalar::V_SINT && m_data->field(field).scalar().has_v_signed_int())
  {
    int64_t t = m_data->field(field).scalar().v_signed_int();
    if (t >= 0)
      return (uint64_t)t;
  }
  throw std::invalid_argument("field of wrong type");
}


const std::string &Row::stringField(int field) const
{
  check_field(field, BYTES);

  if (m_data->field(field).scalar().type() == Mysqlx::Datatypes::Scalar::V_OCTETS
      && m_data->field(field).scalar().has_v_opaque())
    return m_data->field(field).scalar().v_opaque();
  throw std::invalid_argument("field of wrong type");
}


const char *Row::stringField(int field, size_t &rlength) const
{
  check_field(field, BYTES);

  if (m_data->field(field).scalar().type() == Mysqlx::Datatypes::Scalar::V_OCTETS
      && m_data->field(field).scalar().has_v_opaque())
  {
    const std::string &tmp(m_data->field(field).scalar().v_opaque());
    rlength = tmp.length();
    return tmp.data();
  }
  throw std::invalid_argument("field of wrong type");
}


float Row::floatField(int field) const
{
  check_field(field, FLOAT);

  if (m_data->field(field).scalar().type() == Mysqlx::Datatypes::Scalar::V_FLOAT
      && m_data->field(field).scalar().has_v_float())
  {
    return m_data->field(field).scalar().v_float();
  }
  throw std::invalid_argument("field of wrong type");
}


double Row::doubleField(int field) const
{
  check_field(field, DOUBLE);

  if (m_data->field(field).scalar().type() == Mysqlx::Datatypes::Scalar::V_DOUBLE
      && m_data->field(field).scalar().has_v_double())
  {
    return m_data->field(field).scalar().v_double();
  }
  throw std::invalid_argument("field of wrong type");
}



DateTime Row::dateTimeField(int field) const
{
  check_field(field, DATETIME);

  if (m_data->field(field).scalar().type() == Mysqlx::Datatypes::Scalar::V_OCTETS
      && m_data->field(field).scalar().has_v_opaque())
  {
    DateTime dt(m_data->field(field).scalar().v_opaque());
    if (!dt)
      throw Error(CR_MALFORMED_PACKET, "Invalid data received");
    return dt;
  }
  throw std::invalid_argument("field of wrong type");
}


Time Row::timeField(int field) const
{
  check_field(field, TIME);

  if (m_data->field(field).scalar().type() == Mysqlx::Datatypes::Scalar::V_OCTETS
      && m_data->field(field).scalar().has_v_opaque())
  {
    Time t(m_data->field(field).scalar().v_opaque());
    if (!t)
      throw Error(CR_MALFORMED_PACKET, "Invalid data received");
    return t;
  }
  throw std::invalid_argument("field of wrong type");
}


int Row::numFields() const
{
  return m_data->field_size();
}

#ifdef WIN32
#  pragma pop_macro("ERROR")
#endif

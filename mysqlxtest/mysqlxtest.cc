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

#include <google/protobuf/text_format.h>
#include "mysqlx.h"
#include "mysqlx_crud.h"
#include "mysqlx_connection.h"

#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/tokenizer.h>

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <string.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>

#include "expr_parser.h"
#include "utils_mysql_parsing.h"

#include "m_string.h" // needed by writer.h, but has to be included after expr_parser.h
#include <rapidjson/writer.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>

const char CMD_ARG_SEPARATOR = '\t';

#include <mysql/service_my_snprintf.h>

#ifdef _MSC_VER
#  pragma push_macro("ERROR")
#  undef ERROR
#endif


using namespace google::protobuf;

typedef std::map<std::string, std::string> Message_by_full_name;
static Message_by_full_name server_msgs_by_full_name;
static Message_by_full_name client_msgs_by_full_name;


typedef std::map<std::string, std::pair<mysqlx::Message* (*)(), int8_t> > Message_by_name;

static Message_by_name server_msgs_by_name;
static Message_by_name client_msgs_by_name;

typedef std::map<int8_t, std::pair<mysqlx::Message* (*)(), std::string> > Message_by_id;
static Message_by_id server_msgs_by_id;
static Message_by_id client_msgs_by_id;

bool OPT_quiet = false;
bool OPT_bindump = false;
bool OPT_show_warnings = false;
bool OPT_fatal_errors = false;

class Expected_error;
static Expected_error *OPT_expect_error = 0;

static int current_line_number = 0;


static std::map<std::string, std::string> variables;
static std::list<std::string> variables_to_unreplace;

void replace_variables(std::string &s)
{
  for (std::map<std::string, std::string>::const_iterator sub = variables.begin();
       sub != variables.end(); ++sub)
  {
    std::string tmp(sub->second);
    boost::replace_all(tmp, "\"", "\\\"");
    boost::replace_all(tmp, "\n", "\\n");
    boost::replace_all(s, sub->first, tmp);
  }
}


std::string unreplace_variables(const std::string &in, bool clear)
{
  std::string s = in;
  for (std::list<std::string>::const_iterator sub = variables_to_unreplace.begin();
       sub != variables_to_unreplace.end(); ++sub)
  {
    boost::replace_all(s, variables[*sub], *sub);
  }
  if (clear)
    variables_to_unreplace.clear();
  return s;
}


static void print_result_set(mysqlx::Result &result);
static std::string message_to_text(const mysqlx::Message &message);


class Expected_error
{
public:
  Expected_error() {}

  void expect_errno(int err)
  {
    m_expect_errno.push_back(err);
  }

  bool check_error(const mysqlx::Error &err)
  {
    if (!m_expect_errno.empty())
    {
      if (std::find(m_expect_errno.begin(), m_expect_errno.end(), err.error()) == m_expect_errno.end())
      {
        std::cerr << "ERROR: was expecting error ";
        for (std::list<int>::const_iterator i = m_expect_errno.begin(); i != m_expect_errno.end(); ++i)
          std::cout << *i << " ";
        std::cerr << " but instead got " << err.what() << " (code "<<err.error()<<")\n";
        m_expect_errno.clear();
        if (OPT_fatal_errors)
          return false;
      }
      else
      {
        if (m_expect_errno.size() == 1)
          std::cout << "Got expected error: " << err.what() << " (code "<<err.error()<<")\n";
        else
        {
          std::cerr << "Got expecting error (one of ";
          for (std::list<int>::const_iterator i = m_expect_errno.begin(); i != m_expect_errno.end(); ++i)
            std::cout << *i << " ";
          std::cerr <<")\n";
        }
      }
    }
    else
      std::cerr << "ERROR: " << err.what() << " (code "<<err.error()<<")\n";
    m_expect_errno.clear();
    return true;
  }

  bool check_ok()
  {
    if (!m_expect_errno.empty())
    {
      std::cout << "ERROR: was expecting error ";
      for (std::list<int>::const_iterator i = m_expect_errno.begin(); i != m_expect_errno.end(); ++i)
        std::cout << *i << " ";
      std::cout << " but succeeded\n";
      m_expect_errno.clear();
      if (OPT_fatal_errors)
        return false;
    }
    m_expect_errno.clear();
    return true;
  }
private:
  std::list<int> m_expect_errno;
};


class Connection_manager
{
public:
  Connection_manager(const std::string &uri, const mysqlx::Ssl_config &ssl_config_)
  : ssl_config(ssl_config_)
  {
    int pwdfound;
    mysqlx::parse_mysql_connstring(uri, proto, user, pass, host, port, sock, db, pwdfound);

    active_connection.reset(new mysqlx::Connection(ssl_config));
    connections[""] = active_connection;

    active_connection->connect(host, port);
  }

  void connect_default(const bool send_cap_password_expired = false)
  {
    if (send_cap_password_expired)
      active_connection->setup_capability("client.pwd_expire_ok", true);

    active_connection->authenticate(user, pass, db);

    std::stringstream s;
    s << active_connection->client_id();
    variables["%ACTIVE_CLIENT_ID%"] = s.str();
  }

  void create(const std::string &name,
              const std::string &user, const std::string &password, const std::string &db,
              bool no_ssl)
  {
    if (connections.find(name) != connections.end())
      throw std::runtime_error("a session named "+name+" already exists");

    std::cout << "connecting...\n";
    if (no_ssl)
        active_connection.reset(new mysqlx::Connection(mysqlx::Ssl_config()));
    else
    active_connection.reset(new mysqlx::Connection(ssl_config));
    active_connection->connect(host, port);
    if (user != "-")
    {
    if (user.empty())
      active_connection->authenticate(this->user, this->pass, db.empty() ? this->db : db);
    else
      active_connection->authenticate(user, password, db.empty() ? this->db : db);
    }
    active_connection_name = name;
    connections[name] = active_connection;
    std::stringstream s;
    s << active_connection->client_id();
    variables["%ACTIVE_CLIENT_ID%"] = s.str();
    std::cout << "active session is now '" << name << "'\n";
  }

  void abort_active()
  {
    if (active_connection)
    {
      if (!active_connection_name.empty())
        std::cout << "aborting session " << active_connection_name << "\n";
      active_connection->set_closed();
      active_connection.reset();
      connections.erase(active_connection_name);
      if (active_connection_name != "")
        set_active("");
    }
    else
      throw std::runtime_error("no active session");
  }

  void close_active(bool shutdown = false)
  {
    if (active_connection)
    {
      if (active_connection_name.empty() && !shutdown)
        throw std::runtime_error("cannot close default session");
      try
      {
      if (!active_connection_name.empty())
        std::cout << "closing session " << active_connection_name << "\n";

        if (!active_connection->is_closed())
        {
      // send a close message and wait for the corresponding Ok message
      active_connection->send(Mysqlx::Session::Close());
      active_connection->set_closed();
      int msgid;
      std::auto_ptr<mysqlx::Message> msg(active_connection->recv_raw(msgid));
      std::cout << message_to_text(*msg);
      if (Mysqlx::ServerMessages::OK != msgid || static_cast<Mysqlx::Ok*>(msg.get())->msg() != "bye!")
        throw mysqlx::Error(CR_COMMANDS_OUT_OF_SYNC,
                            "Disconnect was expecting Mysqlx.Ok(bye!), but got the one above (one or more calls to -->recv are probably missing)");
      connections.erase(active_connection_name);
      if (!shutdown)
        set_active("");
    }
      }
      catch (...)
      {
        connections.erase(active_connection_name);
        if (!shutdown)
          set_active("");
        throw;
      }
    }
    else if (!shutdown)
      throw std::runtime_error("no active session");
  }

  void set_active(const std::string &name)
  {
    if (connections.find(name) == connections.end())
    {
      std::string slist;
      for (std::map<std::string, boost::shared_ptr<mysqlx::Connection> >::const_iterator it = connections.begin(); it != connections.end(); ++it)
        slist.append(it->first).append(", ");
      if (!slist.empty())
        slist.resize(slist.length()-2);
      throw std::runtime_error("no session named '"+name+"': " + slist);
    }
    active_connection = connections[name];
    active_connection_name = name;
    std::stringstream s;
    s << active_connection->client_id();
    variables["%ACTIVE_CLIENT_ID%"] = s.str();
    std::cout << "switched to session " << (active_connection_name.empty() ? "default" : active_connection_name) << "\n";
  }

  mysqlx::Connection* active()
  {
    if (!active_connection)
      std::runtime_error("no active session");
    return active_connection.get();
  }

private:
  std::map<std::string, boost::shared_ptr<mysqlx::Connection> > connections;
  boost::shared_ptr<mysqlx::Connection> active_connection;
  std::string active_connection_name;
  std::string proto, user, pass, host, sock, db;
  int port;
  mysqlx::Ssl_config ssl_config;
};



static std::string message_to_text(const mysqlx::Message &message)
{
  std::string output;
  std::string name;

  google::protobuf::TextFormat::Printer printer;

  // special handling for nested messages (at least for Notices)
  if (message.GetDescriptor()->full_name() == "Mysqlx.Notice.Frame")
  {
    Mysqlx::Notice::Frame frame = *static_cast<const Mysqlx::Notice::Frame*>(&message);
    switch (frame.type())
    {
      case 1: // warning
      {
        Mysqlx::Notice::Warning subm;
        subm.ParseFromString(frame.payload());
        printer.PrintToString(subm, &output);
        frame.set_payload(subm.GetDescriptor()->full_name() + " { " + output + " }");
        break;
      }
      case 2: // session variable
      {
        Mysqlx::Notice::SessionVariableChanged subm;
        subm.ParseFromString(frame.payload());
        printer.PrintToString(subm, &output);
        frame.set_payload(subm.GetDescriptor()->full_name() + " { " + output + " }");
        break;
      }
      case 3: // session state
      {
        Mysqlx::Notice::SessionStateChanged subm;
        subm.ParseFromString(frame.payload());
        printer.PrintToString(subm, &output);
        frame.set_payload(subm.GetDescriptor()->full_name() + " { " + output + " }");
        break;
      }
    }
    printer.SetInitialIndentLevel(1);
    printer.PrintToString(frame, &output);
  }
  else
  {
    printer.SetInitialIndentLevel(1);
    printer.PrintToString(message, &output);
  }

  return message.GetDescriptor()->full_name() + " {\n" + output + "}\n";
}

static std::string message_to_bindump(const mysqlx::Message &message)
{
  std::string res;
  std::string out;

  message.SerializeToString(&out);

  res.resize(5);
  *(uint32_t*)res.data() = out.size() + 1;

  res[4] = client_msgs_by_name[client_msgs_by_full_name[message.GetDescriptor()->full_name()]].second;
  res.append(out);
  out = res;

  res.clear();
  for (size_t i = 0; i < out.length(); i++)
  {
    if (out[i] == '\\' && i >= 5)
    {
      res.push_back('\\');
      res.push_back('\\');
    }
    else if (isprint(out[i]) && i >= 5 && !isblank(out[i]))
      res.push_back(out[i]);
    else
  {
      static const char *hex = "0123456789abcdef";
      res.append("\\x");
      res.push_back(hex[(out[i] >> 4) & 0xf]);
      res.push_back(hex[out[i] & 0xf]);
    }
  }
  return res;
  }


static std::string bindump_to_data(const std::string &bindump)
{
  std::string res;
  for (size_t i = 0; i < bindump.length(); i++)
  {
    if (bindump[i] == '\\')
    {
      if (bindump[i+1] == '\\')
      {
        res.push_back('\\');
        ++i;
}
      else if (bindump[i+1] == 'x')
      {
        int value = 0;
        static const char *hex = "0123456789abcdef";
        const char *p = strchr(hex, bindump[i+2]);
        if (p)
          value = (p - hex) << 4;
        else
        {
          std::cerr << "ERROR: Invalid bindump char at " << i+2 <<"\n";
          break;
        }
        p = strchr(hex, bindump[i+3]);
        if (p)
          value |= p - hex;
        else
        {
          std::cerr << "ERROR: Invalid bindump char at " << i+3 <<"\n";
          break;
        }
        i += 3;
        res.push_back(value);
      }
    }
    else
      res.push_back(bindump[i]);
  }
  return res;
}


/*
static mysqlx::Message *text_to_server_message(const std::string &name, const std::string &data)
{
  if (server_msgs_by_full_name.find(name) == server_msgs_by_full_name.end())
  {
    std::cerr << "Invalid message type " << name << "\n";
    return NULL;
  }
  mysqlx::Message *message = server_msgs_by_name[server_msgs_by_full_name[name]].first();

  google::protobuf::TextFormat::ParseFromString(data, message);

  return message;
}
*/


class ErrorDumper : public ::google::protobuf::io::ErrorCollector
{
  std::stringstream m_out;

public:
  virtual void AddError(int line, int column, const string & message)
  {
    m_out << "ERROR in message: " << line+1 << ": " << column << ": " << message<<"\n";
  }

  virtual void AddWarning(int line, int column, const string & message)
  {
    m_out << "WARNING in message: " << line+1 << ": " << column << ": " << message<<"\n";
  }

  std::string str() { return m_out.str(); }
};

static mysqlx::Message *text_to_client_message(const std::string &name, const std::string &data, int8_t &msg_id)
{
  if (client_msgs_by_full_name.find(name) == client_msgs_by_full_name.end())
  {
    std::cerr << current_line_number << ": Invalid message type " << name << "\n";
    return NULL;
  }

  Message_by_name::const_iterator msg = client_msgs_by_name.find(client_msgs_by_full_name[name]);
  if (msg == client_msgs_by_name.end())
  {
    std::cerr << current_line_number << ": Invalid message type " << name << "\n";
    return NULL;
  }

  mysqlx::Message *message = msg->second.first();
  msg_id = msg->second.second;

  google::protobuf::TextFormat::Parser parser;
  ErrorDumper dumper;
  parser.RecordErrorsTo(&dumper);
  if (!parser.ParseFromString(data, message))
  {
    std::cerr << "ERROR: " << current_line_number << ": Invalid message in input: " << name << "\n";
    int i = 1;
    for (std::string::size_type p = 0, n = data.find('\n', p+1);
         p != std::string::npos;
         p = (n == std::string::npos ? n : n+1), n = data.find('\n', p+1), ++i)
    {
      std::cerr << i << ": " << data.substr(p, n-p) << "\n";
    }
    std::cerr << "\n" << dumper.str();
    delete message;
    return NULL;
  }

  return message;
}


static bool dump_notices(int type, const std::string &data)
{
  if (type == 3)
  {
    Mysqlx::Notice::SessionStateChanged change;
    change.ParseFromString(data);
    if (!change.IsInitialized())
      std::cerr << "Invalid notice received from server " << change.InitializationErrorString() << "\n";
    else
    {
      if (change.param() == Mysqlx::Notice::SessionStateChanged::ACCOUNT_EXPIRED)
      {
        std::cout << "NOTICE: Account password expired\n";
        return true;
      }
    }
  }
  return false;
}


class Execution_context
{
public:
  Execution_context(std::istream &stream, Connection_manager *cm)
  :m_stream(stream), m_cm(cm)
  {
  }

  std::istream       &m_stream;
  Connection_manager *m_cm;

  mysqlx::Connection *connection() { return m_cm->active(); }
};

class Command
{
public:
  enum Result {Continue, Stop_with_success, Stop_with_failure};

  Command()
  : m_cmd_prefix("-->")
  {
    m_commands["title "]      = &Command::cmd_title;
    m_commands["echo "]       = &Command::cmd_echo;
    m_commands["recvtype "]   = &Command::cmd_recvtype;
    m_commands["recverror "]  = &Command::cmd_recverror;
    m_commands["recvresult"]  = &Command::cmd_recvresult;
    m_commands["recvuntil "]  = &Command::cmd_recvuntil;
    m_commands["enablessl"]   = &Command::cmd_enablessl;
    m_commands["sleep "]      = &Command::cmd_sleep;
    m_commands["login "]      = &Command::cmd_login;
    m_commands["loginerror "] = &Command::cmd_loginerror;
    m_commands["repeat "]     = &Command::cmd_repeat;
    m_commands["endrepeat"]   = &Command::cmd_endrepeat;
    m_commands["system "]     = &Command::cmd_system;
    m_commands["peerdisc "]   = &Command::cmd_peerdisc;
    m_commands["recv"]        = &Command::cmd_recv;
    m_commands["exit"]        = &Command::cmd_exit;
    m_commands["abort"]        = &Command::cmd_abort;
    m_commands["nowarnings"]  = &Command::cmd_nowarnings;
    m_commands["yeswarnings"] = &Command::cmd_yeswarnings;
    m_commands["fatalerrors"] = &Command::cmd_fatalerrors;
    m_commands["nofatalerrors"] = &Command::cmd_nofatalerrors;
    m_commands["newsession "]  = &Command::cmd_newsession;
    m_commands["newsessionplain "]  = &Command::cmd_newsessionplain;
    m_commands["setsession "]  = &Command::cmd_setsession;
    m_commands["setsession"]  = &Command::cmd_setsession; // for setsession with no args
    m_commands["closesession"]= &Command::cmd_closesession;
    m_commands["expecterror "] = &Command::cmd_expecterror;
    m_commands["quiet"]       = &Command::cmd_quiet;
    m_commands["noquiet"]     = &Command::cmd_noquiet;
    m_commands["varfile "]     = &Command::cmd_varfile;
    m_commands["varlet "]      = &Command::cmd_varlet;
    m_commands["varsub "]      = &Command::cmd_varsub;
    m_commands["vargen "]      = &Command::cmd_vargen;
    m_commands["binsend "]     = &Command::cmd_binsend;
  }

  bool is_command_syntax(const std::string &cmd) const
  {
    return 0 == strncmp(cmd.c_str(), m_cmd_prefix.c_str(), m_cmd_prefix.length());
  }

  Result process(Execution_context &context, const std::string &command)
  {
    if (!is_command_syntax(command))
      return Stop_with_failure;

    Command_map::iterator i = std::find_if(m_commands.begin(),
                                           m_commands.end(),
                                           boost::bind(&Command::match_command_name, this, _1, command));

    if (i == m_commands.end())
    {
      std::cerr << "Unknown command " << command << "\n";
      return Stop_with_failure;
    }

    return (*this.*(*i).second)(context, command.c_str() + m_cmd_prefix.length() + (*i).first.length());
  }

private:
  typedef std::map< std::string, Result (Command::*)(Execution_context &,const std::string &) > Command_map;

  struct Loop_do
  {
    std::streampos block_begin;
    int                  iterations;
  };

  Command_map        m_commands;
  std::list<Loop_do> m_loop_stack;
  std::string        m_cmd_prefix;

  bool match_command_name(const Command_map::value_type &command, const std::string &instruction)
  {
    if (m_cmd_prefix.length() + command.first.length() > instruction.length())
      return false;

    std::string::const_iterator i = std::find(instruction.begin(), instruction.end(), ' ');
    std::string                 command_name(instruction.begin() + m_cmd_prefix.length(), i);

    if (0 != command.first.compare(command_name))
    {
      if (instruction.end() != i)
      {
        ++i;
        return 0 == command.first.compare(std::string(instruction.begin() + m_cmd_prefix.length(), i));
      }

      return false;
    }

    return true;
  }

  Result cmd_echo(Execution_context &context, const std::string &args)
  {
    std::string s = args;
    replace_variables(s);
    std::cout << s << "\n";

    return Continue;
  }

  Result cmd_title(Execution_context &context, const std::string &args)
  {
    if (!args.empty())
    {
      std::cout << "\n" << args.substr(1) << "\n";
      std::string sep(args.length()-1, args[0]);
      std::cout << sep << "\n";
    }
    else
      std::cout << "\n\n";

    return Continue;
  }

  Result cmd_recvtype(Execution_context &context, const std::string &args)
  {
    int msgid;
    std::auto_ptr<mysqlx::Message> msg(context.connection()->recv_raw(msgid));
    if (msg.get())
    {
      if (msg->GetDescriptor()->full_name() != args)
        std::cout << "Received unexpected message. Was expecting:\n    " << args << "\nbut got:\n";
      try
      {
        std::cout << message_to_text(*msg) << "\n";

        if (msg->GetDescriptor()->full_name() != args && OPT_fatal_errors)
          return Stop_with_success;
      }
      catch (std::exception &e)
      {
        std::cerr << "ERROR: "<< e.what()<<"\n";
        if (OPT_fatal_errors)
          return Stop_with_success;
      }
    }

    return Continue;
  }

  Result cmd_recverror(Execution_context &context, const std::string &args)
  {
    int msgid;
    std::auto_ptr<mysqlx::Message> msg(context.connection()->recv_raw(msgid));
    if (msg.get())
    {
      bool failed = false;
      if (msg->GetDescriptor()->full_name() != "Mysqlx.Error" || (uint32_t)atoi(args.c_str()) != static_cast<Mysqlx::Error*>(msg.get())->code())
      {
        std::cout << "ERROR: Was expecting Error " << args <<", but got:\n";
        failed = true;
      }
      else
      {
        std::cout << "Got expected error:\n";
      }
      try
      {
        std::cout << message_to_text(*msg) << "\n";
        if (failed && OPT_fatal_errors)
          return Stop_with_success;
      }
      catch (std::exception &e)
      {
        std::cerr << "ERROR: "<< e.what()<<"\n";
        if (OPT_fatal_errors)
          return Stop_with_success;
      }
    }

    return Continue;
  }

  Result cmd_recvresult(Execution_context &context, const std::string &args)
  {
    std::auto_ptr<mysqlx::Result> result;
    try
    {
      result.reset(context.connection()->recv_result());
      print_result_set(*result);
      variables_to_unreplace.clear();
      int64_t x = result->affectedRows();
      if (x >= 0)
        std::cout << x << " rows affected\n";
      else
        std::cout << "command ok\n";
      if (result->lastInsertId() > 0)
        std::cout << "last insert id: " << result->lastInsertId() << "\n";
      if (!result->infoMessage().empty())
        std::cout << result->infoMessage() << "\n";
      {
        std::vector<mysqlx::Result::Warning> warnings(result->getWarnings());
        if (!warnings.empty())
          std::cout << "Warnings generated:\n";
        for (std::vector<mysqlx::Result::Warning>::const_iterator w = warnings.begin();
             w != warnings.end(); ++w)
        {
          std::cout << (w->is_note ? "NOTE" : "WARNING") << " | " << w->code << " | " << w->text << "\n";
        }
      }

      if (!OPT_expect_error->check_ok())
        return Stop_with_failure;
    }
    catch (mysqlx::Error &err)
    {
      if (result.get())
        result->mark_error();
      if (!OPT_expect_error->check_error(err))
        return Stop_with_failure;
    }
    return Continue;
  }

  Result cmd_recvuntil(Execution_context &context, const std::string &args)
  {
    int msgid;
    for (;;)
    {
      std::auto_ptr<mysqlx::Message> msg(context.connection()->recv_raw(msgid));
      if (msg.get())
      {
        try
        {
          std::cout << message_to_text(*msg) << "\n";
        }
        catch (std::exception &e)
        {
          std::cerr << "ERROR: "<< e.what()<<"\n";
          if (OPT_fatal_errors)
            return Stop_with_success;
        }
        if (msg->GetDescriptor()->full_name() == args)
        {
          break;
        }
        else if (msgid == Mysqlx::ServerMessages::ERROR)
          break;
      }
    }
    variables_to_unreplace.clear();
    return Continue;
  }

  Result cmd_enablessl(Execution_context &context, const std::string &args)
  {
    try
    {
      context.connection()->enable_tls();
    }
    catch (const mysqlx::Error &error)
    {
      std::cerr << "ERROR: "<< error.what() << "\n";
      return Stop_with_failure;
    }

    return Continue;
  }

  Result cmd_sleep(Execution_context &context, const std::string &args)
  {
    boost::asio::io_service service;
    boost::asio::deadline_timer dt(service);

    dt.expires_from_now(boost::posix_time::milliseconds(atoi(args.c_str())));

    dt.wait();

    return Continue;
  }

  Result cmd_login(Execution_context &context, const std::string &args)
  {
    std::string s = args;
    std::string user, pass, db, auth_meth;
    std::string::size_type p = s.find(CMD_ARG_SEPARATOR);
    if (p != std::string::npos)
    {
      user = s.substr(0, p);
      s = s.substr(p+1);
      p = s.find(CMD_ARG_SEPARATOR);
      if (p != std::string::npos)
      {
        pass = s.substr(0, p);
        s = s.substr(p+1);
        p = s.find(CMD_ARG_SEPARATOR);
        if (p != std::string::npos)
        {
          db = s.substr(0, p);
          auth_meth = s.substr(p+1);
        }
        else
          db = s;
      }
      else
        pass = s;
    }
    else
      user = s;

    void (mysqlx::Connection::*method)(const std::string &, const std::string &, const std::string &);

    method = &mysqlx::Connection::authenticate_mysql41;

    try
    {
      context.connection()->push_local_notice_handler(boost::bind(dump_notices, _1, _2));
      //XXX
      // Prepered for method map
      if (0 == strncmp(auth_meth.c_str(), "plain", 5))
      {
        method = &mysqlx::Connection::authenticate_plain;
      }
      else if ( !(0 == strncmp(auth_meth.c_str(), "mysql41", 5) || 0 == auth_meth.length()))
        throw mysqlx::Error(CR_UNKNOWN_ERROR, "Wrong authentication method");

      (context.connection()->*method)(user, pass, db);

      context.connection()->pop_local_notice_handler();

      std::cout << "Login OK\n";
    }
    catch (mysqlx::Error &err)
    {
      context.connection()->pop_local_notice_handler();
      std::cerr << "ERROR: " << err.what() << " (code " << err.error() << ")\n";
      if (OPT_fatal_errors)
        return Stop_with_success;
    }

    return Continue;
  }

  Result cmd_repeat(Execution_context &context, const std::string &args)
  {
    Loop_do loop = {context.m_stream.tellg(), atoi(args.c_str())};

    m_loop_stack.push_back(loop);

    return Continue;
  }

  Result cmd_endrepeat(Execution_context &context, const std::string &args)
  {
    while (m_loop_stack.size())
    {
      Loop_do &ld = m_loop_stack.back();

      --ld.iterations;

      if (1 > ld.iterations)
      {
        m_loop_stack.pop_back();
        break;
      }

      context.m_stream.seekg(ld.block_begin);
      break;
    }

    return Continue;
  }

  Result cmd_loginerror(Execution_context &context, const std::string &args)
  {
    std::string s = args;
    std::string expected, user, pass, db;
    std::string::size_type p = s.find('\t');
    if (p != std::string::npos)
    {
      expected = s.substr(0, p);
      s = s.substr(p+1);
      p = s.find('\t');
      if (p != std::string::npos)
      {
        user = s.substr(0, p);
        s = s.substr(p+1);
        p = s.find('\t');
        if (p != std::string::npos)
        {
          pass = s.substr(0, p+1);
          db = s.substr(p+1);
        }
        else
          pass = s;
      }
      else
        user = s;
    }
    else
    {
      std::cout << current_line_number << ": Missing arguments to -->loginerror\n";
      return Stop_with_failure;
    }
    try
    {
      context.connection()->push_local_notice_handler(boost::bind(dump_notices, _1, _2));

      context.connection()->authenticate_mysql41(user, pass, db);

      context.connection()->pop_local_notice_handler();

      std::cout << "ERROR: Login succeeded, but an error was expected\n";
      if (OPT_fatal_errors)
        return Stop_with_failure;
    }
    catch (mysqlx::Error &err)
    {
      context.connection()->pop_local_notice_handler();

      if (err.error() == (int32_t)atoi(expected.c_str()))
        std::cerr << "error (as expected): " << err.what() << " (code " << err.error() << ")\n";
      else
      {
        std::cerr << "ERROR: was expecting: " << expected << " but got: " << err.what() << " (code " << err.error() << ")\n";
        if (OPT_fatal_errors)
          return Stop_with_failure;
      }
    }

    return Continue;
  }

  Result cmd_system(Execution_context &context, const std::string &args)
  {
    // XXX - remove command
    // command used only at dev level
    // example of usage
    // -->system (sleep 3; echo "Killing"; ps aux | grep mysqld | egrep -v "gdb .+mysqld" | grep -v  "kdeinit4"| awk '{print($2)}' | xargs kill -s SIGQUIT)&
    if (0 == system(args.c_str()))
      return Continue;

    return Stop_with_failure;
  }

  Result cmd_peerdisc(Execution_context &context, const std::string &args)
  {
    int expected_delta_time;
    int tolerance;
    int result = sscanf(args.c_str(),"%i %i", &expected_delta_time, &tolerance);

    if (result <1 && result > 2)
    {
      std::cerr << "ERROR: Invalid use of command\n";

      return Stop_with_failure;
    }

    if (1 == result)
    {
      tolerance = 10 * expected_delta_time / 100;
    }

    boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::local_time();
    try
    {
      int msgid;

      std::auto_ptr<mysqlx::Message> msg(context.connection()->recv_raw_with_deadline(msgid, 2 * expected_delta_time));

      if (msg.get())
      {
        std::cerr << "ERROR: Received unexpected message.\n";
        std::cerr << message_to_text(*msg) << "\n";
      }
      else
      {
        throw std::runtime_error("Timeout occur while waiting for disconnection.");
      }
      return Stop_with_failure;
    }
    catch (const mysqlx::Error &ec)
    {
      if (CR_SERVER_GONE_ERROR != ec.error())
      {
        std::cerr << "ERROR: "<< ec.what() << "\n";
        return Stop_with_failure;
      }
    }

    int execution_delta_time = (boost::posix_time::microsec_clock::local_time() - start_time).total_milliseconds();

    if (abs(execution_delta_time - expected_delta_time) > tolerance)
    {
      std::cerr << "ERROR: Peer disconnected after: "<< execution_delta_time << "[ms], expected: " << expected_delta_time << "[ms]\n";
      return Stop_with_failure;
    }

    return Stop_with_success;
  }

  Result cmd_recv(Execution_context &context, const std::string &args)
  {
    int msgid;
    try
    {
    std::auto_ptr<mysqlx::Message> msg(context.connection()->recv_raw(msgid));

    if (msg.get())
        std::cout << unreplace_variables(message_to_text(*msg), true) << "\n";
      if (!OPT_expect_error->check_ok())
        return Stop_with_failure;
    }
    catch (mysqlx::Error &e)
    {
      if (!OPT_expect_error->check_error(e))
        return Stop_with_failure;
      }
      catch (std::exception &e)
      {
        std::cerr << "ERROR: "<< e.what()<<"\n";
      if (OPT_fatal_errors)
        return Stop_with_failure;
      }
    return Continue;
  }

  Result cmd_exit(Execution_context &context, const std::string &args)
  {
    return Stop_with_success;
  }

  Result cmd_abort(Execution_context &context, const std::string &args)
  {
    exit(2);
    return Stop_with_success;
  }

  Result cmd_nowarnings(Execution_context &context, const std::string &args)
  {
    OPT_show_warnings = false;
    return Continue;
  }

  Result cmd_yeswarnings(Execution_context &context, const std::string &args)
  {
    OPT_show_warnings = true;
    return Continue;
  }

  Result cmd_fatalerrors(Execution_context &context, const std::string &args)
  {
    OPT_fatal_errors = true;
    return Continue;
  }

  Result cmd_nofatalerrors(Execution_context &context, const std::string &args)
  {
    OPT_fatal_errors = false;
    return Continue;
  }

  Result cmd_newsessionplain(Execution_context &context, const std::string &args)
  {
    return do_newsession(context, args, true);
  }

  Result cmd_newsession(Execution_context &context, const std::string &args)
  {
    return do_newsession(context, args, false);
  }

  Result do_newsession(Execution_context &context, const std::string &args, bool plain)
  {
    std::string s = args;
    std::string user, pass, db, name;
    std::string::size_type p = s.find(CMD_ARG_SEPARATOR);
    if (p != std::string::npos)
    {
      name = s.substr(0, p);
      s = s.substr(p+1);
      p = s.find(CMD_ARG_SEPARATOR);
      if (p != std::string::npos)
      {
        user = s.substr(0, p);
        s = s.substr(p+1);
        p = s.find(CMD_ARG_SEPARATOR);
        if (p != std::string::npos)
        {
          pass = s.substr(0, p);
          db = s.substr(p+1);
        }
        else
          pass = s;
      }
      else
        user = s;
    }
    else
      name = s;

    try
    {
      context.m_cm->create(name, user, pass, db, plain);
      if (!OPT_expect_error->check_ok())
          return Stop_with_failure;
      }
    catch (mysqlx::Error &err)
    {
      if (!OPT_expect_error->check_error(err))
        return Stop_with_failure;
    }

    return Continue;
  }

  Result cmd_setsession(Execution_context &context, const std::string &args)
  {
    if (!args.empty() && (args[0] == ' ' || args[0] == '\t'))
      context.m_cm->set_active(args.substr(1));
    else
      context.m_cm->set_active(args);
    return Continue;
  }

  Result cmd_closesession(Execution_context &context, const std::string &args)
  {
    try
    {
      if (args == " abort")
        context.m_cm->abort_active();
      else
    context.m_cm->close_active();
      if (!OPT_expect_error->check_ok())
        return Stop_with_failure;
    }
    catch (mysqlx::Error &err)
    {
      if (!OPT_expect_error->check_error(err))
        return Stop_with_failure;
    }
    return Continue;
  }

  Result cmd_expecterror(Execution_context &context, const std::string &args)
  {
    if (!args.empty())
    {
      std::vector<std::string> argl;
      boost::split(argl, args, boost::is_any_of(","), boost::token_compress_on);
      for (std::vector<std::string>::const_iterator arg = argl.begin(); arg != argl.end(); ++arg)
        OPT_expect_error->expect_errno(atoi(arg->c_str()));
    }
    else
    {
      std::cerr << "expecterror requires an errno argument\n";
      return Stop_with_failure;
    }
    return Continue;
  }

  Result cmd_quiet(Execution_context &context, const std::string &args)
  {
    OPT_quiet = true;

    return Continue;
  }

  Result cmd_noquiet(Execution_context &context, const std::string &args)
  {
    OPT_quiet = false;

    return Continue;
  }

  Result cmd_varsub(Execution_context &context, const std::string &args)
  {
    variables_to_unreplace.push_back(args);
    return Continue;
  }

  Result cmd_varlet(Execution_context &context, const std::string &args)
  {
    std::string::size_type p = args.find(' ');
    if (p == std::string::npos)
    {
      if (variables.find(args) == variables.end())
      {
        std::cerr << "Invalid variable " << args << "\n";
      return Stop_with_failure;
    }
      variables.erase(args);
    }
    else
    {
      std::string value = args.substr(p+1);
      replace_variables(value);
      variables[args.substr(0, p)] = value;
    }
    return Continue;
  }

  Result cmd_vargen(Execution_context &context, const std::string &args)
  {
    std::vector<std::string> argl;
    boost::split(argl, args, boost::is_any_of(" "), boost::token_compress_on);
    if (argl.size() != 3)
    {
      std::cerr << "Invalid number of arguments for command vargen\n";
      return Stop_with_failure;
    }
    std::string data(atoi(argl[2].c_str()), *argl[1].c_str());
    variables[argl[0]] = data;
    return Continue;
  }

  Result cmd_varfile(Execution_context &context, const std::string &args)
  {
    std::vector<std::string> argl;
    boost::split(argl, args, boost::is_any_of(" "), boost::token_compress_on);
    if (argl.size() != 2)
    {
      std::cerr << "Invalid number of arguments for command varfile " << args << "\n";
      return Stop_with_failure;
    }

    std::ifstream file(argl[1].c_str());
    if (!file.is_open())
    {
      std::cerr << "Coult not open file " << argl[1]<<"\n";
      return Stop_with_failure;
    }

    file.seekg(0, file.end);
    size_t len = file.tellg();
    file.seekg(0);

    char *buffer = new char[len];
    file.read(buffer, len);
    variables[argl[0]] = std::string(buffer, len);
    delete []buffer;

    return Continue;
  }

  Result cmd_binsend(Execution_context &context, const std::string &args)
    {
    std::string data = bindump_to_data(args);
    std::cout << "Sending " << data.length() << " bytes raw data...\n";
    context.m_cm->active()->send_bytes(data);
    return Continue;
  }
};


static int process_client_message(mysqlx::Connection *connection, int8_t msg_id, const mysqlx::Message &msg)
{
  if (!OPT_quiet)
    std::cout << "send " << message_to_text(msg) << "\n";

  if (OPT_bindump)
    std::cout << message_to_bindump(msg) << "\n";

  try
  {
    // send request
    connection->send(msg_id, msg);

    if (!OPT_expect_error->check_ok())
      return 1;
  }
  catch (mysqlx::Error &err)
  {
    if (!OPT_expect_error->check_error(err))
    return 1;
  }
  return 0;
}

static void print_result_set(mysqlx::Result &result)
{
  boost::shared_ptr<std::vector<mysqlx::ColumnMetadata> > meta(result.columnMetadata());

  for (std::vector<mysqlx::ColumnMetadata>::const_iterator col = meta->begin();
       col != meta->end(); ++col)
  {
    if (col != meta->begin())
      std::cout << "\t";
    std::cout << col->name;
  }
  std::cout << "\n";

  for (;;)
  {
    std::auto_ptr<mysqlx::Row> row(result.next());
    if (!row.get())
      break;

    for (int i = 0; i < row->numFields(); i++)
    {
      if (i != 0)
        std::cout << "\t";

      if (row->isNullField(i))
      {
        std::cout << "null";
        continue;
      }

      try
      {
        const mysqlx::ColumnMetadata &col(meta->at(i));

        switch (col.type)
        {
          case mysqlx::SINT:
            std::cout << row->sInt64Field(i);
            break;
          case mysqlx::UINT:
            std::cout << row->uInt64Field(i);
            break;
          case mysqlx::DOUBLE:
            if (col.fractional_digits >= 31)
            {
              char buffer[100];
              my_gcvt(row->doubleField(i), MY_GCVT_ARG_DOUBLE, sizeof(buffer)-1, buffer, NULL);
              std::cout << buffer;
            }
            else
            {
              char buffer[100];
              my_fcvt(row->doubleField(i), col.fractional_digits, buffer, NULL);
              std::cout << buffer;
            }
            break;
          case mysqlx::FLOAT:
            if (col.fractional_digits >= 31)
            {
              char buffer[100];
              my_gcvt(row->floatField(i), MY_GCVT_ARG_FLOAT, sizeof(buffer)-1, buffer, NULL);
              std::cout << buffer;
            }
            else
            {
              char buffer[100];
              my_fcvt(row->floatField(i), col.fractional_digits, buffer, NULL);
              std::cout << buffer;
            }
            break;
          case mysqlx::BYTES:
            {
              std::string tmp(row->stringField(i));
              std::cout << unreplace_variables(tmp, false);
            }
            break;
          case mysqlx::TIME:
            std::cout << row->timeField(i);
            break;
          case mysqlx::DATETIME:
            std::cout << row->dateTimeField(i);
            break;
          case mysqlx::DECIMAL:
            std::cout << row->decimalField(i);
            break;
          case mysqlx::SET:
            std::cout << row->setFieldStr(i);
            break;
          case mysqlx::ENUM:
            std::cout << row->enumField(i);
            break;
          case mysqlx::BIT:
            std::cout << row->bitField(i);
            break;
        }
      }
      catch (std::exception &e)
      {
        std::cout << "ERROR: " << e.what() << "\n";
      }
    }
    std::cout << "\n";
  }
}


static int run_sql_batch(mysqlx::Connection *conn, const std::string &sql_)
{
  std::string delimiter = ";";
  std::vector<std::pair<size_t, size_t> > ranges;
  std::stack<std::string> input_context_stack;
  std::string sql = sql_;

  replace_variables(sql);

  shcore::mysql::splitter::determineStatementRanges(sql.data(), sql.length(), delimiter,
                                           ranges, "\n", input_context_stack);

  for (std::vector<std::pair<size_t, size_t> >::const_iterator st = ranges.begin(); st != ranges.end(); ++st)
  {
    try
    {
      if (!OPT_quiet)
          std::cout << "RUN " << sql.substr(st->first, st->second) << "\n";
      std::auto_ptr<mysqlx::Result> result(conn->execute_sql(sql.substr(st->first, st->second)));
      if (result.get())
      {
        do
        {
          print_result_set(*result.get());
        }
        while (result->nextDataSet());
        int64_t x = result->affectedRows();
        if (x >= 0)
          std::cout << x << " rows affected\n";
        if (result->lastInsertId() > 0)
          std::cout << "last insert id: " << result->lastInsertId() << "\n";
        if (!result->infoMessage().empty())
          std::cout << result->infoMessage() << "\n";

        if (OPT_show_warnings)
        {
          std::vector<mysqlx::Result::Warning> warnings(result->getWarnings());
          if (!warnings.empty())
            std::cout << "Warnings generated:\n";
          for (std::vector<mysqlx::Result::Warning>::const_iterator w = warnings.begin();
               w != warnings.end(); ++w)
          {
            std::cout << (w->is_note ? "NOTE" : "WARNING") << " | " << w->code << " | " << w->text << "\n";
          }
        }
      }
    }
    catch (mysqlx::Error &err)
    {
      std::cerr << "ERROR executing " << sql.substr(st->first, st->second) << ":\n";
      std::cerr << "ERROR: " << err.what() << " (code " << err.error() << ")\n";
      variables_to_unreplace.clear();
      if (OPT_fatal_errors)
        return 1;
    }
  }
  variables_to_unreplace.clear();
  return 0;
}


enum Block_result
{
  Block_result_feed_more,
  Block_result_eated_but_not_hungry,
  Block_result_not_hungry,
  Block_result_indigestion,
  Block_result_everyone_not_hungry
};

class Block_processor
{
public:
  virtual ~Block_processor() {}

  virtual Block_result feed(std::istream &input, const char *linebuf, bool normalize_only) = 0;
  virtual bool feed_ended_is_state_ok() { return true; }
};

typedef boost::shared_ptr<Block_processor> Block_processor_ptr;


class Sql_block_processor : public Block_processor
{
public:
  Sql_block_processor(Connection_manager *cm)
  : m_cm(cm), m_sql(false)
  {}


  virtual Block_result feed(std::istream &input, const char *linebuf, bool normalize_only)
  {
    if (m_sql)
    {
      if (normalize_only)
        std::cout << linebuf << "\n";

      if (strcmp(linebuf, "-->endsql") == 0)
      {
        if (!normalize_only)
        {
          int r = run_sql_batch(m_cm->active(), m_rawbuffer);
          if (r != 0)
          {
            return Block_result_indigestion;
          }
        }
        m_sql = false;

        return Block_result_eated_but_not_hungry;
      }
      else
        m_rawbuffer.append(linebuf).append("\n");

      return Block_result_feed_more;
    }

    // -->command
    if (strcmp(linebuf, "-->sql") == 0)
    {
      m_rawbuffer.clear();
      m_sql = true;
      // feed everything until -->endraw to the mysql client

      if (normalize_only)
        std::cout << linebuf << "\n";

      return Block_result_feed_more;
    }

    return Block_result_not_hungry;
  }

  virtual bool feed_ended_is_state_ok()
  {
    if (m_sql)
    {
      std::cerr << current_line_number << ": Unclosed -->sql directive\n";
      return false;
    }

    return true;
  }

private:
  Connection_manager *m_cm;
  std::string m_rawbuffer;
  bool m_sql;
};

class Single_command_processor: public Block_processor
{
public:
  Single_command_processor(Connection_manager *cm)
  : m_cm(cm)
  {
  }

  virtual Block_result feed(std::istream &input, const char *linebuf, bool normalize_only)
  {
    Execution_context context(input, m_cm);

    if (m_command.is_command_syntax(linebuf))
    {
      if (normalize_only)
        std::cout << linebuf << "\n";
      else
      {
        Command::Result r = m_command.process(context, linebuf);
        if (Command::Stop_with_failure == r)
          return Block_result_indigestion;
        else if (Command::Stop_with_success == r)
          return Block_result_everyone_not_hungry;
      }

      return Block_result_eated_but_not_hungry;
    }
    // # comment
    else if (linebuf[0] == '#' || linebuf[0] == 0)
    {
      if (normalize_only)
        std::cout << linebuf << "\n";

      return Block_result_eated_but_not_hungry;
    }

    return Block_result_not_hungry;
  }

private:
  Command m_command;
  Connection_manager *m_cm;
};

class Snd_message_block_processor: public Block_processor
{
public:
  Snd_message_block_processor(Connection_manager *cm)
  : m_cm(cm)
  {
  }

  virtual Block_result feed(std::istream &input, const char *linebuf, bool normalize_only)
  {
    const char *p;
    if (m_full_name.empty())
    {
      if ((p = strstr(linebuf, " {")))
      {
        m_full_name = std::string(linebuf, p-linebuf);
        m_buffer.clear();
        return Block_result_feed_more;
      }
    }
    else
    {
      if (linebuf[0] == '}')
      {
        int8_t msg_id;
        std::string processed_buffer = m_buffer;
        replace_variables(processed_buffer);

        std::auto_ptr<mysqlx::Message> msg(text_to_client_message(m_full_name, processed_buffer, msg_id));

        m_full_name.clear();
        if (!msg.get())
          return Block_result_indigestion;

        if (normalize_only)
        {
          std::cout << message_to_text(*msg.get());
        }
        else
        {
          int r = process_client_message(m_cm->active(), msg_id, *msg.get());
          if (r != 0)
            return Block_result_indigestion;
        }

        return Block_result_eated_but_not_hungry;
      }
      else
      {
        m_buffer.append(linebuf).append("\n");
        return Block_result_feed_more;
      }
    }

    return Block_result_not_hungry;
  }

  virtual bool feed_ended_is_state_ok()
  {
    if (!m_full_name.empty())
    {
      std::cerr << current_line_number << ": Incomplete message " << m_full_name << "\n";
      return false;
    }

    return true;
  }

private:
  Connection_manager *m_cm;
  std::string m_buffer;
  std::string m_full_name;
};


static int process_client_input(std::istream &input, std::vector<Block_processor_ptr> &eaters, bool normalize_only = false)
{
  const std::size_t buffer_length = 32 * 1024;
  char              linebuf[buffer_length + 1];

  linebuf[buffer_length] = 0;

  if (!input.good())
  {
    std::cerr << "Input stream isn't valid\n";

    return 1;
  }

  Block_processor_ptr hungry_block_reader;

  while (!input.eof())
  {
    Block_result result = Block_result_not_hungry;

    input.getline(linebuf, buffer_length);
    ++current_line_number;

    if (!hungry_block_reader)
    {
      std::vector<Block_processor_ptr>::iterator i = eaters.begin();

      while (i != eaters.end() &&
             Block_result_not_hungry == result)
      {
        result = (*i)->feed(input, linebuf, normalize_only);

        if (Block_result_indigestion == result)
          return 1;

        if (Block_result_feed_more == result)
          hungry_block_reader = (*i);

        ++i;
      }

      if (Block_result_everyone_not_hungry == result)
        break;

      continue;
    }

    result = hungry_block_reader->feed(input, linebuf, normalize_only);

    if (Block_result_indigestion == result)
      return 1;

    if (Block_result_feed_more != result)
      hungry_block_reader.reset();

    if (Block_result_everyone_not_hungry == result)
      break;
  }

  std::vector<Block_processor_ptr>::iterator i = eaters.begin();

  while (i != eaters.end())
  {
    if (!(*i)->feed_ended_is_state_ok())
      return 1;

    ++i;
  }

  return 0;
}


#include "cmdline_options.h"

class My_command_line_options : public Command_line_options
{
public:
  enum Run_mode{
    RunTest,
    RunTestWithoutAuth,
    Normalize
  } run_mode;

  std::string run_file;
  bool        has_file;
  bool        cap_expired_password;

  int port;
  std::string host;
  std::string uri;
  std::string password;
  std::string schema;
  mysqlx::Ssl_config ssl;

  void print_help()
  {
    std::cout << "mysqlxtest <options>\n";
    std::cout << "Options:\n";
    std::cout << "-f, --file=<file>     Reads input from file\n";
    std::cout << "-n, --no-auth         Skip authentication which is required by -->sql block (run mode)\n";
    std::cout << "-N, --normalize       Normalize input and print it back instead of executing (run mode)\n";
    std::cout << "-u, --user=<user>a    Connection user\n";
    std::cout << "-p, --password=<pass> Connection password\n";
    std::cout << "-h, --host=<host>     Connection host\n";
    std::cout << "-P, --port=<port>     Connection port\n";
    std::cout << "--schema=<schema>     Default schema to connect to\n";
    std::cout << "--uri=<uri>           Connection URI\n";
    std::cout << "--ssl-key             X509 key in PEM format\n";
    std::cout << "--ssl-ca              CA file in PEM format\n";
    std::cout << "--ssl-ca_path         CA directory\n";
    std::cout << "--ssl-cert            X509 cert in PEM format\n";
    std::cout << "--ssl-cipher          SSL cipher to use\n";
    std::cout << "--connect-expired-password Allow expired password\n";
    std::cout << "--quiet               Don't print out messages sent";
    std::cout << "-B, --bindump         Dump binary representation of messages sent, in format suitable for ";
    std::cout << "--help                     Show command line help\n";
    std::cout << "--help-commands            Show help for input commands\n";
    std::cout << "\nOnly one option that changes run mode is allowed.\n";
  }

  void print_help_commands()
  {
    std::cout << "Input may be a file (or if no --file is specified, it stdin will be used)\n";
    std::cout << "The following commands may appear in the input script:\n";
    std::cout << "-->echo <text>\n";
    std::cout << "  Prints the text (allows variables)\n";
    std::cout << "-->title <c><text>\n";
    std::cout << "  Prints the text with an underline, using the character <c>\n";
    std::cout << "-->sql\n";
    std::cout << "  Begins SQL block. SQL statements that appear will be executed and results printed (allows variables).\n";
    std::cout << "-->endsql\n";
    std::cout << "  End SQL block. End a block of SQL started by -->sql\n";
    std::cout << "-->enablessl\n";
    std::cout << "  Enables ssl on current connection\n";
    std::cout << "<protomsg>\n";
    std::cout << "  Encodes the text format protobuf message and sends it to the server (allows variables).\n";
    std::cout << "-->recv\n";
    std::cout << "  Read and print one message from the server\n";
    std::cout << "-->recvresult\n";
    std::cout << "  Read and print one resultset from the server\n";
    std::cout << "-->recverror <errno>\n";
    std::cout << "  Read a message and ensure that it's an error of the expected type\n";
    std::cout << "-->recvtype <msgtype>\n";
    std::cout << "  Read one message and print it, checking that its type is the specified one\n";
    std::cout << "-->recvuntil <msgtype>\n";
    std::cout << "  Read messages and print them, until a msg of the specified type (or Error) is received\n";
    std::cout << "-->repeat <N>\n";
    std::cout << "  Begin block of instructions that should be repeated N times\n";
    std::cout << "-->endrepeat\n";
    std::cout << "  End block of instructions that should be repeated - next iteration\n";
    std::cout << "-->system <CMD>\n";
    std::cout << "  Execute application or script (dev only)\n";
    std::cout << "-->exit\n";
    std::cout << "  Stops reading commands, disconnects and exits (same as <eof>/^D)\n";
    std::cout << "-->abort\n";
    std::cout << "  Exit immediately, without performing cleanup\n";
    std::cout << "-->nowarnings/-->yeswarnings\n";
    std::cout << "   Whether to print warnings generated by the statement (default no)\n";
    std::cout << "-->peerdisc <MILISECONDS> [TOLERANCE]\n";
    std::cout << "  Expect that xplugin disconnects after given number of milliseconds and tolerance\n";
    std::cout << "-->sleep <SECONDS>\n";
    std::cout << "  Stops execution of mysqxtest for given number of seconds\n";
    std::cout << "-->login <user>\t<pass>\t<db>\t<mysql41|plain>]\n";
    std::cout << "  Performs authentication steps (use with --no-auth)\n";
    std::cout << "-->loginerror <errno>\t<user>\t<pass>\t<db>\n";
    std::cout << "  Performs authentication steps expecting an error (use with --no-auth)\n";
    std::cout << "-->fatalerrors/nofatalerrors\n";
    std::cout << "  Whether to immediately exit on MySQL errors\n";
    std::cout << "-->expecterror <errno>\n";
    std::cout << "  Expect a specific errof for the next command and fail if something else occurs\n";
    std::cout << "  Works for: newsession, closesession, recvresult\n";
    std::cout << "-->newsession <name>\t<user>\t<pass>\t<db>\n";
    std::cout << "  Create a new connection with given name and account (use - as user for no-auth)\n";
    std::cout << "-->newsessionplain <name>\t<user>\t<pass>\t<db>\n";
    std::cout << "  Create a new connection with given name and account and force it to NOT use ssl, even if its generally enabled\n";
    std::cout << "-->setsession <name>\n";
    std::cout << "  Activate the named session\n";
    std::cout << "-->closesession [abort]\n";
    std::cout << "  Close the active session (unless its the default session)\n";
    std::cout << "-->varfile <varname> <datafile>\n";
    std::cout << "   Assigns the contents of the file to the named variable\n";
    std::cout << "-->varlet <varname> <value>\n";
    std::cout << "   Assign the value (can be another variable) to the variable\n";
    std::cout << "-->varsub <varname>\n";
    std::cout << "   Add a variable to the list of variables to replace for the next recv or sql command (value is replaced by the name)\n";
    std::cout << "-->binsend <bindump>[<bindump>...]\n";
    std::cout << "   Sends one or more binary message dumps to the server (generate those with --bindump)\n";
    std::cout << "-->quiet/noquiet\n";
    std::cout << "   Toggle verbose messages\n";
    std::cout << "# comment\n";
  }

  bool set_mode(Run_mode mode)
  {
    if (RunTest != run_mode)
      return false;

    run_mode = mode;

    return true;
  }

  My_command_line_options(int argc, char **argv)
  : Command_line_options(argc, argv), run_mode(RunTest), has_file(false), cap_expired_password(false), port(0)
  {
    std::string user;

    run_mode = RunTest; // run tests by default

    for (int i = 1; i < argc && exit_code == 0; i++)
    {
      char *value;
      if (check_arg_with_value(argv, i, "--file", "-f", value))
      {
        run_file = value;
        has_file = true;
      }
      else if (check_arg(argv, i, "--normalize", "-N"))
      {
        if (!set_mode(Normalize))
        {
          std::cerr << "Only one option that changes run mode is allowed.\n";
          exit_code = 1;
        }
      }
      else if (check_arg(argv, i, "--no-auth", "-n"))
      {
        if (!set_mode(RunTestWithoutAuth))
        {
          std::cerr << "Only one option that changes run mode is allowed.\n";
          exit_code = 1;
        }
      }
      else if (check_arg_with_value(argv, i, "--password", "-p", value))
        password = value;
      else if (check_arg_with_value(argv, i, "--ssl-key", NULL, value))
        ssl.key = value;
      else if (check_arg_with_value(argv, i, "--ssl-ca", NULL, value))
        ssl.ca = value;
      else if (check_arg_with_value(argv, i, "--ssl-ca_path", NULL, value))
        ssl.ca_path = value;
      else if (check_arg_with_value(argv, i, "--ssl-cert", NULL, value))
        ssl.cert = value;
      else if (check_arg_with_value(argv, i, "--ssl-cipher", NULL, value))
        ssl.cipher = value;
      else if (check_arg_with_value(argv, i, "--host", "-h", value))
        host = value;
      else if (check_arg_with_value(argv, i, "--user", "-u", value))
        user = value;
      else if (check_arg_with_value(argv, i, "--schema", NULL, value))
        schema = value;
      else if (check_arg_with_value(argv, i, "--port", "-P", value))
        port = atoi(value);
      else if (check_arg_with_value(argv, i, "--password", "-p", value))
        password = value;
      else if (check_arg(argv, i, "--bindump", "-B"))
        OPT_bindump = true;
      else if (check_arg(argv, i, "--connect-expired-password", NULL))
        cap_expired_password = true;
      else if (check_arg(argv, i, "--quiet", "-q"))
        OPT_quiet = true;
      else if (check_arg(argv, i, "--help", "--help"))
      {
        print_help();
        exit_code = 1;
      }
      else if (check_arg(argv, i, "--help-commands", "--help-commands"))
      {
        print_help_commands();
        exit_code = 1;
      }
      else if (exit_code == 0)
      {
        std::cerr << argv[0] << ": unknown option " << argv[i] << "\n";
        exit_code = 1;
        break;
      }
    }

    if (port == 0)
      port = 33060;
    if (host.empty())
      host = "localhost";

    if (uri.empty())
    {
      uri = user;
      if (!uri.empty()) {
        if (!password.empty()) {
          uri.append(":").append(password);
        }
        uri.append("@");
      }
      uri.append(host);
      {
        char buf[10];
        my_snprintf(buf, sizeof(buf), ":%i", port);
        uri.append(buf);
      }
      if (!schema.empty())
        uri.append("/").append(schema);
    }
  }
};


std::vector<Block_processor_ptr> create_block_processors(Connection_manager *cm)
{
  std::vector<Block_processor_ptr> result;

  result.push_back(boost::make_shared<Sql_block_processor>(cm));
  result.push_back(boost::make_shared<Single_command_processor>(cm));
  result.push_back(boost::make_shared<Snd_message_block_processor>(cm));

  return result;
}


static int process_client_input_on_session(const My_command_line_options &options, std::istream &input)
{
  Connection_manager cm(options.uri, options.ssl);
  int r = 1;

  try
  {
    std::vector<Block_processor_ptr> eaters;

    cm.connect_default(options.cap_expired_password);
    eaters = create_block_processors(&cm);
    r = process_client_input(input, eaters);
    cm.close_active(true);
  }
  catch (mysqlx::Error &error)
  {
    std::cerr << "ERROR: " << error.what() << " (code " << error.error() << ")\n";
    std::cerr << "not ok\n";
    return 1;
  }

  if (r == 0)
    std::cerr << "ok\n";
  else
    std::cerr << "not ok\n";

  return r;
}


static int process_client_input_no_auth(const My_command_line_options &options, std::istream &input)
{
  Connection_manager cm(options.uri, options.ssl);
  int r = 1;

  try
  {
    std::vector<Block_processor_ptr> eaters;

    cm.active()->set_closed();
    eaters = create_block_processors(&cm);
    r = process_client_input(input, eaters);
  }
  catch (mysqlx::Error &error)
  {
    std::cerr << "ERROR: " << error.what() << " (code " << error.error() << ")\n";
    std::cerr << "not ok\n";
    return 1;
  }

  if (r == 0)
   std::cerr << "ok\n";
  else
   std::cerr << "not ok\n";

  return r;
}


static int normalize_client_input(const My_command_line_options &options, std::istream &input)
{
  std::vector<Block_processor_ptr> eaters = create_block_processors(NULL);

  return process_client_input(input, eaters, true);
}

typedef int (*Program_mode)(const My_command_line_options &, std::istream &input);

static std::istream &get_input(My_command_line_options &opt, std::ifstream &file)
{
  if (opt.has_file)
  {
    file.open(opt.run_file.c_str());

    if (!file.is_open())
    {
      std::cerr << "ERROR: Could not open file " << opt.run_file << "\n";
      opt.exit_code = 1;
    }

    return file;
  }

  return std::cin;
}

static Program_mode get_mode_function(const My_command_line_options &opt)
{
  switch(opt.run_mode)
  {
  case My_command_line_options::RunTestWithoutAuth:
    return process_client_input_no_auth;

  case My_command_line_options::Normalize:
    return normalize_client_input;

  case My_command_line_options::RunTest:
  default:
    return process_client_input_on_session;
  }
}

int main(int argc, char **argv)
{
  OPT_expect_error = new Expected_error();
  std::ifstream fs;
  My_command_line_options options(argc, argv);

  std::istream &input = get_input(options, fs);
  Program_mode  mode  = get_mode_function(options);

  
  if (options.exit_code != 0)
    return options.exit_code;

  try
  {
    return mode(options, input);
  }
  catch (std::exception &e)
  {
    std::cerr << "ERROR: " << e.what() << "\n";
    return 1;
  }
}


#include "mysqlx_all_msgs.h"

#ifdef _MSC_VER
#  pragma pop_macro("ERROR")
#endif

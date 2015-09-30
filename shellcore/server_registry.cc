/*
* Copyright (c) 2014, 2015 Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/server_registry.h"
#include "uuid_gen.h"
#include "myjson/myjson.h"
#include "myjson/mutable_myjson.h"
#include "rapidjson/document.h"
#include "utils_file.h"
#include "utils_json.h"

#include <cstring>
#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <boost/random.hpp>
#pragma GCC diagnostic pop
#include <boost/algorithm/string.hpp>
//#include <boost/function_output_iterator.hpp>
//#include <boost/lambda/lambda.hpp>
//#include <boost/bind.hpp>
//#include <boost/phoenix/function/adapt_callable.hpp>
//#include <boost/spirit/include/phoenix.hpp>

#ifndef WIN32
#  include <sys/file.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <errno.h>
#  include <cstring>
#else
#  include <windows.h>
#endif

#include "my_global.h"

using namespace shcore;

// Extracted from header "my_aes.h"

/** Supported AES cipher/block mode combos */
enum my_aes_opmode
{
  my_aes_128_ecb,
  my_aes_192_ecb,
  my_aes_256_ecb,
  my_aes_128_cbc,
  my_aes_192_cbc,
  my_aes_256_cbc
};

extern "C" int my_aes_get_size(uint32 source_length, enum my_aes_opmode mode);

extern "C" int my_aes_encrypt(const unsigned char *source, uint32 source_length,
  unsigned char *dest,
  const unsigned char *key, uint32 key_length,
enum my_aes_opmode mode, const unsigned char *iv);

extern "C" int my_aes_decrypt(const unsigned char *source, uint32 source_length,
  unsigned char *dest,
  const unsigned char *key, uint32 key_length,
enum my_aes_opmode mode, const unsigned char *iv);

Connection_options::Keywords_table Connection_options::_keywords_table;

std::string Server_registry::_file_path = ".mysql_server_registry.cnf";

void Connection_options::Keywords_table::validate_options_mandatory_included(Connection_options& options)
{
  Connection_options::iterator myend = options.end();
  for (int i = 0; i < MAX_KEYWORDS; ++i)
  {
    if (_is_optional[i]) continue;
    Connection_options::iterator it = options.find(_keywords[i]);
    if (it == myend)
      throw std::runtime_error((boost::format("The connection option %s is mandatory") % _keywords[i]).str());
  }
}

int Server_registry::encrypt_buffer(const char *plain, int plain_len, char cipher[], const char *my_key)
{
  int aes_len;

  aes_len = my_aes_get_size(plain_len, my_aes_128_ecb);
  if (my_aes_encrypt((const unsigned char *)plain, plain_len,
    (unsigned char *)cipher,
    (const unsigned char *)my_key, 36,
    my_aes_128_ecb, NULL) == aes_len)
    return (aes_len);

  return -1;
}

int Server_registry::decrypt_buffer(const char *cipher, int cipher_len, char plain[], const char *my_key)
{
  int aes_length;

  if ((aes_length = my_aes_decrypt((const unsigned char *)cipher, cipher_len,
    (unsigned char *)plain,
    (const unsigned char *)my_key,
    36,
    my_aes_128_ecb, NULL)) > 0)
    return aes_length;

  return -1;
}

Server_registry::Server_registry()
{
  get_user_config_path();

  std::string path = shcore::get_user_config_path();
  path += Server_registry::_file_path;
  _filename = path;

  _filename_lock = _filename + ".lock";

  init();
}

Server_registry::Server_registry(const std::string& data_source_file)
{
  _filename = data_source_file;
  _filename_lock = _filename + ".lock";
  init();
}

void Server_registry::init()
{
  // Use a random seed for UUIDs
  std::time_t now = std::time(NULL);
  boost::uniform_int<> dist(INT_MIN, INT_MAX);
  boost::mt19937 gen;
  boost::variate_generator<boost::mt19937 &, boost::uniform_int<> > vargen(gen, dist);
  gen.seed(now);
  init_uuid(vargen());

  _lock = new Lock_file(_filename_lock);
  try
  {
    //load_file_rapidjson();
    load_file();
    delete _lock;
  }
  catch (...)
  {
    if (_lock) delete _lock;
    throw;
  }
}

/*
void Server_registry::load_file_rapidjson()
{
  const char *c_filename = _filename.c_str();
  std::ifstream *iff = new std::ifstream(c_filename, std::ios::in | std::ios::binary);
  int nerrno = errno;
  if (!iff && nerrno == ENOENT)
  {
    std::ofstream of(c_filename);
    of.close();
    delete iff;
    iff = new std::ifstream(c_filename, std::ios::in | std::ios::binary);
  }
  if (iff)
  {
    std::string s;
    iff->seekg(0, iff->end);
    int pos = iff->tellg();
    if (pos != -1)
      s.resize(static_cast<std::string::size_type>(pos));
    iff->seekg(0, std::ios::beg);
    iff->read(&(s[0]), s.size());
    iff->close();
    delete iff;

    if (s.empty()) return;

    rapidjson::Document doc;
    doc.Parse(s.c_str());

    for (rapidjson::Value::ConstValueIterator it = doc.Begin(); it != doc.End(); ++it)
    {
      if (!it->IsObject())
        throw std::runtime_error((boost::format("The server registry at %s does not have the right format.") % c_filename).str());

      rapidjson::Value::ConstMemberIterator val = it->FindMember("uuid");
      const std::string& cs_uuid = val->value.GetString();
      add_connection_options(cs_uuid, "");

      for (rapidjson::Value::ConstMemberIterator it2 = it->MemberBegin(); it2 != it->MemberEnd(); ++it2)
      {
        if (it2->name == "password")
        {
          char decipher[4096];
          const std::string& password = it2->value.GetString();
          int len = password.size();
          int decipher_len = Server_registry::decrypt_buffer(password.c_str(), len, decipher, cs_uuid.c_str());
          if (decipher_len == -1)
            throw std::runtime_error("Error decrypting data");
          decipher[decipher_len] = '\0';
          const std::string s_decipher(static_cast<const char *>(decipher));
          set_value(cs_uuid, "password", s_decipher);
        }
        else
        {
          set_value(cs_uuid, it2->name.GetString(), it2->value.GetString());
        }
      }
    }
  }
  else
  {
    nerrno = errno;
    std::string errmsj = (boost::format("Cannot open file %s: %s") % _filename % std::strerror(nerrno)).str();
    throw std::runtime_error(errmsj);
  }
}
*/

void Server_registry::load_file()
{
  const char *c_filename = _filename.c_str();
  std::ifstream *iff = new std::ifstream(c_filename, std::ios::in | std::ios::binary);
  int nerrno = errno;
  if (!iff && nerrno == ENOENT)
  {
    std::ofstream of(c_filename);
    of.close();
    delete iff;
    iff = new std::ifstream(c_filename, std::ios::in | std::ios::binary);
  }
  if (iff)
  {
    std::string s;
    iff->seekg(0, iff->end);
    int pos = iff->tellg();
    if (pos != -1)
      s.resize(static_cast<std::string::size_type>(pos));
    iff->seekg(0, std::ios::beg);
    iff->read(&(s[0]), s.size());
    iff->close();
    delete iff;

    if (s.empty()) return;

    myjson::MYJSON *myjs = new myjson::MYJSON(s);
    myjson::MYJSON myjsDoc;
    myjson::MYJSON myjsDoc2;

    for (myjson::MYJSON::iterator it = myjs->begin(); it != myjs->end(); ++it)
    {
      if (!myjs->get_document(it.key(), myjsDoc2))
        throw std::runtime_error((boost::format("Missing document for key %s") % it.key()).str());
      if (!myjsDoc2.get_document("config", myjsDoc))
        throw std::runtime_error((boost::format("Missing config section in configuration with app %s") % it.key()).str());
      myjson::MYJSON::iterator myend = myjsDoc.end();
      
      std::string cs_uuid;
      myjsDoc.get_text("uuid", cs_uuid);
      std::string app;
      myjsDoc2.get_text("app", app);

      add_connection_options(cs_uuid,"", app);

      for (myjson::MYJSON::iterator it2 = myjsDoc.begin(); it2 != myend; ++it2)
      {
        if (std::strcmp(it2.key(), "password") == 0)
        {
          char decipher[4096];
          std::string password;
          myjsDoc.get_text("password", password);
          int len = password.size();
          int decipher_len = Server_registry::decrypt_buffer(password.c_str(), len, decipher, cs_uuid.c_str());
          if (decipher_len == -1)
            throw std::runtime_error("Error decrypting data");
          decipher[decipher_len] = '\0';
          const std::string s_key(it2.key());
          const std::string s_decipher(static_cast<const char *>(decipher));
          set_value(cs_uuid, s_key, s_decipher);
        }
        else
        {
          set_value(cs_uuid, it2.key(), it2.data());
        }
      }
    }
  }
  else
  {
    nerrno = errno;
    std::string errmsj = (boost::format("Cannot open file %s: %s") % _filename % std::strerror(nerrno)).str();
    throw std::runtime_error(errmsj);
  }
}

Server_registry::~Server_registry()
{
  end_uuid();
}

std::string Server_registry::get_new_uuid()
{
  uuid_type uuid;
  generate_uuid(uuid);

  // TODO: rewrite to simulate an sticky std::setw using boost functors

  std::stringstream str;
  str << std::hex << std::noshowbase << std::setfill('0') << std::setw(2);
  //*
  str << (int)uuid[0] << std::setw(2) << (int)uuid[1] << std::setw(2) << (int)uuid[2] << std::setw(2) << (int)uuid[3];
  str << "-" << std::setw(2) << (int)uuid[4] << std::setw(2) << (int)uuid[5];
  str << "-" << std::setw(2) << (int)uuid[6] << std::setw(2) << (int)uuid[7];
  str << "-" << std::setw(2) << (int)uuid[8] << std::setw(2) << (int)uuid[9];
  str << "-" << std::setw(2) << (int)uuid[10] << std::setw(2) << (int)uuid[11]
    << std::setw(2) << (int)uuid[12] << std::setw(2) << (int)uuid[13]
    << std::setw(2) << (int)uuid[14] << std::setw(2) << (int)uuid[15];
  //*/

  //std::copy(uuid, uuid + 15, boost::make_function_output_iterator( boost::lambda::var(str) << std::setw(2) << (int)(boost::lambda::_1) ));

  return str.str();
}

Connection_options& Server_registry::add_connection_options(const std::string& options)
{
  Connection_options cs(options);
  cs._uuid = Server_registry::get_new_uuid();
  std::string uuid = cs._uuid;
  _connections[uuid] = cs;
  Connection_options& result = _connections[uuid];
  _connections_by_name[cs.get_name()] = &result;
  return result;
}

Connection_options& Server_registry::add_connection_options(const std::string& uuid, const std::string& options)
{
  Connection_options cs(options);
  cs._uuid = uuid;
  _connections[uuid] = cs;
  Connection_options& result = _connections[uuid];
  _connections_by_name[cs.get_name()] = &result;
  return result;
}

Connection_options& Server_registry::add_connection_options(const std::string& uuid, const std::string& options, const std::string& name)
{
  Connection_options cs(options);
  cs._uuid = uuid;
  cs.set_name(name);
  _connections[uuid] = cs;
  Connection_options& result = _connections[uuid];
  _connections_by_name[name] = &result;
  return result;
}

void Server_registry::remove_connection_options(Connection_options &options)
{
  _connections.erase(options._uuid);
  _connections.erase(options.get_name());
}

Connection_options& Server_registry::get_connection_by_name(const std::string& name)
{
  if (_connections_by_name.find(name) == _connections_by_name.end())
    throw std::runtime_error((boost::format("Connection not found for app name: %s") % name).str());
  return *_connections_by_name[name];
}

Connection_options& Server_registry::get_connection_options(const std::string &uuid)
{
  connections_map_t::iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());    
  return it->second;
}

std::string Server_registry::get_name(const std::string &uuid) const
{
  connections_map_t::const_iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  const Connection_options& cs = it->second;
  return cs.get_name();
}

std::string Server_registry::get_server(const std::string &uuid) const
{
  connections_map_t::const_iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  const Connection_options& cs = it->second;
  return cs.get_server();
}

std::string Server_registry::get_user(const std::string &uuid) const
{
  connections_map_t::const_iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  const Connection_options& cs = it->second;
  return cs.get_user();
}

std::string Server_registry::get_port(const std::string &uuid) const
{
  connections_map_t::const_iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  const Connection_options& cs = it->second;
  return cs.get_port();
}

std::string Server_registry::get_password(const std::string &uuid) const
{
  connections_map_t::const_iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  const Connection_options& cs = it->second;
  return cs.get_password();
}

std::string Server_registry::get_protocol(const std::string &uuid) const
{
  connections_map_t::const_iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  const Connection_options& cs = it->second;
  return cs.get_protocol();
}

std::string Server_registry::get_value(const std::string &uuid, const std::string &name) const
{
  connections_map_t::const_iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  const Connection_options& cs = it->second;
  return cs.get_value(name);
}

void Server_registry::set_connection_options(const std::string &uuid, const Connection_options &conn_str)
{
  _connections[uuid] = conn_str;
  Connection_options& result = _connections[uuid];
  _connections_by_name[conn_str.get_name()] = &result;
}

void Server_registry::set_name(const std::string &uuid, const std::string &name)
{
  connections_map_t::iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  Connection_options& cs = it->second;
  cs.set_name(name);
}

void Server_registry::set_server(const std::string &uuid, const std::string &server)
{
  connections_map_t::iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  Connection_options& cs = it->second;
  cs.set_server(server);
}

void Server_registry::set_user(const std::string &uuid, const std::string &user)
{
  connections_map_t::iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  Connection_options& cs = it->second;
  cs.set_user(user);
}

void Server_registry::set_port(const std::string &uuid, const std::string &port)
{
  connections_map_t::iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  Connection_options& cs = it->second;
  cs.set_port(port);
}

void Server_registry::set_password(const std::string &uuid, const std::string &password)
{
  connections_map_t::iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  Connection_options& cs = it->second;
  cs.set_password(password);
}

void Server_registry::set_protocol(const std::string &uuid, const std::string &protocol)
{
  connections_map_t::iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  Connection_options& cs = it->second;
  cs.set_protocol(protocol);
}

void Server_registry::set_value(const std::string &uuid, const std::string &name, const std::string &value)
{
  connections_map_t::iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  Connection_options& cs = it->second;
  cs.set_value(name, value);
}

/*
void Server_registry::merge_rapidjson()
{
  shcore::JSON_dumper dumper(true);
 
  dumper.start_array();
  std::map<std::string, Connection_options>::iterator myend = _connections.end();
  for (std::map<std::string, Connection_options>::iterator it = _connections.begin(); it != myend; ++it)
  {
    dumper.start_object();
    bool uuid_checked = false;
    Connection_options& cs = it->second;
    Connection_options::iterator myend2 = cs.end();
    for (Connection_options::iterator it2 = cs.begin(); it2 != myend2; ++it2)
    {
      if (it2->first == "uuid")
      {
        uuid_checked = true;
        dumper.append_string(it2->first, it2->second);
      }
      else if (it2->first == "password")
      {
        // encrypt password
        char cipher[4096];
        std::string uuid = cs.get_uuid();
        int cipher_len = Server_registry::encrypt_buffer(it2->second.c_str(), it2->second.size(), cipher, uuid.c_str());
        if (cipher_len == -1)
          throw std::runtime_error("Error encrypting data.");
        cipher[cipher_len] = '\0';
        std::string s_cipher(static_cast<const char *>(cipher), cipher_len);
        dumper.append_string(it2->first, s_cipher);
      }
      else
      {
        dumper.append_string(it2->first, it2->second);
      }
    }
    if (!uuid_checked)
    {
      std::string uuid = cs.get_uuid();
      dumper.append_string("uuid", uuid);
    }

    dumper.end_object();
  }
  dumper.end_array();

  std::string fulljson = dumper.str();
  Lock_file lock2(_filename_lock);
  std::ofstream of(_filename.c_str(), std::ios::trunc | std::ios::binary);
  of.write(fulljson.c_str(), fulljson.size());
  of.flush();
}
*/

void Server_registry::merge()
{
  myjson::Mutable_MYJSON myfile;
  myjson::Mutable_MYJSON *myjs;

  std::map<std::string, Connection_options>::iterator myend = _connections.end();
  for (std::map<std::string, Connection_options>::iterator it = _connections.begin(); it != myend; ++it)
  {
    Connection_options& cs = it->second;
    bool uuid_checked = false;

    myjs = new myjson::Mutable_MYJSON();
    myjs->append_value("app", cs.get_name().c_str());
    myjson::Mutable_MYJSON *myjs2 = new myjson::Mutable_MYJSON();

    Connection_options::iterator myend2 = cs.end();
    for (Connection_options::iterator it2 = cs.begin(); it2 != myend2; ++it2)
    {
      if (it2->first == "app") continue;
      if (it2->first == "uuid")
      {
        uuid_checked = true;
        myjs2->append_value(it2->first.c_str(), it2->second.c_str());
      }
      else if (it2->first == "password")
      {
        // encrypt password
        char cipher[4096];
        std::string uuid = cs.get_uuid();
        int cipher_len = Server_registry::encrypt_buffer(it2->second.c_str(), it2->second.size(), cipher, uuid.c_str());
        if (cipher_len == -1)
          throw std::runtime_error("Error encrypting data.");
        cipher[cipher_len] = '\0';
        myjs2->append_value(it2->first.c_str(), static_cast<const char *>(cipher), cipher_len);
      }
      else
      {
        myjs2->append_value(it2->first.c_str(), it2->second.c_str(), it2->second.size());
      }
    }
    if (!uuid_checked)
    {
      std::string uuid = cs.get_uuid();
      myjs2->append_value("uuid", uuid.c_str(), uuid.size());
    }
    myjs2->done();
    myjs->append_document("config", *myjs2);

    myjs->done();
    myfile.append_document(it->first.c_str(), *myjs);
    delete myjs2;
    delete myjs;
  }

  myfile.done();
  std::string fulljson = myfile.as_json(true);
  char *json_data = (char *)std::malloc(sizeof(char)* (fulljson.size() + 1));
  std::memcpy(json_data, fulljson.c_str(), fulljson.size());

  Lock_file lock2(_filename_lock);
  std::ofstream of(_filename.c_str(), std::ios::trunc | std::ios::binary);
  of.write(json_data, fulljson.size());
  of.flush();
  std::free(json_data);
}

Connection_options::Connection_options(std::string options) : _connection_options(options)
{
  if (!options.empty())
    parse();
}

Connection_options::Connection_options() : _uuid(Server_registry::get_new_uuid())
{
}

std::string Connection_options::get_uuid() const
{
  return _uuid;
}

std::string Connection_options::get_connection_options() const
{
  return _connection_options;
}

std::string Connection_options::get_name() const
{
  const_iterator it = find(_keywords_table[(int)App]);
  if (it == end())
    throw std::runtime_error((boost::format("'app' attribute not found in connection.")).str());
  return it->second;
}

std::string Connection_options::get_server() const
{
  const_iterator it = find(_keywords_table[Server]);
  if (it == end())
    throw std::runtime_error((boost::format("'server' attribute not found in connection.")).str());
  return it->second;
}

std::string Connection_options::get_user() const
{
  const_iterator it = find(_keywords_table[User]);
  if (it == end())
    throw std::runtime_error((boost::format("'user' attribute not found in connection.")).str());
  return at(_keywords_table[User]);
}

std::string Connection_options::get_port() const
{
  const_iterator it = find(_keywords_table[Port]);
  if (it == end())
    throw std::runtime_error((boost::format("'port' attribute not found in connection.")).str());
  return at(_keywords_table[Port]);
}

std::string Connection_options::get_password() const
{
  const_iterator it = find(_keywords_table[Password]);
  if (it == end())
    throw std::runtime_error((boost::format("'port' attribute not found in connection.")).str());
  return at(_keywords_table[Password]);
}

std::string Connection_options::get_protocol() const
{
  const_iterator it = find(_keywords_table[Protocol]);
  if (it == end())
    throw std::runtime_error((boost::format("'protocol' attribute not found in connection.")).str());
  return at(_keywords_table[Protocol]);
}

std::string Connection_options::get_value(const std::string &name) const
{
  const_iterator it = find(name);
  if (it == end())
    throw std::runtime_error((boost::format("'%s' attribute not found in connection.") % name).str());
  return it->second;
}

std::string Connection_options::get_value_if_exists(const std::string &name) const
{
  const_iterator it = find(name);
  if (it != end())
    return it->second;
  return "";
}

void Connection_options::set_connection_options(const std::string &conn_str)
{
  _connection_options = conn_str;
  clear();
  parse();
}

void Connection_options::set_name(const std::string &name)
{
  operator[](_keywords_table[App]) = name;
}

void Connection_options::set_server(const std::string &server)
{
  operator[](_keywords_table[Server]) = server;
}

void Connection_options::set_user(const std::string &user)
{
  operator[](_keywords_table[User]) = user;
}

void Connection_options::set_port(const std::string &port)
{
  operator[](_keywords_table[Port]) = boost::lexical_cast<std::string>(port);
}

void Connection_options::set_password(const std::string &password)
{
  operator[](_keywords_table[Password]) = password;
}

void Connection_options::set_protocol(const std::string &protocol)
{
  operator[](_keywords_table[Protocol]) = protocol;
}

void Connection_options::set_value(const std::string& name, const std::string& value)
{
  operator[](name) = value;
}

void Connection_options::parse()
{
  std::string::size_type p_start = 0, p_i;

  do
  {
    p_i = _connection_options.find(';', p_start);

    std::string buf = _connection_options.substr(p_start, p_i - p_start);
    std::string::size_type p_eq = buf.find('=');
    std::string key = buf.substr(0, p_eq);
    std::string value = buf.substr(p_eq + 1);

    boost::trim(key);
    boost::trim(value);

    if (p_eq != std::string::npos)
      operator[](key) = value;

    p_start = p_i + 1;

    if (p_i == std::string::npos) break;
  } while (true);

  // Validate that all essential fields have been included in the connections options.
  _keywords_table.validate_options_mandatory_included(*this);
}

#ifdef _WIN32
Lock_file::Lock_file(const std::string &path) throw (std::invalid_argument, std::runtime_error, file_locked_error)
: path(path), handle(0)
{
  if (path.empty()) throw std::invalid_argument("invalid path");

  handle = CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
    OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (handle == INVALID_HANDLE_VALUE)
  {
    if (GetLastError() == ERROR_SHARING_VIOLATION)
      throw file_locked_error("File already locked");
    throw std::runtime_error((boost::format("Error creating lock file (%i)") % GetLastError()).str());
  }

  char buffer[32];
  sprintf(buffer, "%i", GetCurrentProcessId());
  DWORD bytes_written;

  if (!WriteFile(handle, buffer, strlen(buffer), &bytes_written, NULL) || bytes_written != strlen(buffer))
  {
    CloseHandle(handle);
    DeleteFileA(path.c_str());
    throw std::runtime_error("Could not write to lock file");
  }
}

Lock_file::~Lock_file()
{
  if (handle)
    CloseHandle(handle);
  DeleteFileA(path.c_str());
}

Lock_file::Status Lock_file::check(const std::string &path)
{
  // open the file and see if it's locked
  HANDLE h = CreateFileA(path.c_str(),
    GENERIC_WRITE,
    FILE_SHARE_WRITE | FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (h == INVALID_HANDLE_VALUE)
  {
    switch (GetLastError())
    {
      case ERROR_SHARING_VIOLATION:
        // if file cannot be opened for writing, it is locked...
        // so open it for reading to check the owner process id written in it
        h = CreateFileA(path.c_str(),
          GENERIC_READ,
          FILE_SHARE_WRITE | FILE_SHARE_READ,
          NULL,
          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h != INVALID_HANDLE_VALUE)
        {
          char buffer[32];
          DWORD bytes_read;
          if (ReadFile(h, buffer, sizeof(buffer), &bytes_read, NULL))
          {
            CloseHandle(h);
            buffer[bytes_read] = 0;
            if (atoi(buffer) == GetCurrentProcessId())
              return LockedSelf;
            return LockedOther;
          }
          CloseHandle(h);
          return LockedOther;
        }
        // if the file is locked for read, assume its locked by some unrelated process
        // since this class never locks it for read
        return LockedOther;
        //throw std::runtime_error(strfmt("Could not read process id from lock file (%i)", GetLastError());
        break;
      case ERROR_FILE_NOT_FOUND:
        return NotLocked;
      case ERROR_PATH_NOT_FOUND:
        throw std::invalid_argument("Invalid path");
      default:
        throw std::runtime_error((boost::format("Could not open lock file (%i)") % GetLastError()).str());
    }
  }
  else
  {
    CloseHandle(h);
    // if the file could be opened with DENY_WRITE, it means no-one is locking it
    return NotLocked;
  }
}
#else
Lock_file::Lock_file(const std::string &apath) throw (std::invalid_argument, std::runtime_error, file_locked_error)
: path(apath)
{
  if (path.empty()) throw std::invalid_argument("invalid path");

  fd = open(path.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd < 0)
  {
    // this could mean lock exists, that it's a dangling file or that it's currently being locked by some other process/thread
    // we just go on and try to lock the file if the file already exists
    if (errno == ENOENT || errno == ENOTDIR)
      throw std::invalid_argument("invalid path");

    throw std::runtime_error((boost::format("%s creating lock file") % std::strerror(errno)).str());
  }
  if (flock(fd, LOCK_EX | LOCK_NB) < 0)
  {
    close(fd);
    fd = -1;
    if (errno == EWOULDBLOCK)
      throw file_locked_error("file already locked");

    throw std::runtime_error((boost::format("%s while locking file") % std::strerror(errno)).str());
  }

  if (ftruncate(fd, 0) < 0 )
  {
    close(fd);
    fd = -1;
    throw std::runtime_error((boost::format("%s while truncating file") % std::strerror(errno)).str());
  }

  char pid[32];
  snprintf(pid, sizeof(pid), "%i", getpid());
  if (write(fd, pid, strlen(pid) + 1) < 0)
  {
    close(fd);
    throw std::runtime_error((boost::format("%s while locking file") % std::strerror(errno)).str());
  }
}

Lock_file::~Lock_file()
{
  if (fd >= 0)
    close(fd);
  unlink(path.c_str());
}

Lock_file::Status Lock_file::check(const std::string &path)
{
  int fd = open(path.c_str(), O_RDONLY);
  if (fd < 0)
    return NotLocked;

  if (flock(fd, LOCK_EX | LOCK_NB) < 0)
  {
    char pid[32];
    // couldn't lock file, check if we own it ourselves
    int c = read(fd, pid, sizeof(pid) - 1);
    close(fd);
    if (c < 0)
      return LockedOther;
    pid[c] = 0;
    if (atoi(pid) != getpid())
      return LockedOther;
    return LockedSelf;
  }
  else // nobody holds a lock on the file, so this is a leftover
  {
    flock(fd, LOCK_UN);
    close(fd);
    return NotLocked;
  }
}
#endif

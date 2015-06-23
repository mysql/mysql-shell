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
#include "my_global.h"
#include "utils_file.h"

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
  // Use a random seed for UUIDs
  std::time_t now = std::time(NULL);
  boost::uniform_int<> dist(INT_MIN, INT_MAX);
  boost::mt19937 gen;
  boost::variate_generator<boost::mt19937 &, boost::uniform_int<> > vargen(gen, dist);
  gen.seed(now);
  init_uuid(vargen());

  get_user_config_path();

  std::string path = shcore::get_user_config_path();
  path += Server_registry::_file_path;
  _filename = path;

  _filename_lock = _filename + ".lock";

  _lock = new Lock_file(_filename_lock);
  try {
    load_file();
    delete _lock;
  }
  catch (...)
  {
    if (_lock) delete _lock;
    throw;
  }
}

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
    myjson::MYJSON::iterator myend = myjs->end();
    for (myjson::MYJSON::iterator it = myjs->begin(); it != myend; ++it)
    {
      myjson::MYJSON constr;
      myjs->get_document(it.key(), constr);

      std::string cs_uuid;
      constr.get_text("uuid", cs_uuid);

      add_connection_options(cs_uuid, "");

      myjson::MYJSON::iterator myend2 = constr.end();
      for (myjson::MYJSON::iterator it2 = constr.begin(); it2 != myend2; ++it2)
      {
        if (std::strcmp(it2.key(), "password") == 0)
        {
          char decipher[4096];
          std::string password;
          constr.get_text("password", password);
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

Connection_options& Server_registry::add_connection_options(const std::string options)
{
  Connection_options cs(options);
  cs._uuid = Server_registry::get_new_uuid();
  std::string uuid = cs._uuid;
  _connections[uuid] = cs;
  return _connections[uuid];
  return _connections[""];
}

Connection_options& Server_registry::add_connection_options(const std::string uuid, const std::string options)
{
  Connection_options cs(options);
  cs._uuid = uuid;
  _connections[uuid] = cs;
  return _connections[uuid];
}

void Server_registry::remove_connection_options(Connection_options &options)
{
  _connections.erase(options._uuid);
}

Connection_options& Server_registry::get_connection_options(const std::string &uuid)
{
  const std::string my_uuid = uuid;
  return _connections[my_uuid];
}

std::string Server_registry::get_name(const std::string &uuid) const
{
  const Connection_options& cs = _connections.at(uuid);
  return cs.get_name();
}

std::string Server_registry::get_server(const std::string &uuid) const
{
  const Connection_options& cs = _connections.at(uuid);
  return cs.get_server();
}

std::string Server_registry::get_user(const std::string &uuid) const
{
  const Connection_options& cs = _connections.at(uuid);
  return cs.get_user();
}

std::string Server_registry::get_port(const std::string &uuid) const
{
  const Connection_options& cs = _connections.at(uuid);
  return cs.get_port();
}

std::string Server_registry::get_password(const std::string &uuid) const
{
  const Connection_options& cs = _connections.at(uuid);
  return cs.get_password();
}

std::string Server_registry::get_protocol(const std::string &uuid) const
{
  const Connection_options& cs = _connections.at(uuid);
  return cs.get_protocol();
}

std::string Server_registry::get_value(const std::string &uuid, const std::string &name) const
{
  const Connection_options& cs = _connections.at(uuid);
  return cs.get_value(name);
}

void Server_registry::set_connection_options(const std::string &uuid, const Connection_options &conn_str)
{
  _connections[uuid] = conn_str;
}

void Server_registry::set_name(const std::string &uuid, const std::string &name)
{
  Connection_options& cs = _connections[uuid];
  cs.set_name(name);
}

void Server_registry::set_server(const std::string &uuid, const std::string &server)
{
  Connection_options& cs = _connections[uuid];
  cs.set_server(server);
}

void Server_registry::set_user(const std::string &uuid, const std::string &user)
{
  Connection_options& cs = _connections[uuid];
  cs.set_user(user);
}

void Server_registry::set_port(const std::string &uuid, const std::string &port)
{
  Connection_options& cs = _connections[uuid];
  cs.set_port(port);
}

void Server_registry::set_password(const std::string &uuid, const std::string &password)
{
  Connection_options& cs = _connections[uuid];
  cs.set_password(password);
}

void Server_registry::set_protocol(const std::string &uuid, const std::string &protocol)
{
  Connection_options& cs = _connections[uuid];
  cs.set_protocol(protocol);
}

void Server_registry::set_value(const std::string &uuid, const std::string &name, const std::string &value)
{
  Connection_options& cs = _connections[uuid];
  cs.set_value(name, value);
}

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

    Connection_options::iterator myend2 = cs.end();
    for (Connection_options::iterator it2 = cs.begin(); it2 != myend2; ++it2)
    {
      if (it2->first == "uuid")
      {
        uuid_checked = true;
        myjs->append_value(it2->first.c_str(), it2->second.c_str());
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
        myjs->append_value(it2->first.c_str(), static_cast<const char *>(cipher), cipher_len);
      }
      else
      {
        myjs->append_value(it2->first.c_str(), it2->second.c_str(), it2->second.size());
      }
    }
    if (!uuid_checked)
    {
      std::string uuid = cs.get_uuid();
      myjs->append_value("uuid", uuid.c_str(), uuid.size());
    }

    myjs->done();
    myfile.append_document(it->first.c_str(), *myjs);
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

Connection_options::Connection_options(std::string options) : _Connection_options(options)
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

std::string Connection_options::get_Connection_options() const
{
  return _Connection_options;
}

std::string Connection_options::get_name() const
{
  return at(_keywords_table[(int)Name]);
}

std::string Connection_options::get_server() const
{
  return at(_keywords_table[Server]);
}

std::string Connection_options::get_user() const
{
  return at(_keywords_table[User]);
}

std::string Connection_options::get_port() const
{
  //return boost::lexical_cast<int>(at(_keywords_table[Port]));
  return at(_keywords_table[Port]);
}

std::string Connection_options::get_password() const
{
  return at(_keywords_table[Password]);
}

std::string Connection_options::get_protocol() const
{
  return at(_keywords_table[Protocol]);
}

std::string Connection_options::get_value(const std::string &name) const
{
  return at(name);
}

void Connection_options::set_connection_options(const std::string &conn_str)
{
  _Connection_options = conn_str;
  clear();
  parse();
}

void Connection_options::set_name(const std::string &name)
{
  operator[](_keywords_table[Name]) = name;
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
    p_i = _Connection_options.find(';', p_start);

    std::string buf = _Connection_options.substr(p_start, p_i - p_start);
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

  ftruncate(fd, 0);

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
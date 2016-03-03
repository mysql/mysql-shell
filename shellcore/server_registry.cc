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
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "uuid_gen.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"
#include "logger/logger.h"
#include "utils_json.h"
#include "shellcore/types.h"

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

Connection_options::Keywords_table Connection_options::_keywords_table;

void Connection_options::Keywords_table::validate_options_mandatory_included(Connection_options& options)
{
  Connection_options::iterator myend = options.end();
  for (int i = 0; i < MAX_KEYWORDS; ++i)
  {
    if (_is_optional[i]) continue;
    Connection_options::iterator it = options.find(_keywords[i]);
    if (it == myend)
      throw shcore::Exception::argument_error((boost::format("The connection option %s is mandatory") % _keywords[i]).str());
  }
}

const int Server_registry::_version(2);

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
}

void Server_registry::load()
{
  Lock_file _lock(_filename_lock);

  const char *c_filename = _filename.c_str();
  boost::shared_ptr<std::ifstream> iff(new std::ifstream(c_filename, std::ios::in | std::ios::binary));
  int nerrno = errno;
  if (iff->fail() && nerrno == ENOENT)
  {
    std::ofstream of(c_filename);
    of.close();
    iff.reset(new std::ifstream(c_filename, std::ios::in | std::ios::binary));
  }
  
  if (!iff->fail())
  {
    std::string s;
    iff->seekg(0, iff->end);
    int pos = iff->tellg();
    if (pos != -1)
      s.resize(static_cast<std::string::size_type>(pos));
    iff->seekg(0, std::ios::beg);
    iff->read(&(s[0]), s.size());
    iff->close();

    if (s.empty()) return;

    rapidjson::Document doc;
    doc.Parse(s.c_str());
    if (doc.GetParseError() != rapidjson::kParseErrorNone)
    {
      log_error("Server Registry: Error when parsing the file '%s', error: %s at %lu", c_filename, rapidjson::GetParseError_En(doc.GetParseError()), 
        doc.GetErrorOffset());
      return;
    }

    int i = 1;
    rapidjson::Value::ConstValueIterator it = doc.Begin();
    bool version_ok = false;
    // try to read the version
    if (it->IsObject())
    {
      rapidjson::Value::ConstMemberIterator it_ver = it->FindMember("version");
      if (it_ver != it->MemberEnd())
      {
        try 
        {
          if (Server_registry::_version == boost::lexical_cast<int>(it_ver->value.GetString()))
          {
            version_ok = true;
            it++;
          }
        }
        catch (const boost::bad_lexical_cast& e)
        {
          log_error("Server Registry: Version is not parsable as an int for file '%s'", c_filename);
          return;
        }
      }
    }
    if (!version_ok)
    {
      log_error("Server Registry: Version does not exist for file '%s' (maybe file is for an old XShell version?) or doesn't match the expected version '%d'", c_filename, Server_registry::_version);
      return;
    }

    for (; it != doc.End(); ++it, ++i)
    {
      if (!it->IsObject())
      {
        log_error("Server Registry: The entry number '%d' at file '%s' does not have the right format", i, c_filename);
        continue;
      }

      const std::string& app = it->FindMember("app")->value.GetString();
      const rapidjson::Value& config = it->FindMember("config")->value;

      rapidjson::Value::ConstMemberIterator val = config.FindMember("uuid");
      const std::string& cs_uuid = val->value.GetString();
      add_connection_options(cs_uuid, app, "", false, true);

      for (rapidjson::Value::ConstMemberIterator it2 = config.MemberBegin(); it2 != config.MemberEnd(); ++it2)
      {
        set_value(cs_uuid, it2->name.GetString(), it2->value.GetString());
      }
    }
  }
  else
  {
    nerrno = errno;
    log_error("Cannot open file %s: %s", _filename.c_str(), std::strerror(nerrno));
  }
}

Server_registry::~Server_registry()
{
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

Connection_options& Server_registry::add_connection_options(const std::string& uuid, const std::string& name, const std::string& options, bool overwrite, bool placeholder)
{
  if (!shcore::is_valid_identifier(name))
    throw shcore::Exception::argument_error((boost::format("The app name '%s' is not a valid identifier") % name).str());
  else if (!overwrite && _connections_by_name.find(name) != _connections_by_name.end())
    throw shcore::Exception::argument_error((boost::format("The app name '%s' already exists") % name).str());

  Connection_options cs(options, placeholder);
  cs._uuid = uuid;
  cs.set_name(name);

  if (_connections_by_name.find(name) != _connections_by_name.end())
  {
    std::string old_uuid = _connections_by_name[name]->get_uuid();
    _connections_by_name.erase(name);
    _connections.erase(old_uuid);
  }

  _connections[uuid] = cs;
  Connection_options& result = _connections[uuid];
  _connections_by_name[name] = &result;
  return result;
}

Connection_options& Server_registry::add_connection_options(const std::string &name, const std::string &options, bool overwrite)
{
  std::string uuid = get_new_uuid();
  return add_connection_options(uuid, name, options, overwrite, false);
}

Connection_options& Server_registry::update_connection_options(const std::string &name, const std::string &options)
{
  if (_connections_by_name.find(name) == _connections_by_name.end())
    throw shcore::Exception::argument_error((boost::format("The app name '%s' does not exist") % name).str());

  std::string uuid = _connections_by_name[name]->get_uuid();
  return add_connection_options(uuid, name, options, true, false);
}

void Server_registry::remove_connection_options(Connection_options &options)
{
  std::string name = options.get_name();
  _connections.erase(options._uuid);
  _connections.erase(name);
}

Connection_options& Server_registry::get_connection_options(const std::string& name)
{
  std::map<std::string, Connection_options*>::iterator it = _connections_by_name.find(name);
  if (it == _connections_by_name.end())
    throw std::runtime_error((boost::format("Connection not found for app name: %s") % name).str());
  return *(it->second);
}

void Server_registry::set_value(const std::string &uuid, const std::string &name, const std::string &value)
{
  connections_map_t::iterator it = _connections.find(uuid);
  if (it == _connections.end())
  throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  Connection_options& cs = it->second;
  int idkey = Connection_options::get_keyword_id(name); 
  if (idkey == (int)App)
  {
    if (!shcore::is_valid_identifier(name))
      throw std::runtime_error((boost::format("The app name '%s' is not a valid identifier") % name).str());
    std::string cur_name = cs.get_name();
    if (cur_name != value)
    {
      _connections_by_name.erase(cs.get_name());
      _connections_by_name[name] = &cs;
    }
  }
  if (idkey != -1)
    cs.set_value(name, value);
  else
    log_warning("The key '%s' was not stored because is not valid", name);
}

void Server_registry::merge()
{
  rapidjson::Document doc;
  doc.SetArray();
  rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
  // Add versioning to the file
  rapidjson::Value my_obj_version(rapidjson::kObjectType);
  std::string version = boost::lexical_cast<std::string>(Server_registry::_version);
  rapidjson::GenericStringRef<char> version_label("version");
  rapidjson::Value version_value(version.c_str(), version.size());
  my_obj_version.AddMember(version_label, version_value, allocator);
  doc.PushBack(my_obj_version, allocator);
  // Add the connections
  std::map<std::string, Connection_options>::iterator myend = _connections.end();
  for (std::map<std::string, Connection_options>::iterator it = _connections.begin(); it != myend; ++it)
  {
    bool uuid_checked = false;
    rapidjson::Value my_obj(rapidjson::kObjectType);
    rapidjson::Value my_obj2(rapidjson::kObjectType);
    Connection_options& cs = it->second;
    const std::string app_name = cs.get_name();
    Connection_options::iterator myend2 = cs.end();
    for (Connection_options::iterator it2 = cs.begin(); it2 != myend2; ++it2)
    {
      if (it2->first == "app") continue;

      rapidjson::GenericStringRef<char> name_label(it2->first.c_str(), it2->first.size());
      rapidjson::Value item_value(it2->second.c_str(), it2->second.size(), allocator);
      if (it2->first == "uuid")
      {
        uuid_checked = true;
        my_obj.AddMember(name_label, item_value, allocator);
      }
      else
      {
        my_obj.AddMember(name_label, item_value, allocator);
      }
    }
    if (!uuid_checked)
    {
      std::string uuid = cs.get_uuid();
      rapidjson::GenericStringRef<char> uuid_label("uuid", 4);
      rapidjson::Value uuid_data(uuid.c_str(), uuid.size(), allocator);
      my_obj.AddMember(uuid_label, uuid_data, allocator);
    }
    rapidjson::GenericStringRef<char> app_label("app", 3);
    rapidjson::Value app_data(app_name.c_str(), app_name.size(), allocator);
    my_obj2.AddMember(app_label, app_data, allocator);
    rapidjson::GenericStringRef<char> config_label("config", 6);
    my_obj2.AddMember(config_label, my_obj, allocator);
    doc.PushBack(my_obj2, allocator);
  }
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  doc.Accept(writer);

  // dump into file the JSON contents
  const char *fulljson = strbuf.GetString();
  Lock_file lock2(_filename_lock);
  std::ofstream of(_filename.c_str(), std::ios::trunc | std::ios::binary);
  of.write(fulljson, strbuf.GetSize());
  of.flush();
}

void Server_registry::set_keyword_value(const std::string &uuid, Connection_keywords key, const std::string& value)
{
  connections_map_t::iterator it = _connections.find(uuid);
  if (it == _connections.end())
    throw std::runtime_error((boost::format("Connection not found for uuid: %s") % uuid).str());
  Connection_options& cs = it->second;
  cs.set_keyword_value(key, value);
}

Connection_options::Connection_options(std::string options, bool placeholder) : _connection_options(options)
{
  if (!placeholder)
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

std::string Connection_options::get_value(const std::string &name) const
{
  const_iterator it = find(name);
  if (it == end())
    throw std::runtime_error((boost::format("'%s' attribute not found in connection") % name).str());
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

void Connection_options::set_keyword_value(Connection_keywords key, const std::string& value)
{
  operator[](_keywords_table[key]) = value;
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

  if (ftruncate(fd, 0) < 0)
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
    int c = read(fd, pid, sizeof(pid)-1);
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
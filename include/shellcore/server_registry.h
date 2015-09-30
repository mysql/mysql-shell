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

#ifndef _SERVER_REGISTRY_H_
#define _SERVER_REGISTRY_H_

#include "common.h"

#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  undef UNICODE
#  include <windows.h>
#endif

#include <string>
#include <map>
#include <stdexcept>

namespace shcore {
  enum Connection_protocol
  {
    Legacy,
    Instance_manager,
    Fabric,
    X_server
  };

  enum ConnectionKeywords
  {
    App = 0,
    Server = 1,
    User = 2,
    Port = 3,
    Password = 4,
    Protocol = 5,
    Schema = 6
  };

  class SHCORE_PUBLIC Connection_options
  {
  public:
    // Parses the connection string and fills a map with the individual key value pairs.
    Connection_options(const std::string options);
    Connection_options();

    // generates a connection string.
    std::string get_uuid() const;
    std::string get_connection_options() const;
    std::string get_value(const std::string &name) const;

    std::string get_value_if_exists(const std::string &name) const;
    std::string get_name() const    { return get_keyword_value(App); }
    std::string get_server() const  { return get_keyword_value(Server); }
    std::string get_user() const    { return get_keyword_value(User); }
    std::string get_port() const    { return get_keyword_value(Port); }
    std::string get_password() const{ return get_keyword_value(Password); }
    std::string get_protocol() const{ return get_keyword_value(Protocol); }
    std::string get_schema() const  { return get_keyword_value(Schema); }

    void set_connection_options(const std::string &conn_str);
    void set_value(const std::string& name, const std::string& value);
    void set_name(const std::string &name)        { set_keyword_value(App, name); }
    void set_server(const std::string &server)    { set_keyword_value(Server, server); }
    void set_user(const std::string &user)        { set_keyword_value(User, user); }
    void set_port(const std::string &port)        { set_keyword_value(Port, port); }
    void set_password(const std::string &password){ set_keyword_value(Password, password); }
    void set_protocol(const std::string &protocol){ set_keyword_value(Protocol, protocol); }
    void set_schema(const std::string &schema)    { set_keyword_value(Schema, schema); }

    typedef std::map<std::string, std::string> map_t;
    typedef map_t::iterator iterator;
    typedef map_t::const_iterator const_iterator;
    typedef map_t::mapped_type mapped_type;
    typedef map_t::key_type key_type;

    map_t::iterator begin() { return _data.begin(); }
    map_t::iterator end() { return _data.end(); }
    map_t::const_iterator begin() const { return _data.begin(); }
    map_t::const_iterator end() const { return _data.end(); }
    size_t size() const { return _data.size(); }
    void clear() { return _data.clear(); }
    mapped_type& at(key_type& key) { return _data.at(key); }
    const mapped_type& at(const key_type& key) const { return _data.at(key); }
    mapped_type& operator[](const key_type& key) { return _data.operator[](key); }
    iterator find(const key_type& key) { return _data.find(key); }
    const_iterator find(const key_type& key) const { return _data.find(key); }

  private:
    std::string get_keyword_value(ConnectionKeywords key) const { return get_value_if_exists(_keywords_table[key]); }
    void set_keyword_value(ConnectionKeywords key, const std::string& value);

    map_t _data;

    struct Keywords_table
    {
    private:
      enum { MAX_KEYWORDS = 7 };
      std::string _keywords[MAX_KEYWORDS];
      typedef std::map<std::string, ConnectionKeywords> keywords_to_int_map;
      keywords_to_int_map _keywords_to_int;
      bool _is_optional[MAX_KEYWORDS];
      inline void init_keyword(std::string name, int idx, bool optional)
      {
        _keywords[idx] = name;
        _is_optional[idx] = optional;
        _keywords_to_int[name] = (ConnectionKeywords)idx;
      }

    public:
      Keywords_table()
      {
        // For now, all properties are optional (can be provided in a mix of server registry connection string & shell connection string).
        init_keyword("app", App, true);
        init_keyword("server", Server, true);
        init_keyword("user", User, true);
        init_keyword("port", Port, true);
        init_keyword("password", Password, true);
        init_keyword("protocol", Protocol, true);
        init_keyword("schema", Schema, true);
      }

      std::string operator[](const int idx)
      {
        return _keywords[idx];
      }

      void validate_options_mandatory_included(Connection_options& options);
    };

    static struct Keywords_table _keywords_table;

    std::string _connection_options;
    std::string _uuid;
    Connection_protocol _protocol;

    friend class Server_registry;
    void parse();
  };

  struct Lock_file;

  class SHCORE_PUBLIC Server_registry
  {
  public:
    // opens the connection registry for a given operating system user
    // the ctor reads, loads & closes the file.
    Server_registry(const std::string& data_source_file);

    ~Server_registry();

    //void load_file_rapidjson();
    void load_file();

    Connection_options& add_connection_options(const std::string &options);
    Connection_options& add_connection_options(const std::string &uuid, const std::string &options);
    void remove_connection_options(Connection_options &options);

    // gets a connection by name (app attribute)
    Connection_options& get_connection_by_name(const std::string& name);
    Connection_options& get_connection_options(const std::string& uuid);
    std::string get_name(const std::string &uuid) const     { return get_keyword_value(uuid, App); }
    std::string get_server(const std::string &uuid) const   { return get_keyword_value(uuid, Server); }
    std::string get_user(const std::string &uuid) const     { return get_keyword_value(uuid, User); }
    std::string get_port(const std::string &uuid) const     { return get_keyword_value(uuid, Port); }
    std::string get_password(const std::string &uuid) const { return get_keyword_value(uuid, Password); }
    std::string get_protocol(const std::string &uuid) const { return get_keyword_value(uuid, Protocol); }
    std::string get_schema(const std::string &uuid) const   { return get_keyword_value(uuid, Schema); }
    std::string get_value(const std::string &uuid, const std::string &name) const;

    void set_connection_options(const std::string &uuid, const Connection_options &conn_str);
    void set_name(const std::string &uuid, const std::string &name)         { set_keyword_value(uuid, App, name); }
    void set_server(const std::string &uuid, const std::string &server)     { set_keyword_value(uuid, Server, server); }
    void set_user(const std::string &uuid, const std::string &user)         { set_keyword_value(uuid, User, user); }
    void set_port(const std::string &uuid, const std::string &port)         { set_keyword_value(uuid, Port, port); }
    void set_password(const std::string &uuid, const std::string &password) { set_keyword_value(uuid, Password, password); }
    void set_protocol(const std::string &uuid, const std::string &protocol) { set_keyword_value(uuid, Protocol, protocol); }
    void set_schema(const std::string &uuid, const std::string &schema)     { set_keyword_value(uuid, Schema, schema); }
    void set_value(const std::string &uuid, const std::string &name, const std::string &value);

    //void merge_rapidjson();
    void merge();

    static std::string get_new_uuid();

    std::map<std::string, Connection_options>::iterator begin() { return _connections.begin(); }
    std::map<std::string, Connection_options>::iterator end() { return _connections.end(); }

    std::map<std::string, Connection_options>::const_iterator begin() const { return _connections.begin(); }
    std::map<std::string, Connection_options>::const_iterator end() const { return _connections.end(); }

  private:
    // Connection strings indexed by uuid
    typedef std::map<std::string, Connection_options> connections_map_t;
    std::map<std::string, Connection_options> _connections;
    std::map<std::string, Connection_options*> _connections_by_name;
    std::string _filename;
    std::string _filename_lock;
    Lock_file *_lock;

    void init();
    Connection_options& add_connection_options(const std::string& uuid, const std::string& options, const std::string& name);
    static int encrypt_buffer(const char *plain, int plain_len, char cipher[], const char *my_key);
    static int decrypt_buffer(const char *cipher, int cipher_len, char plain[], const char *my_key);

    std::string get_keyword_value(const std::string &uuid, ConnectionKeywords key) const;
    void set_keyword_value(const std::string &uuid, ConnectionKeywords key, const std::string& value);

    static std::string _file_path;
  };

  class SHCORE_PUBLIC file_locked_error : public std::runtime_error
  {
  public:
    file_locked_error(const std::string &msg) : std::runtime_error(msg)
    {}
  };

  struct SHCORE_PUBLIC Lock_file
  {
#ifdef _WIN32
#pragma warning(disable: 4251) // DLL interface required for std::string member.
#pragma warning(disable: 4290) // C++ exception specification ignored.
    HANDLE handle;
#else
    int fd;
#endif

    std::string path;
    enum Status
    {
      LockedSelf, // lock file exists and is the process itself
      LockedOther, // lock file exists and its owner is running
      NotLocked,
    };

    Lock_file(const std::string &path) throw (std::invalid_argument, std::runtime_error, file_locked_error);
    ~Lock_file();
#undef check // there's a #define check in osx
    static Status check(const std::string &path);
  };
}

#endif

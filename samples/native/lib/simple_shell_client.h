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

#ifndef SIMPLE_SHELL_CLIENT_H_
#define SIMPLE_SHELL_CLIENT_H_

#ifdef _WIN32
# if _DLL
#  ifdef SHELL_CLIENT_NATIVE_EXPORTS
#   define SHELL_CLIENT_NATIVE_PUBLIC __declspec(dllexport)
#  else
#   define SHELL_CLIENT_NATIVE_PUBLIC __declspec(dllimport)
#  endif
# else
#  define SHELL_CLIENT_NATIVE_PUBLIC
# endif
#else
# define SHELL_CLIENT_NATIVE_PUBLIC
#endif

#include <string>
#include <vector>
#include <stdexcept>
#include <map>

#include "shellcore/types.h"
#include "shellcore/shell_core.h"
#include "modules/base_session.h"
#include "shellcore/lang_base.h"

class Result_set;
class Result_set_metadata;

class SHELL_CLIENT_NATIVE_PUBLIC Simple_shell_client
{
public:
  Simple_shell_client();
  virtual ~Simple_shell_client();
  // Makes a connection, throws an std::runtime_error in case of error.
  void make_connection(const std::string& connstr);
  void switch_mode(shcore::Shell_core::Mode mode);
  boost::shared_ptr<Result_set> execute(const std::string query);
protected:
  virtual void print(const char *text);
  virtual void print_error(const char *text);
  virtual bool input(const char *text, std::string &ret);
  virtual bool password(const char *text, std::string &ret);
  virtual void source(const char* module);
private:
  shcore::Interpreter_delegate _delegate;
  boost::shared_ptr<mysh::ShellBaseSession> _session;
  boost::shared_ptr<shcore::Shell_core> _shell;
  boost::shared_ptr<Result_set> _last_result;

  static void deleg_print(void *self, const char *text);
  static void deleg_print_error(void *self, const char *text);
  static bool deleg_input(void *self, const char *text, std::string &ret);
  static bool deleg_password(void *self, const char *text, std::string &ret);
  static void deleg_source(void *self, const char *module);

  shcore::Value connect_session(const shcore::Argument_list &args);
  boost::shared_ptr<Result_set> process_line(const std::string &line);
  void process_result(shcore::Value result);
  bool do_shell_command(const std::string &line);
  bool connect(const std::string &uri);
  boost::shared_ptr<std::vector<Result_set_metadata> > populate_metadata(shcore::Value& metadata);
};

class SHELL_CLIENT_NATIVE_PUBLIC Result_set_metadata
{
public:
  Result_set_metadata(const std::string catalog, const std::string db, const std::string table, const std::string orig_table, const std::string name, const std::string orig_name,
    int charset, int length, int type, int flags, int decimal) : _catalog(catalog), _db(db), _table(table), _orig_table(orig_table), _name(name), _orig_name(orig_name),
    _charset(charset), _length(length), _type(type), _flags(flags), _decimal(decimal) { }

  std::string get_catalog() const { return _catalog; }
  std::string get_db() const { return _db; }
  std::string get_table() const { return _table; }
  std::string get_orig_table() const { return _orig_table; }
  std::string get_name() const { return _name; }
  std::string get_orig_name() const { return _orig_name; }
  int get_charset() const { return _charset; }
  int get_length() const { return _length; }
  int get_type() const { return _type; }
  int get_flags() const { return _flags; }
  int get_decimal() const { return _decimal; }
private:
  std::string _catalog;
  std::string _db;
  std::string _table;
  std::string _orig_table;
  std::string _name;
  std::string _orig_name;
  int _charset;
  int _length;
  /* This is the enum enum_field_types from mysql_com.h  */
  int _type;
  int _flags;
  int _decimal;
};

class SHELL_CLIENT_NATIVE_PUBLIC Result_set
{
public:
  Result_set(int64_t affected_rows, int warning_count, std::string execution_time) : _affected_rows(affected_rows), _warning_count(warning_count), _execution_time(execution_time)
  {}
  virtual ~Result_set() { }
  int64_t get_affected_rows() { return _affected_rows; }
  int get_warning_count() { return _warning_count; }
  std::string get_execution_time() { return _execution_time; }
protected:
  int64_t _affected_rows;
  int _warning_count;
  std::string _execution_time;
};

class SHELL_CLIENT_NATIVE_PUBLIC Table_result_set : public Result_set
{
public:
  Table_result_set(boost::shared_ptr<std::vector<shcore::Value> > data, boost::shared_ptr<std::vector<Result_set_metadata> > metadata,
    int64_t affected_rows, int warning_count, std::string execution_time) : Result_set(affected_rows, warning_count, execution_time), _metadata(metadata), _data(data)
  {
  }
  boost::shared_ptr<std::vector<Result_set_metadata> > get_metadata() { return _metadata; }
  boost::shared_ptr<std::vector<shcore::Value> > get_data() { return _data; }
  virtual ~Table_result_set() { }
protected:
  boost::shared_ptr<std::vector<Result_set_metadata> > _metadata;
  boost::shared_ptr<std::vector<shcore::Value> > _data;
};

class SHELL_CLIENT_NATIVE_PUBLIC Document_result_set : public Result_set
{
public:
  Document_result_set(boost::shared_ptr<std::vector<shcore::Value> > data, int64_t affected_rows, int warning_count, std::string execution_time) :
    Result_set(affected_rows, warning_count, execution_time), _data(data)
  {
  }
  virtual ~Document_result_set() { }
  boost::shared_ptr<std::vector<shcore::Value> > get_data() { return _data; }
protected:
  boost::shared_ptr<std::vector<shcore::Value> > _data;
};

#endif

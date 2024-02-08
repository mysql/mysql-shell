/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __MYSH__UTILS_JSON__
#define __MYSH__UTILS_JSON__

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/writer.h>
#include <string>

#include "mysqlshdk/include/mysqlshdk_export.h"
#include "mysqlshdk/include/scripting/types.h"

namespace shcore {

size_t fmt_double(double d, char *buffer, size_t buffer_len);
/**
 * Raw JSON wrapper with custom conversion of the Double value using
 * the my_gcvt function ported from the server, which generates a string
 * value with the most number of significat digits and thus precision.
 */
template <typename T>
class My_writer : public rapidjson::Writer<T> {
 public:
  My_writer(T &outstream) : rapidjson::Writer<T>(outstream) {}
  bool Double(double data) {
    char buffer[32];
    size_t len = fmt_double(data, buffer, sizeof(buffer));

    rapidjson::Writer<T>::Prefix(rapidjson::kNumberType);
    return rapidjson::Writer<T>::WriteRawValue(buffer, len);
  }
};

/**
 * Pretty JSON wrapper with custom conversion of the Double value using
 * the my_gcvt function ported from the server, which generates a string
 * value with the most number of significat digits and thus precision.
 */
template <typename T>
class My_pretty_writer : public rapidjson::PrettyWriter<T> {
 public:
  My_pretty_writer(T &outstream) : rapidjson::PrettyWriter<T>(outstream) {}
  bool Double(double data) {
    char buffer[32];
    size_t len = fmt_double(data, buffer, sizeof(buffer));

    rapidjson::PrettyWriter<T>::PrettyPrefix(rapidjson::kNumberType);
    return rapidjson::PrettyWriter<T>::WriteRawValue(buffer, len);
  }
};

class SHCORE_PUBLIC Writer_base {
 protected:
  class SStream {
   public:
    using Ch = std::string::value_type;

    void Put(Ch c) { data += c; }
    void Flush() {}

    std::string data;
  };

  SStream _data;

 public:
  virtual ~Writer_base() noexcept = default;

  virtual void start_array() = 0;
  virtual void end_array() = 0;
  virtual void start_object() = 0;
  virtual void end_object() = 0;
  virtual void append_null() = 0;
  virtual void append_bool(bool data) = 0;
  virtual void append_int(int data) = 0;
  virtual void append_int64(int64_t data) = 0;
  virtual void append_uint(unsigned int data) = 0;
  virtual void append_uint64(uint64_t data) = 0;
  virtual void append_string(const std::string &data) = 0;
  virtual void append_string(const char *data, size_t length) = 0;
  virtual void append_float(double data) = 0;
  virtual void append_document(const rapidjson::Document &document) = 0;

 public:
  const std::string &str() const { return _data.data; }
};

class SHCORE_PUBLIC Raw_writer final : public Writer_base {
 public:
  Raw_writer() : _writer(_data){};

  virtual void start_array() { _writer.StartArray(); }
  virtual void end_array() { _writer.EndArray(); }
  virtual void start_object() { _writer.StartObject(); }
  virtual void end_object() { _writer.EndObject(); }

  virtual void append_null() { _writer.Null(); };
  virtual void append_bool(bool data) { _writer.Bool(data); };
  virtual void append_int(int data) { _writer.Int(data); };
  virtual void append_int64(int64_t data) { _writer.Int64(data); };
  virtual void append_uint(unsigned int data) { _writer.Uint(data); };
  virtual void append_uint64(uint64_t data) { _writer.Uint64(data); };
  virtual void append_string(const char *data, size_t length) {
    _writer.String(data, unsigned(length));
  };
  virtual void append_string(const std::string &data) {
    _writer.String(data.c_str(), unsigned(data.length()));
  };
  virtual void append_float(double data) { _writer.Double(data); };
  virtual void append_document(const rapidjson::Document &document) {
    document.Accept(_writer);
  };

 private:
  My_writer<SStream> _writer;
};

class SHCORE_PUBLIC Pretty_writer final : public Writer_base {
 public:
  Pretty_writer() : _writer(_data){};

  virtual void start_array() { _writer.StartArray(); }
  virtual void end_array() { _writer.EndArray(); }
  virtual void start_object() { _writer.StartObject(); }
  virtual void end_object() { _writer.EndObject(); }

  virtual void append_null() { _writer.Null(); }
  virtual void append_bool(bool data) { _writer.Bool(data); }
  virtual void append_int(int data) { _writer.Int(data); }
  virtual void append_int64(int64_t data) { _writer.Int64(data); }
  virtual void append_uint(unsigned int data) { _writer.Uint(data); }
  virtual void append_uint64(uint64_t data) { _writer.Uint64(data); }
  virtual void append_string(const char *data, size_t length) {
    _writer.String(data, unsigned(length));
  };
  virtual void append_string(const std::string &data) {
    _writer.String(data.c_str(), unsigned(data.length()));
  }
  virtual void append_float(double data) { _writer.Double(data); }

  virtual void append_document(const rapidjson::Document &document) {
    document.Accept(_writer);
  };

 private:
  My_pretty_writer<SStream> _writer;
};

class SHCORE_PUBLIC JSON_dumper final {
 public:
  explicit JSON_dumper(bool pprint = false, size_t binary_limit = 0);

  void start_array() {
    _deep_level++;
    _writer->start_array();
  }
  void end_array() {
    _deep_level--;
    _writer->end_array();
  }
  void start_object() {
    _deep_level++;
    _writer->start_object();
  }
  void end_object() {
    _deep_level--;
    _writer->end_object();
  }

  void append_value(const shcore::Value &value);
  void append_value(const std::string &key, const shcore::Value &value);
  void append(const shcore::Value &value);
  void append(const std::string &key, const shcore::Value &value);
  void append(const char *key, const shcore::Value &value);

  void append(const Dictionary_t &value);
  void append(const std::string &key, const Dictionary_t &value);
  void append(const char *key, const Dictionary_t &value);

  void append(const Array_t &value);
  void append(const std::string &key, const Array_t &value);
  void append(const char *key, const Array_t &value);

  void append_null() const;
  void append_null(const std::string &key) const;
  void append_null(const char *key) const;

  void append_bool(bool data) const;
  void append_bool(const std::string &key, bool data) const;
  void append(bool data) const;
  void append(const std::string &key, bool data) const;
  void append(const char *key, bool data) const;

  void append_int(int data) const;
  void append_int(const std::string &key, int data) const;
  void append(int data) const;
  void append(const std::string &key, int data) const;
  void append(const char *key, int data) const;

  void append_int64(int64_t data) const;
  void append_int64(const std::string &key, int64_t data) const;
  void append(int64_t data) const;
  void append(const std::string &key, int64_t data) const;
  void append(const char *key, int64_t data) const;

  void append_uint(unsigned int data) const;
  void append_uint(const std::string &key, unsigned int data) const;
  void append(unsigned int data) const;
  void append(const std::string &key, unsigned int data) const;
  void append(const char *key, unsigned int data) const;

  void append_uint64(uint64_t data) const;
  void append_uint64(const std::string &key, uint64_t data) const;
  void append(uint64_t data) const;
  void append(const std::string &key, uint64_t data) const;
  void append(const char *key, uint64_t data) const;

  void append_string(const std::string &data) const;
  void append_string(const char *data, size_t length) const;
  void append_string(const std::string &key, const std::string &data) const;
  void append(const std::string &data) const;
  inline void append(const char *data) const { append_string(data); }
  void append(const std::string &key, const std::string &data) const;
  void append(const char *key, const std::string &data) const;

  void append_float(double data) const;
  void append_float(const std::string &key, double data) const;
  void append(double data) const;
  void append(const std::string &key, double data) const;
  void append(const char *key, double data) const;

  void append_json(const std::string &data) const;

  int deep_level() const { return _deep_level; }

  const std::string &str() const { return _writer->str(); }

 private:
  int _deep_level{0};
  size_t _binary_limit{0};

  std::unique_ptr<Writer_base> _writer;
};
}  // namespace shcore
#endif /* defined(__MYSH__UTILS_JSON__) */

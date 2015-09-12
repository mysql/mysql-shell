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

#ifndef __MYSH__UTILS_JSON__
#define __MYSH__UTILS_JSON__

#include <string>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

#include "shellcore/common.h"

namespace shcore
{
  // This class is to wrap the Raw and Pretty writers from rapidjson since
  // they
  class SHCORE_PUBLIC Writer_base
  {
  protected:

    class SStream
    {
    public:
      void Put(char c)
      {
        data += c;
      }
      void Flush(){}

      std::string data;
    };

    SStream _data;

  public:
    virtual ~Writer_base(){}

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
    virtual void append_string(const std::string& data) = 0;
    virtual void append_float(double data) = 0;

  public:
    std::string str() { return _data.data; }
  };

  class SHCORE_PUBLIC Raw_writer :public Writer_base
  {
  public:
    Raw_writer() :_writer(_data){};
    virtual ~Raw_writer(){}

    virtual void start_array(){ _writer.StartArray(); }
    virtual void end_array() { _writer.EndArray(); }
    virtual void start_object() { _writer.StartObject(); }
    virtual void end_object() { _writer.EndObject(); }

    virtual void append_null(){ _writer.Null(); };
    virtual void append_bool(bool data) { _writer.Bool(data); };
    virtual void append_int(int data) { _writer.Int(data); };
    virtual void append_int64(int64_t data) { _writer.Int64(data); };
    virtual void append_uint(unsigned int data) { _writer.Uint(data); };
    virtual void append_uint64(uint64_t data) { _writer.Uint64(data); };
    virtual void append_string(const std::string& data) { _writer.String(data.c_str(), unsigned(data.length())); };
    virtual void append_float(double data) { _writer.Double(data); };

  private:
    rapidjson::Writer<SStream>_writer;
  };

  class SHCORE_PUBLIC Pretty_writer :public Writer_base
  {
  public:
    Pretty_writer() :_writer(_data){};
    virtual ~Pretty_writer(){}

    virtual void start_array(){ _writer.StartArray(); }
    virtual void end_array() { _writer.EndArray(); }
    virtual void start_object() { _writer.StartObject(); }
    virtual void end_object() { _writer.EndObject(); }

    virtual void append_null(){ _writer.Null(); }
    virtual void append_bool(bool data) { _writer.Bool(data); }
    virtual void append_int(int data) { _writer.Int(data); }
    virtual void append_int64(int64_t data) { _writer.Int64(data); }
    virtual void append_uint(unsigned int data) { _writer.Uint(data); }
    virtual void append_uint64(uint64_t data) { _writer.Uint64(data); }
    virtual void append_string(const std::string& data) { _writer.String(data.c_str(), unsigned(data.length())); }
    virtual void append_float(double data) { _writer.Double(data); }

  private:
    rapidjson::PrettyWriter<SStream>_writer;
  };

  struct Value;
  class SHCORE_PUBLIC JSON_dumper
  {
  public:
    JSON_dumper(bool pprint = false);
    virtual ~JSON_dumper();

    void start_array()  const { _writer->start_array(); }
    void end_array()    const { _writer->end_array(); }
    void start_object() const { _writer->start_object(); }
    void end_object()   const { _writer->end_object(); }

    void append_value(const Value &value) const;
    void append_value(const std::string& key, const Value &value)const;

    void append_null()const;
    void append_null(const std::string& key)const;

    void append_bool(bool data)const;
    void append_bool(const std::string& key, bool data)const;

    void append_int(int data)const;
    void append_int(const std::string& key, int data)const;

    void append_int64(int64_t data)const;
    void append_int64(const std::string& key, int64_t data)const;

    void append_uint(unsigned int data)const;
    void append_uint(const std::string& key, unsigned int data)const;

    void append_uint64(uint64_t data)const;
    void append_uint64(const std::string& key, uint64_t data)const;

    void append_string(const std::string& data)const;
    void append_string(const std::string& key, const std::string& data)const;

    void append_float(double data)const;
    void append_float(const std::string& key, double data)const;

    std::string str()
    {
      return _writer->str();
    }

  private:
    bool _pretty;

    Writer_base* _writer;
  };
}
#endif /* defined(__MYSH__UTILS_JSON__) */

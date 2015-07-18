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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef _MYSQLX_CONNECTOR_H_
#define _MYSQLX_CONNECTOR_H_

#include <stdexcept>
#include <vector>
#include <map>
#include <set>

#include "xdatetime.h"

#include <boost/enable_shared_from_this.hpp>

namespace Mysqlx
{
  namespace Sql
  {
    class Row;
  }
}

namespace google { namespace protobuf { class Message; } }

namespace mysqlx
{
  typedef google::protobuf::Message Message;

  bool parse_mysql_connstring(const std::string &connstring,
    std::string &protocol, std::string &user, std::string &password,
    std::string &host, int &port, std::string &sock,
    std::string &db, int &pwd_found);

  class Result;

  class Error : public std::runtime_error
  {
  public:
    Error(int error = 0, const std::string &message = "");
    virtual ~Error() BOOST_NOEXCEPT_OR_NOTHROW;
    int error() const { return _error; }

  private:
    std::string _message;
    int _error;
  };

  class AuthError : public std::runtime_error
  {
  public:
    AuthError(const std::string &message)
    : std::runtime_error(message) {}
    virtual ~AuthError() BOOST_NOEXCEPT_OR_NOTHROW;
  };

  class Schema;
  class Connection;

  class Session : public boost::enable_shared_from_this<Session>
  {
  public:
    Session();
    ~Session();
    Result *executeSql(const std::string &sql);

    boost::shared_ptr<Schema> getSchema(const std::string &name);

    Connection *connection() { return m_connection; }
  private:
    Connection *m_connection;
    std::map<std::string, boost::shared_ptr<Schema> > m_schemas;
  };
  typedef boost::shared_ptr<Session> SessionRef;

  SessionRef openSession(const std::string &uri, const std::string &pass);

  SessionRef openSession(const std::string &host, int port, const std::string &schema,
                         const std::string &user, const std::string &pass);

  enum FieldType
  {
    SINT,
    UINT,

    DOUBLE,
    FLOAT,

    BYTES,

    TIME,
    DATETIME,
    SET,
    ENUM,
    BIT,
    DECIMAL
  };

  struct ColumnMetadata
  {
    FieldType type;
    std::string name;
    std::string original_name;

    std::string table;
    std::string original_table;

    std::string schema;
    std::string catalog;

    std::string charset;

    uint32_t fractional_digits;

    uint32_t length;

    uint32_t flags;
    uint32_t content_type;
  };

  class Document
  {
  public:
    explicit Document(const std::string &doc);
    Document(const Document &doc);

    const std::string &str() const { return *m_data; }

  private:
    boost::shared_ptr<std::string> m_data;
  };

  class TableValue
  {
  public:
    enum Type
    {
      TInteger,
      TUInteger,
      TNull,
      TDouble,
      TFloat,
      TBool,
      TString,
      TOctets,
      TExpression,
    };

    TableValue(const TableValue &other)
    {
      m_type = other.m_type;
      m_value.b = other.m_value.b;
      m_value.d = other.m_value.d;
      m_value.f = other.m_value.f;
      m_value.i = other.m_value.i;
      m_value.ui = other.m_value.ui;

      if (m_type == TString || m_type == TOctets)
        m_value.s = new std::string(*other.m_value.s);
    }

    explicit TableValue(const std::string &s, Type type = TString)
    {
      m_type = type;
      m_value.s = new std::string(s);
    }

    explicit TableValue(int64_t n)
    {
      m_type = TInteger;
      m_value.i = n;
    }

    explicit TableValue(uint64_t n)
    {
      m_type = TUInteger;
      m_value.ui = n;
    }

    explicit TableValue(double n)
    {
      m_type = TDouble;
      m_value.d = n;
    }

    explicit TableValue(float n)
    {
      m_type = TFloat;
      m_value.f = n;
    }

    explicit TableValue(bool n)
    {
      m_type = TBool;
      m_value.b = n;
    }

    explicit TableValue()
    {
      m_type = TNull;
    }

    ~TableValue()
    {
      if (m_type == TString || m_type == TOctets || m_type == TExpression)
        delete m_value.s;
    }

    inline Type type() const { return m_type; }

    inline operator uint64_t ()
    {
      if (m_type != TUInteger)
        throw std::logic_error("type error");
      return m_value.ui;
    }

    inline operator int64_t ()
    {
      if (m_type != TInteger)
        throw std::logic_error("type error");
      return m_value.i;
    }

    inline operator double()
    {
      if (m_type != TDouble)
        throw std::logic_error("type error");
      return m_value.d;
    }

    inline operator float()
    {
      if (m_type != TFloat)
        throw std::logic_error("type error");
      return m_value.f;
    }

    inline operator bool()
    {
      if (m_type != TBool)
        throw std::logic_error("type error");
      return m_value.b;
    }

    inline operator const std::string & ()
    {
      if (m_type != TString && m_type != TOctets && m_type != TExpression)
        throw std::logic_error("type error");
      return *m_value.s;
    }

  private:
    Type m_type;
    union
    {
      std::string *s;
      int64_t i;
      uint64_t ui;
      double d;
      float f;
      bool b;
    } m_value;
  };

  class DocumentValue
  {
  public:
    enum Type
    {
      TString,
      TInteger,
      TFloat,
      TDocument,
      TExpression
    };

    DocumentValue(const DocumentValue &other)
    {
      m_type = other.m_type;
      m_value.f = other.m_value.f;
      m_value.i = other.m_value.i;

      if (m_type == TString || m_type == TExpression)
        m_value.s = new std::string(*other.m_value.s);

      if (m_type == TDocument)
        m_value.d = new Document(*other.m_value.d);
    }

    explicit DocumentValue(const std::string &s, bool expression = false)
    {
      m_type = expression ? TExpression : TString;
      m_value.s = new std::string(s);
    }

    explicit DocumentValue(int64_t n)
    {
      m_type = TInteger;
      m_value.i = n;
    }

    explicit DocumentValue(uint64_t n)
    {
      m_type = TInteger;
      m_value.i = n;
    }

    explicit DocumentValue(double n)
    {
      m_type = TFloat;
      m_value.f = n;
    }

    explicit DocumentValue(const Document &doc)
    {
      m_type = TDocument;
      m_value.d = new Document(doc);
    }

    ~DocumentValue()
    {
      if (m_type == TDocument)
        delete m_value.d;
      else if (m_type == TString || m_type == TExpression)
        delete m_value.s;
    }

    inline Type type() const { return m_type; }

    inline operator int64_t ()
    {
      if (m_type != TInteger)
        throw std::logic_error("type error");
      return m_value.i;
    }

    inline operator double()
    {
      if (m_type != TFloat)
        throw std::logic_error("type error");
      return m_value.f;
    }

    inline operator const std::string & ()
    {
      if (m_type != TString && m_type != TExpression)
        throw std::logic_error("type error");
      return *m_value.s;
    }

    inline operator const Document & ()
    {
      if (m_type != TDocument)
        throw std::logic_error("type error");
      return *m_value.d;
    }

  private:
    Type m_type;
    union
    {
      std::string *s;
      int64_t i;
      double f;
      Document *d;
    } m_value;
  };

  class Row
  {
  public:
    ~Row();

    bool isNullField(int field) const;
    int32_t sIntField(int field) const;
    uint32_t uIntField(int field) const;
    int64_t sInt64Field(int field) const;
    uint64_t uInt64Field(int field) const;
    uint64_t bitField(int field) const;
    std::string stringField(int field) const;
    std::string decimalField(int field) const;
    std::string setFieldStr(int field) const;
    std::set<std::string> setField(int field) const;
    std::string enumField(int field) const;
    const char *stringField(int field, size_t &rlength) const;
    float floatField(int field) const;
    double doubleField(int field) const;
    DateTime dateTimeField(int field) const;
    Time timeField(int field) const;

    int numFields() const;

  private:
    friend class Result;
    Row(boost::shared_ptr<std::vector<ColumnMetadata> > columns, Mysqlx::Sql::Row *data);

    void check_field(int field, FieldType type) const;

    boost::shared_ptr<std::vector<ColumnMetadata> > m_columns;
    Mysqlx::Sql::Row *m_data;
  };


  class Result
  {
  public:
    ~Result();

    boost::shared_ptr<std::vector<ColumnMetadata> > columnMetadata();
    int64_t lastInsertId() const { return m_last_insert_id; }
    int64_t affectedRows() const { return m_affected_rows; }

    bool ready();
    void wait();

    Row *next();
    bool nextResult();
    void discardData();

    struct Warning
    {
      std::string text;
      int code;
      bool is_note;
    };
    const std::vector<Warning> &getWarnings() const { return m_warnings; }
  private:
    Result();
    Result(const Result &o);
    Result(Connection *owner, bool expect_data);

    void read_metadata();
    std::auto_ptr<Row> read_row();
    void read_stmt_ok();

    void handle_notice(int32_t type, const std::string &data);

    int get_message_id();
    mysqlx::Message* pop_message();

    mysqlx::Message* current_message;
    int                            current_message_id;

    friend class Connection;
    Connection *m_owner;
    boost::shared_ptr<std::vector<ColumnMetadata> > m_columns;
    int64_t m_last_insert_id;
    int64_t m_affected_rows;

    std::vector<Warning> m_warnings;

    enum {
      ReadStmtOkI, // initial state
      ReadMetadataI, // initial state
      ReadMetadata,
      ReadRows,
      ReadStmtOk,
      ReadDone, // end
      ReadError // end
    } m_state;
  };
};

#endif

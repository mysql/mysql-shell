/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MYSQLSHDK_LIBS_UTILS_DOCUMENT_PARSER_H_
#define MYSQLSHDK_LIBS_UTILS_DOCUMENT_PARSER_H_

#include <string>
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/utils_buffered_input.h"

namespace shcore {

class invalid_json : public std::invalid_argument {
 public:
  invalid_json(const std::string &e, std::size_t offs)
      : std::invalid_argument(e), m_offset(offs) {
    m_msg += e;
    m_msg += " at offset ";
    m_msg += std::to_string(m_offset);
  }

  size_t offset() const { return m_offset; }

  const char *what() const noexcept override { return m_msg.c_str(); }

 private:
  size_t m_offset;    //< Byte offset from file beginning
  std::string m_msg;  //< Exception message
};

enum class Bson_type {
  NONE,
  OBJECT_ID,
  DATE,
  TIMESTAMP,
  INTEGER,
  LONG,
  DECIMAL,
  REGEX,
  BINARY
};

struct Document_reader_options {
  bool convert_bson_types = false;
  bool convert_bson_id = false;
  bool ignore_date = false;
  bool ignore_timestamp = false;
  bool ignore_regexp = false;
  bool ignore_regexp_options = true;
  bool ignore_binary = false;
  bool decimal_as_double = false;
  mysqlshdk::utils::nullable<std::string> extract_oid_time;

  bool ignore_type(Bson_type type) const;
};

/**
 * Loads JSON documents from a given Buffered_input
 */
class Document_reader {
 public:
  Document_reader(Buffered_input *input,
                  const shcore::Document_reader_options &options)
      : m_source(input), m_options(options){};

  virtual std::string next() = 0;
  bool eof() { return m_source->eof(); }

 protected:
  Buffered_input *m_source;
  const Document_reader_options &m_options;
};

/**
 * Loads JSON document from a Buffered_input where the content
 * is formatted as standard JSON documents
 */
class Json_reader : public Document_reader {
 public:
  Json_reader(Buffered_input *input,
              const shcore::Document_reader_options &options)
      : Document_reader(input, options) {}
  std::string next() override;
};

/**
 * Base class for standard JSON document generators.
 *
 * Childs of this class should be able to interpret the data coming from the
 * Buffered_input in order to create a standard JSON document.
 *
 * The Document_reader_options contains flags that affect the way the parsing
 * is done. Child classes should handle these flags whenever applicable.
 */
class Document_parser {
 public:
  virtual std::string parse() = 0;

  Document_parser(Buffered_input *input, const Document_reader_options &options,
                  size_t depth = 0, bool as_array = false,
                  const std::string context = "")
      : m_source(input),
        m_options(options),
        m_depth(depth),
        m_as_array(as_array),
        m_context(context) {}

 protected:
  Buffered_input *m_source;
  const Document_reader_options &m_options;
  size_t m_depth = 0;
  bool m_as_array = false;
  std::string m_context;
};

/**
 * Reads the next JSON document contained in the Buffered_input.
 *
 * The data in the Buffered_input is meant to be standard JSON documents.
 *
 * The Document_reader_options contains flags to indicate how specific
 * BSON data types should be loaded into the returned JSON document.
 */
class Json_document_parser : public Document_parser {
 public:
  Json_document_parser(Buffered_input *input,
                       const Document_reader_options &options, size_t depth = 0,
                       bool as_array = false, const std::string &context = "")
      : Document_parser(input, options, depth, as_array, context) {}

  /**
   * Parses a single JSON document from the Buffered_input.
   */
  std::string parse() override;

  struct Bson_token {
    Bson_token(char atype, const std::string &astring = "",
               std::string *string_ptr = nullptr, double *number = nullptr,
               size_t len = 0)
        : type(atype),
          value(astring),
          target(string_ptr),
          ntarget(number),
          length(len){};
    char type;
    std::string value = "";
    std::string *target = nullptr;
    double *ntarget = nullptr;
    size_t length = 0;
  };

 private:
  std::string *m_document;
  size_t m_document_start_offset = 0;

  // Helper variables, identify the position in m_document
  // for the last parsed attribute.
  size_t m_last_attribute_start = 0;
  size_t m_last_attribute_end = 0;

  void parse(std::string *document);

  void get_char(std::string *target = nullptr);
  void get_string(std::string *target, const std::string &context = "");
  void get_value(std::string *target);
  void get_whitespaces(std::string *target);

  // BSON Handling functions
  Bson_type get_bson_type();
  void parse_bson_document(Bson_type type);
  void get_bson_data(const std::vector<Bson_token> &tokens,
                     const std::string &context);
  void parse_bson_oid();
  void parse_bson_date();
  void parse_bson_timestamp();
  void parse_bson_decimal();
  void parse_bson_integer(Bson_type type);
  void parse_bson_regex();
  void parse_bson_binary();

  void throw_premature_end();
  void throw_invalid_json(const std::string &missing,
                          const std::string &context, std::size_t offset);
  void clear_document();
};

}  // namespace shcore
#endif  // MYSQLSHDK_LIBS_UTILS_DOCUMENT_PARSER_H_

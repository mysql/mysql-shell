/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/document_parser.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <limits>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <deque>
#include <mutex>
#include <string>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/shellcore/shell_console.h"

namespace shcore {
namespace {
std::string hexify(const std::string &data) {
  if (data.size() == 0) {
    return std::string{};
  }
  std::string s(3 * data.size(), 'x');

  std::string::iterator k = s.begin();

  for (const unsigned char i : data) {
    *k++ = "0123456789abcdef"[i >> 4];
    *k++ = "0123456789abcdef"[i & 0x0F];
    *k++ = ' ';
  }
  s.resize(3 * data.size() - 1);
  return s;
}
}  // namespace

const shcore::Option_pack_def<Document_reader_options>
    &Document_reader_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Document_reader_options>()
          .optional("convertBsonTypes",
                    &Document_reader_options::convert_bson_types)
          .optional("convertBsonOid", &Document_reader_options::convert_bson_id)
          .optional("extractOidTime",
                    &Document_reader_options::extract_oid_time)
          .optional("ignoreDate", &Document_reader_options::ignore_date)
          .optional("ignoreTimestamp",
                    &Document_reader_options::ignore_timestamp)
          .optional("ignoreBinary", &Document_reader_options::ignore_binary)
          .optional("ignoreRegex", &Document_reader_options::ignore_regexp)
          .optional("ignoreRegexOptions",
                    &Document_reader_options::ignore_regexp_options)
          .optional("decimalAsDouble",
                    &Document_reader_options::decimal_as_double)
          .on_done(&Document_reader_options::on_unpacked_options);

  return opts;
}

void Document_reader_options::on_unpacked_options() {
  // The default value for convert_bson_id is the value of convert_bson_types
  if (convert_bson_id.is_null()) convert_bson_id = convert_bson_types;

  if (!extract_oid_time.is_null() && !convert_bson_id.get_safe(false)) {
    throw shcore::Exception::argument_error(
        "The 'extractOidTime' option can not be used if 'convertBsonOid' is "
        "disabled.");
  }

  std::vector<std::string> used_options;

  if (!ignore_date.is_null()) used_options.push_back("ignoreDate");
  if (!ignore_timestamp.is_null()) used_options.push_back("ignoreTimestamp");
  if (!ignore_binary.is_null()) used_options.push_back("ignoreBinary");
  if (!ignore_regexp.is_null()) used_options.push_back("ignoreRegex");
  if (!ignore_regexp_options.is_null())
    used_options.push_back("ignoreRegexOptions");
  if (!decimal_as_double.is_null()) used_options.push_back("decimalAsDouble");

  if (!used_options.empty() && !convert_bson_types.get_safe(false)) {
    throw shcore::Exception::argument_error(shcore::str_format(
        "The following option%s can not be used if 'convertBsonTypes' is "
        "disabled: %s",
        used_options.size() > 1 ? "s" : "",
        shcore::str_join(used_options, ", ", [](const std::string &data) {
          return "'" + data + "'";
        }).c_str()));
  }

  if (ignore_regexp.get_safe(false) && ignore_regexp_options.get_safe(false)) {
    throw shcore::Exception::argument_error(
        "The 'ignoreRegex' and 'ignoreRegexOptions' options can't both be "
        "enabled");
  }

  if (!extract_oid_time.is_null() && (*extract_oid_time).empty()) {
    throw shcore::Exception::runtime_error(
        "Option 'extractOidTime' can not be empty.");
  }
}

bool Document_reader_options::ignore_type(Bson_type type) const {
  if (convert_bson_types.get_safe(false)) {
    switch (type) {
      case Bson_type::OBJECT_ID:
        return !convert_bson_id.get_safe(false);
      case Bson_type::DATE:
        return ignore_date.get_safe(false);
      case Bson_type::TIMESTAMP:
        return ignore_timestamp.get_safe(false);
      case Bson_type::REGEX:
        return ignore_regexp.get_safe(false);
      case Bson_type::BINARY:
        return ignore_binary.get_safe(false);
      case Bson_type::LONG:
      case Bson_type::INTEGER:
      case Bson_type::DECIMAL:
        return false;
      case Bson_type::NONE:
        break;
    }
  } else {
    return type == Bson_type::OBJECT_ID ? !convert_bson_id.get_safe(false)
                                        : true;
  }
  return true;
}

std::string Json_reader::next() {
  std::deque<char> context;
  m_source->skip_whitespaces();

  Json_document_parser parser(m_source, m_options);
  return parser.parse();
}

void Json_reader::parse_bom() {
  std::string header;
  header.reserve(4);
  for (int i = 0; i < 4; i++) {
    const auto c = m_source->peek();
    if (c == '{' || ::isspace(c) || m_source->eof()) {
      break;
    } else {
      header += m_source->get();
    }
  }

  if (header.size() == 0) {
    return;
  }

  if (std::string{'\xef', '\xbb', '\xbf'}.compare(header) == 0) {
    // nop
  } else if (std::string{'\x00', '\x00', '\xfe', '\xff'}.compare(header) == 0) {
    // utf-32 be
    throw std::runtime_error("UTF-32BE encoded document is not supported.");
  } else if (std::string{'\xff', '\xfe', '\x00', '\x00'}.compare(header) == 0) {
    // utf-32 le
    throw std::runtime_error("UTF-32LE encoded document is not supported.");
  } else if (std::string{'\xfe', '\xff'}.compare(header) == 0) {
    // utf-16 be
    throw std::runtime_error("UTF-16BE encoded document is not supported.");
  } else if (std::string{'\xff', '\xfe'}.compare(header) == 0) {
    // utf-16 le
    throw std::runtime_error("UTF-16LE encoded document is not supported.");
  } else {
    throw std::runtime_error("JSON document contains invalid bytes (" +
                             hexify(header) + ") at the begining of the file.");
  }
}

Document_parser::Document_parser(Buffered_input *input,
                                 const Document_reader_options &options,
                                 size_t depth, bool as_array,
                                 const std::string context)
    : m_source(input),
      m_options(options),
      m_depth(depth),
      m_as_array(as_array),
      m_context(context) {}

void Json_document_parser::throw_premature_end() {
  throw invalid_json("Premature end of input stream", m_source->offset());
}

void Json_document_parser::throw_invalid_json(const std::string &missing,
                                              const std::string &context,
                                              std::size_t offset) {
  std::string msg = "Unexpected data, expected to find ";
  msg += missing;
  if (!context.empty()) {
    msg += " " + context;
  }
  throw invalid_json(msg, offset);
}

void Json_document_parser::clear_document() {
  m_document->resize(m_document_start_offset);
}

Bson_type Json_document_parser::get_bson_type() {
  static std::map<std::string, Bson_type> types = {
      {"oid", Bson_type::OBJECT_ID},
      {"date", Bson_type::DATE},
      {"timestamp", Bson_type::TIMESTAMP},
      {"numberDecimal", Bson_type::DECIMAL},
      {"numberInt", Bson_type::INTEGER},
      {"numberLong", Bson_type::LONG},
      {"regex", Bson_type::REGEX},
      {"binary", Bson_type::BINARY}};

  try {
    std::string field =
        m_document->substr(m_last_attribute_start + 1,
                           m_last_attribute_end - m_last_attribute_start - 1);
    return types.at(field);
  } catch (const std::out_of_range &exception) {
    return Bson_type::NONE;
  }
}

std::string Json_document_parser::parse() {
  std::string document;
  parse(&document);
  return document;
}

/**
 * This function carries on the actual parsing of a document by appending new
 * data into the received document.
 *
 * Nested documents/arrays are parsed by creating a new instance of the class
 * and calling this function so the same output buffer is used everywhere.
 */
void Json_document_parser::parse(std::string *document) {
  m_document = document;
  m_document_start_offset = m_document->size();
  if (m_source->eof()) return;

  if (m_source->peek() != (m_as_array ? '[' : '{')) {
    std::string type = m_as_array ? "array" : "object";
    throw invalid_json("Input does not start with a JSON " + type,
                       m_source->offset());
  }

  // Appends the initial character
  (*m_document) += m_source->get();

  get_whitespaces(m_document);

  bool complete = false;

  // Tests for an empty object/array
  if (m_source->peek() == (m_as_array ? ']' : '}')) {
    (*m_document) += m_source->get();
    complete = true;
  }

  int field_count = 0;
  while (!complete && !m_source->eof()) {
    if (!m_as_array) {
      // Next data is an attribute which is quoted
      m_last_attribute_start = m_document->size() + 1;
      get_string(m_document);
      m_last_attribute_end = m_document->size() - 1;

      if (m_options.convert_bson_types.get_safe(false) ||
          m_options.convert_bson_id.get_safe(false)) {
        auto type = get_bson_type();
        // If the first field is a mongo special field
        // The original document is translated based on
        // options and the right data is returned
        if (!field_count && !m_options.ignore_type(type)) {
          parse_bson_document(type);
          return;
        }
      }

      get_whitespaces(m_document);

      if (m_source->eof()) throw_premature_end();

      if (m_source->peek() != ':')
        throw invalid_json(
            "Unexpected character, expected field/value separator ':'",
            m_source->offset());

      (*m_document) += m_source->get();

      get_whitespaces(m_document);

      field_count++;
    }
    get_value(m_document);

    get_whitespaces(m_document);

    // Only comma or closing is expected
    if (m_source->eof()) throw_premature_end();

    if (m_source->peek() == (m_as_array ? ']' : '}'))
      complete = true;
    else if (m_source->peek() != ',') {
      std::string type = m_as_array ? "value" : "field";
      throw invalid_json(
          "Unexpected character, expected " + type + " separator ','",
          m_source->offset());
    }

    // Consumes the , or the closing character
    (*m_document) += m_source->get();
    get_whitespaces(m_document);
  }

  if (!complete) throw_premature_end();
}

/**
 * BSON Data Types are self contained in embedded documents.
 *
 * This function executes calls the correct parsing routine for each
 * supported BSON Data Type.
 */
void Json_document_parser::parse_bson_document(Bson_type type) {
  switch (type) {
    case Bson_type::OBJECT_ID:
      parse_bson_oid();
      break;
    case Bson_type::DATE:
      parse_bson_date();
      break;
    case Bson_type::TIMESTAMP:
      parse_bson_timestamp();
      break;
    case Bson_type::LONG:
    case Bson_type::INTEGER:
      parse_bson_integer(type);
      break;
    case Bson_type::DECIMAL:
      parse_bson_decimal();
      break;
    case Bson_type::REGEX:
      parse_bson_regex();
      break;
    case Bson_type::BINARY:
      parse_bson_binary();
      break;
    case Bson_type::NONE:
      break;
  }
}

/**
 * Helper function, returns the correct missing label based on a
 * Bson_token definition
 */
std::string get_missing(const Json_document_parser::Bson_token &token) {
  switch (token.type) {
    case 'X':
      if (token.length == 0)
        return "a string with hexadecimal digits";
      else if (token.length == 1)
        return "a string with an hexadecimal digit";
      else
        return "a string with " + std::to_string(token.length) +
               " hexadecimal digits";
    case 'S':
      if (token.length == 0)
        return "a string";
      else if (token.length == 1)
        return "a string with one character";
      else
        return "a string with " + std::to_string(token.length) + " characters";
    case 'I':
      return "an integer string";
    case 'i':
      return "an integer";
    case 'N':
      return "a numeric string";
    case 'n':
      return "a number";
    case 'v':
      return "a value";
    case '{':
    case '}':
    case '[':
    case ']':
    case ',':
    case ':':
      std::string missing;
      missing = "'";
      missing.append(1, token.type);
      missing += "'";
      return missing;
  }

  assert(0);
  return "";
}

/**
 * This is a helper function used to read through the structure of a document
 * representing a BSON Data Type.
 *
 * @param target: The document where the read data will be appended
 * @param tokens: An array of Bson_tokens with information about how the
 * different tokens should be read as well as the types of validations to be
 *                done on them.
 *
 * A Bson_token contains the following elements:
 * - type: Indicates the type of token to be read.
 * - value: Used to trigger a verification so the read data matches this value.
 * - target: A recipient where the read data will be stored, if not given
 *           the read data is simply discarded.
 * - ntarget: Similar to target but for numeric values.
 * - length: Used to trigger a length validation, the read data should match
 *           this length.
 *
 * Following the available token types:
 *
 * a) Data coming as strings (Uppercase Types):
 * - S: to read a string
 * - N: to read a JSON number in a string
 * - I: to read an integer number in a string
 * - X: to read hexdecimal digits in a string
 *
 * b) Raw data (Lowercase Types):
 * - n: to read a JSON number
 * - i: to read an integer
 * - v: to read any value
 *
 * c) JSON separators
 * - These are: '{', '}', '[', ']', ',' and ':'
 *
 * Available Validations:
 *
 * The validations are not available for all the tokens, they were enabled
 * as needed, current validations include:
 *
 * - S and X values must match the token length if it's != 0
 * - S values must match the token value is it's != ""
 * - I and i values must be pure digits
 * - X values must be pure hexadecimal digits
 * - N and n values must be fully convertible to double
 */
void Json_document_parser::get_bson_data(const std::vector<Bson_token> &tokens,
                                         const std::string &context) {
  for (const auto &token : tokens) {
    std::string missing_data = get_missing(token);

    // Where the read value will be stored
    std::string *target = token.target;

    // If caller did not provide a recipient for the read value, a temporal is
    // provided.
    std::string temporal;
    if (!token.target) target = &temporal;

    // Backup the position of the recipient where the token will be stored
    size_t target_start = target->size();
    size_t value_start = target_start;
    size_t value_size = 0;

    // Backup the offset where the read token will start
    m_source->skip_whitespaces();
    auto offset = m_source->offset() + 1;

    switch (token.type) {
      case 'S':
      case 'X':
      case 'I':
      case 'N':
        if (m_source->peek() != '"')
          throw_invalid_json(missing_data, context, offset);
        get_string(target, context);
        value_start++;
        value_size = target->size() - target_start - 2;
        break;
      case 'v':
      case 'i':
      case 'n':
        get_value(target);
        value_size = target->size() - target_start;
        break;
      case '{':
      case '}':
      case '[':
      case ']':
      case ',':
      case ':':
        if (m_source->peek() != token.type) {
          throw_invalid_json(missing_data, context, offset);
        } else {
          get_char(target);
        }
        continue;
      default:
        assert(0);
    }

    // S and X strings may be validated to have a specific length
    if (token.length && (token.type == 'S' || token.type == 'X') &&
        value_size != token.length) {
      throw_invalid_json(missing_data, context, offset);
    }

    // S strings may be validated to be a specific value if the value is defined
    if (token.type == 'S' && !token.value.empty() &&
        token.value != target->substr(target_start)) {
      throw_invalid_json(token.value, context, offset);
    }

    // X strings mist have pure hexadecimal digits
    if (token.type == 'X') {
      for (size_t index = 0; index < value_size; index++) {
        if (!isxdigit((*target)[value_start + index]))
          throw_invalid_json(missing_data, context, offset);
      }
    }

    // Numeric values are converted into double to verify validity
    if (token.type == 'n' || token.type == 'N' || token.type == 'i' ||
        token.type == 'I') {
      if (value_size == 0) throw_invalid_json(missing_data, context, offset);

      // Integers must have pure decimal digits
      if (token.type == 'I' || token.type == 'i') {
        size_t digit_index = 0;
        if ((*target)[0] == '+' || (*target)[0] == '-') digit_index++;

        for (size_t index = digit_index; index < value_size; index++) {
          if (!isdigit((*target)[value_start + index]))
            throw_invalid_json(missing_data, context, offset);
        }
      }

      std::string str_number = target->substr(value_start, value_size);
      str_number = shcore::str_strip(str_number);

      // If a placeholder is provided, the number is placed there
      double *number = token.ntarget;
      double temp_dbl;
      if (!number) number = &temp_dbl;
      char *end = nullptr;
      (*number) = std::strtod(str_number.c_str(), &end);

      // End will point to to the character after the last converted character
      // So it should be the end of the string to a complete conversion
      if (*end != '\0') throw_invalid_json(missing_data, context, offset);
    }
  }
  m_source->skip_whitespaces();
}

/**
 * Process a BSON ObjectID Document
 */
void Json_document_parser::parse_bson_oid() {
  // Throws away {"$oid"
  clear_document();

  // And gets the associated value
  get_bson_data(
      {
          {':'},
          {'X', "", m_document, nullptr, 24},
          {'}'},
      },
      "processing extended JSON for $oid");

  // Extraction of time from $oid is done for ObjectID value at the _id field
  // Of the first level document, also only if requested
  if (m_depth == 1 && !m_options.extract_oid_time.is_null() &&
      m_context == "_id") {
    size_t pos;
    std::string tstamp_hex =
        "0x" + m_document->substr(m_document_start_offset + 1, 8);
    time_t tstamp = std::stol(tstamp_hex, &pos, 16);
    std::string tstamp_str = mysqlshdk::utils::fmttime(
        "%F %T", mysqlshdk::utils::Time_type::GMT, &tstamp);

    m_document->append(",\"" + *m_options.extract_oid_time + "\":\"" +
                       tstamp_str + "\"");
  }
}

/**
 * Process a BSON Date Document
 */
void Json_document_parser::parse_bson_date() {
  // Throws away {"$date"
  clear_document();

  // And gets the associated value
  get_bson_data(
      {
          {':'},
          {'S', "", m_document},
          {'}'},
      },
      "processing extended JSON for $date");
}

/**
 * Process a BSON Decimal Document
 */
void Json_document_parser::parse_bson_decimal() {
  // Throws away {"$numberDecimal"
  clear_document();

  std::string decimal_str;
  std::string *custom_target =
      m_options.decimal_as_double.get_safe(false) ? &decimal_str : m_document;
  double number;

  // Gets the associated value
  get_bson_data({{':'}, {'N', "", custom_target, &number}, {'}'}},
                "processing extended JSON for $numberDecimal");

  if (m_options.decimal_as_double.get_safe(false)) {
    m_document->append(std::to_string(number));
  }
}

/**
 * Processes BSON NumberInt and NumberLong Documents
 */
void Json_document_parser::parse_bson_integer(Bson_type type) {
  // Throws away {"$numberInt" or {$numberLong
  clear_document();
  std::string value;
  std::string context = "processing extended JSON for $number";
  if (type == Bson_type::LONG)
    context += "Long";
  else
    context += "Int";

  get_bson_data({{':'}, {'I', "", &value, nullptr}, {'}'}}, context);

  std::string number = value.substr(1, value.size() - 2);

  m_document->append(number);
}

bool Json_document_parser::valid_timestamp(double json_number) {
  struct tm local_time_struct;
  time_t tstamp = static_cast<time_t>(json_number);

  // Converts the time_t version of the original number to
  // a tm struct

#ifdef _WIN32
  localtime_s(&local_time_struct, &tstamp);
#else
  localtime_r(&tstamp, &local_time_struct);
#endif

  // Now gets the time_t value from that struct
  time_t local_tstamp = mktime(&local_time_struct);

  // If final value matches the original value, then it's a valid date
  return json_number == local_tstamp;
}

/**
 * Processes BSON Timestamp Document
 */
void Json_document_parser::parse_bson_timestamp() {
  // Throws away {"$timestamp"
  clear_document();

  std::string tstamp_str;
  double tstamp_n;

  get_bson_data({{':'},
                 {'{'},
                 {'S', "\"t\""},
                 {':'},
                 {'n', "", &tstamp_str, &tstamp_n},
                 {','},
                 {'S', "\"i\""},
                 {':'},
                 {'n'},
                 {'}'},
                 {'}'}},
                "processing extended JSON for $timestamp");

  if (!valid_timestamp(tstamp_n)) {
    throw invalid_json(
        "Invalid timestamp value found processing extended JSON for "
        "$timestamp.",
        m_source->offset());
  } else {
    time_t tstamp = static_cast<time_t>(tstamp_n);

    m_document->append("\"" +
                       mysqlshdk::utils::fmttime(
                           "%F %T", mysqlshdk::utils::Time_type::GMT, &tstamp) +
                       "\"");
  }
}

/**
 * Processes BSON Regex Document
 */
void Json_document_parser::parse_bson_regex() {
  // Throws away {"$regex"
  clear_document();
  std::string regex;
  std::string options;
  auto offset = m_source->offset();
  get_bson_data({{':'},
                 {'S', "", &regex},
                 {','},
                 {'S', "\"$options\""},
                 {':'},
                 {'S', "", &options},
                 {'}'}},
                "processing extended JSON for $regex");

  // Unquotes the options
  options = options.substr(1, options.size() - 2);

  if (m_options.ignore_regexp_options.get_safe(true)) {
    m_document->append(regex);
    if (!options.empty()) {
      std::string warning = "The regular expression for " + m_context +
                            " contains options being ignored: " + options + ".";
      mysqlsh::current_console()->print_warning(warning);
      log_warning("%s", warning.c_str());
    }
  } else {
    for (const auto &c : options) {
      if (c != 'i' && c != 'm' && c != 'x' && c != 's')
        throw invalid_json(
            "Unexpected data, invalid options found processing extended JSON "
            "for $regex",
            offset);
    }

    m_document->append("\"/");
    m_document->append(regex.substr(1, regex.size() - 2));
    m_document->append("/");
    m_document->append(options);
    m_document->append("\"");
  }
}

/**
 * Processes BSON BinData Document
 */
void Json_document_parser::parse_bson_binary() {
  // Throws away {"$binary"
  clear_document();

  get_bson_data({{':'},
                 {'S', "", m_document},
                 {','},
                 {'S', "\"$type\""},
                 {':'},
                 {'X', "", nullptr, nullptr, 2},
                 {'}'}},
                "processing extended JSON for $binary");
}

void Json_document_parser::get_char(std::string *target) {
  if (target)
    (*target) += m_source->get();
  else
    m_source->get();
}

/**
 * Parses a double quoted string from m_source
 * @param target: the target string where the read characters will be appended
 * @returns A boolean indicating whether the string may be a mongodb special
 *          key, they start with $.
 */
void Json_document_parser::get_string(std::string *target,
                                      const std::string &context) {
  if (m_source->peek() != '"')
    throw_invalid_json("a string", context, m_source->offset());

  get_char(target);

  // Count holds the count of the characters read between the quotes
  int count = 0;
  bool done = false;
  while (!m_source->eof() && !done) {
    switch (m_source->peek()) {
      case '\\':
        get_char(target);
        get_char(target);
        break;

      case '"':
        get_char(target);
        done = true;
        break;

      default:
        get_char(target);
    }
    if (!done) count++;
  }

  if (!done) throw_premature_end();
}

void Json_document_parser::get_whitespaces(std::string *target) {
  while (!m_source->eof() && ::isspace(m_source->peek())) get_char(target);
}

void Json_document_parser::get_value(std::string *target) {
  switch (m_source->peek()) {
    case '\0':
      // end of input before end of document
      throw_premature_end();
      break;

    case '{': {
      std::string context;
      if (!m_as_array) {
        size_t size = m_last_attribute_end - m_last_attribute_start;
        context = m_document->substr(m_last_attribute_start, size);
      }
      Json_document_parser parser(m_source, m_options, m_depth + 1, false,
                                  context);
      parser.parse(m_document);
      break;
    }
    case '[': {
      Json_document_parser parser(m_source, m_options, m_depth + 1, true);
      parser.parse(m_document);
      break;
    }
    case '"':
      get_string(target);
      break;
    case '}':
      throw invalid_json("Unexpected '}'", m_source->offset());
      break;
    case ']':
      throw invalid_json("Unexpected ']'", m_source->offset());
      break;
    default: {
      while (m_source->peek() != ',' &&
             m_source->peek() != (m_as_array ? ']' : '}'))
        get_char(target);
    }
  }
}

}  // namespace shcore

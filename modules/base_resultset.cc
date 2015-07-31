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

#include "base_resultset.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"
#include "shellcore/common.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>

using namespace mysh;
using namespace shcore;

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

BaseResultset::BaseResultset()
{
  add_method("nextDataSet", boost::bind(&BaseResultset::next_result, this, _1), NULL);
  add_method("next", boost::bind(&BaseResultset::next, this, _1), NULL);
  add_method("all", boost::bind(&BaseResultset::all, this, _1), NULL);
  add_method("__paged_output__", boost::bind(&BaseResultset::print, this, _1), NULL);

  add_method("getColumnMetadata", boost::bind(&BaseResultset::get_member_method, this, _1, "getColumnMetadata", "columnMetadata"), NULL);
  add_method("getAffectedRows", boost::bind(&BaseResultset::get_member_method, this, _1, "getAffectedRows", "affectedRows"), NULL);
  add_method("getFetchedRowCount", boost::bind(&BaseResultset::get_member_method, this, _1, "getFetchedRowCount", "fetchedRowCount"), NULL);
  add_method("getWarningCount", boost::bind(&BaseResultset::get_member_method, this, _1, "getWarningCount", "warningCount"), NULL);
  add_method("getWarnings", boost::bind(&BaseResultset::get_member_method, this, _1, "getWarnings", "warnings"), NULL);
  add_method("getExecutionTime", boost::bind(&BaseResultset::get_member_method, this, _1, "getExecutionTime", "executionTime"), NULL);
  add_method("getLastInsertId", boost::bind(&BaseResultset::get_member_method, this, _1, "getLastInsertId", "lastInsertId"), NULL);
  add_method("getInfo", boost::bind(&BaseResultset::get_member_method, this, _1, "getInfo", "info"), NULL);
  add_method("getHasData", boost::bind(&BaseResultset::get_member_method, this, _1, "getHasData", "hasData"), NULL);
}

std::vector<std::string> BaseResultset::get_members() const
{
  std::vector<std::string> members(shcore::Cpp_object_bridge::get_members());
  members.push_back("columnMetadata");
  members.push_back("fetchedRowCount");
  members.push_back("affectedRows");
  members.push_back("warningCount");
  members.push_back("executionTime");
  members.push_back("lastInsertId");
  members.push_back("info");
  members.push_back("hasData");
  return members;
}

bool BaseResultset::operator == (const Object_bridge &other) const
{
  return this == &other;
}

shcore::Value BaseResultset::get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop)
{
  std::string function = class_name() + "::" + method;
  args.ensure_count(0, function.c_str());

  return get_member(prop);
}

shcore::Value BaseResultset::print(const shcore::Argument_list &args)
{
  std::string function = class_name() + "::print";

  args.ensure_count(0, 3, function.c_str());

  std::string format;
  bool interactive = true;
  bool show_warnings = false;
  if (args.size() > 0)
    interactive = args.bool_at(0);

  if (args.size() > 1)
    format = args.string_at(1);

  if (args.size() > 2)
    show_warnings = args.bool_at(2);

  if (format == "jsonraw" || format == "jsonpretty")
    print_json(format, show_warnings);
  else
    print_normal(interactive, format, show_warnings);

  return shcore::Value();
}

void BaseResultset::print_json(const std::string& format, bool show_warnings)
{
  shcore::Value::Map_type_ref data(new shcore::Value::Map_type);

  if (get_member("hasData").as_bool())
  {
    shcore::Value records = all(shcore::Argument_list());

    (*data)["row_count"] = get_member("fetchedRowCount");
    (*data)["rows"] = records;
  }
  else
  {
    (*data)["affected_rows"] = get_member("affectedRows");
  }

  (*data)["duration"] = get_member("executionTime");
  (*data)["warning_count"] = get_member("warningCount");
  (*data)["info"] = get_member("info");

  if ((*data)["warning_count"].as_int() && show_warnings)
    (*data)["warnings"] = get_member("warnings");

  shcore::Value map(data);

  shcore::print(map.json(format == "jsonpretty") + "\n");
}

void BaseResultset::print_normal(bool interactive, const std::string& format, bool show_warnings)
{
  std::string output;

  do
  {
    if (get_member("hasData").as_bool())
    {
      shcore::Argument_list args;
      shcore::Value records = all(args);
      shcore::Value::Array_type_ref array_records = records.as_array();

      // Gets the first row to determine there is data to print
      // Base_row * row = next_row();

      if (array_records->size())
      {
        // print rows from result, with stats etc
        if (interactive || format == "table")
          print_table(array_records);
        else
          print_tabbed(array_records);

        int row_count = int(array_records->size());
        output = (boost::format("%lld %s in set") % row_count % (row_count == 1 ? "row" : "rows")).str();
      }
      else
        output = "Empty set";
    }
    else
    {
      // Some queries return -1 since affected rows do not apply to them
      int affected_rows = get_member("affectedRows").as_int();
      if (affected_rows == ~(uint64_t)0)
        output = "Query OK";
      else
        // In case of Query OK, prints the actual number of affected rows.
        output = (boost::format("Query OK, %lld %s affected") % affected_rows % (affected_rows == 1 ? "row" : "rows")).str();
    }

    // This information output is only printed in interactive mode
    int warning_count = 0;
    if (interactive)
    {
      warning_count = get_member("warningCount").as_uint();

      if (warning_count)
        output.append((boost::format(", %d warning%s") % warning_count % (warning_count == 1 ? "" : "s")).str());

      output.append(" ");
      output.append((boost::format("(%s)") % get_member("executionTime").as_string()).str());
      output.append("\n");

      shcore::print(output);
    }

    std::string info = get_member("info").as_string();
    if (!info.empty())
      shcore::print("\n" + info + "\n");

    // Prints the warnings if there were any
    if (warning_count && show_warnings)
      print_warnings();
  } while (next_result(shcore::Argument_list()).as_bool());
}

void BaseResultset::print_tabbed(shcore::Value::Array_type_ref records)
{
  boost::shared_ptr<shcore::Value::Array_type> metadata = get_member("columnMetadata").as_array();

  unsigned int index = 0;
  unsigned int field_count = metadata->size();
  std::vector<std::string> formats(field_count, "%-");

  // Prints the initial separator line and the column headers
  // TODO: Consider the charset information on the length calculations
  for (index = 0; index < field_count; index++)
  {
    std::string data = (*(*metadata)[index].as_map())["name"].as_string();
    shcore::print(data);
    shcore::print(index < (field_count - 1) ? "\t" : "\n");
  }

  // Now prints the records
  for (size_t row_index = 0; row_index < records->size(); row_index++)
  {
    boost::shared_ptr<Row> row = (*records)[row_index].as_object<Row>();

    for (size_t field_index = 0; field_index < field_count; field_index++)
    {
      std::string raw_value = row->get_member(field_index).descr();
      shcore::print(raw_value);
      shcore::print(field_index < (field_count - 1) ? "\t" : "\n");
    }
  }
}

void BaseResultset::print_table(shcore::Value::Array_type_ref records)
{
  boost::shared_ptr<shcore::Value::Array_type> metadata = get_member("columnMetadata").as_array();

  // Updates the metadata max_length field with the maximum length of the field values
  size_t row_index;
  for (row_index = 0; row_index < records->size(); row_index++)
  {
    boost::shared_ptr<Row> row = (*records)[row_index].as_object<Row>();
    for (size_t field_index = 0; field_index < metadata->size(); field_index++)
    {
      int field_length = row->get_member(field_index).repr().length();

      if (field_length >(*(*metadata)[field_index].as_map())["max_length"].as_int())
        (*(*metadata)[field_index].as_map())["max_length"] = Value(field_length);
    }
  }
  //-----------

  unsigned int index = 0;
  unsigned int field_count = metadata->size();
  std::vector<std::string> formats(field_count, "%-");

  // Calculates the max column widths and constructs the separator line.
  std::string separator("+");
  for (index = 0; index < field_count; index++)
  {
    int max_field_length = 0;
    max_field_length = std::max<unsigned int>((*(*metadata)[index].as_map())["max_length"].as_int(), (*(*metadata)[index].as_map())["name_length"].as_int());
    max_field_length = std::max<unsigned int>(max_field_length, MIN_COLUMN_LENGTH);
    (*(*metadata)[index].as_map())["max_length"] = Value(max_field_length);
    //_metadata[index].max_length(max_field_length);

    // Creates the format string to print each field
    formats[index].append(boost::lexical_cast<std::string>(max_field_length));
    if (index == field_count - 1)
      formats[index].append("s |");
    else
      formats[index].append("s | ");

    std::string field_separator(max_field_length + 2, '-');
    field_separator.append("+");
    separator.append(field_separator);
  }
  separator.append("\n");

  // Prints the initial separator line and the column headers
  // TODO: Consider the charset information on the length calculations
  shcore::print(separator + "| ");
  for (index = 0; index < field_count; index++)
  {
    std::string data = (boost::format(formats[index]) % (*(*metadata)[index].as_map())["name"].as_string()).str();
    shcore::print(data);

    // Once the header is printed, updates the numeric fields formats
    // so they are right aligned
    if ((*(*metadata)[index].as_map())["is_numeric"].as_bool())
      formats[index] = formats[index].replace(1, 1, "");
  }

  shcore::print("\n" + separator);

  // Now prints the records
  for (row_index = 0; row_index < records->size(); row_index++)
  {
    shcore::print("| ");

    boost::shared_ptr<Row> row = (*records)[row_index].as_object<Row>();

    for (size_t field_index = 0; field_index < metadata->size(); field_index++)
    {
      std::string raw_value = row->get_member(field_index).descr();
      std::string data = (boost::format(formats[field_index]) % (raw_value)).str();

      shcore::print(data);
    }
    shcore::print("\n");
  }

  shcore::print(separator);
}

void BaseResultset::print_warnings()
{
  Value warnings = get_member("warnings");

  if (warnings)
  {
    Value::Array_type_ref warning_list = warnings.as_array();
    size_t index = 0, size = warning_list->size();

    while (index < size)
    {
      Value record = warning_list->at(index);
      boost::shared_ptr<mysh::Row> row = record.as_object<mysh::Row>();

      unsigned long error = row->get_member("Code").as_int();

      std::string type = row->get_member("Level").as_string();
      std::string msg = row->get_member("Message").as_string();
      shcore::print((boost::format("%s (Code %ld): %s\n") % type % error % msg).str());

      index++;
    }
  }
}

Row::Row()
{
  add_method("getLength", boost::bind(&Row::get_member_method, this, _1, "getLength", "__length__"), NULL);
}

std::string &Row::append_descr(std::string &s_out, int indent, int UNUSED(quote_strings)) const
{
  std::string nl = (indent >= 0) ? "\n" : "";
  s_out += "[";
  for (size_t index = 0; index < value_iterators.size(); index++)
  {
    if (index > 0)
      s_out += ",";

    s_out += nl;

    if (indent >= 0)
      s_out.append((indent + 1) * 4, ' ');

    value_iterators[index]->second.append_descr(s_out, indent < 0 ? indent : indent + 1, '"');
  }

  s_out += nl;
  if (indent > 0)
    s_out.append(indent * 4, ' ');

  s_out += "]";

  return s_out;
}

void Row::append_json(const shcore::JSON_dumper& dumper) const
{
  dumper.start_object();

  for (size_t index = 0; index < value_iterators.size(); index++)
    dumper.append_value(value_iterators[index]->first, value_iterators[index]->second);

  dumper.end_object();
}

std::string &Row::append_repr(std::string &s_out) const
{
  return append_descr(s_out);
}

shcore::Value Row::get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop)
{
  std::string function = class_name() + "::" + method;
  args.ensure_count(0, function.c_str());

  return get_member(prop);
}

//! Returns the list of members that this object has
std::vector<std::string> Row::get_members() const
{
  std::vector<std::string> l = shcore::Cpp_object_bridge::get_members();
  l.push_back("__length__");

  for (size_t index = 0; index < value_iterators.size(); index++)
    l.push_back(value_iterators[index]->first);

  return l;
}

//! Implements equality operator
bool Row::operator == (const Object_bridge &UNUSED(other)) const
{
  return false;
}

//! Returns the value of a member
shcore::Value Row::get_member(const std::string &prop) const
{
  if (prop == "__length__")
    return shcore::Value((int)values.size());
  else
  {
    unsigned int index = 0;
    if (sscanf(prop.c_str(), "%u", &index) == 1)
      return get_member(index);
    else
    {
      std::map<std::string, shcore::Value>::const_iterator it;
      if ((it = values.find(prop)) != values.end())
        return it->second;
    }
  }

  return shcore::Cpp_object_bridge::get_member(prop);
}

shcore::Value Row::get_member(size_t index) const
{
  if (index < value_iterators.size())
    return value_iterators[index]->second;
  else
    return shcore::Value();
}

void Row::add_item(const std::string &key, shcore::Value value)
{
  value_iterators.push_back(values.insert(values.end(), std::pair<std::string, shcore::Value>(key, value)));
}
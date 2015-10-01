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

#include "shell_resultset_dumper.h"
#include "shellcore/shell_core_options.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

using namespace shcore;

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

ResultsetDumper::ResultsetDumper(boost::shared_ptr<mysh::BaseResultset> target) :
_resultset(target)
{
}

void ResultsetDumper::dump()
{
  std::string format = Shell_core_options::get()->get_string(SHCORE_OUTPUT_FORMAT);
  bool interactive = Shell_core_options::get()->get_bool(SHCORE_INTERACTIVE);
  bool show_warnings = Shell_core_options::get()->get_bool(SHCORE_SHOW_WARNINGS);

  std::string type = _resultset->class_name();

  if (type == "CollectionResultset" || format.find("json") == 0)
    dump_json(format, show_warnings);
  else
    dump_normal(interactive, format, show_warnings);
}

void ResultsetDumper::dump_json(const std::string& format, bool show_warnings)
{
  shcore::Value::Map_type_ref data(new shcore::Value::Map_type);

  if (_resultset->get_member("hasData").as_bool())
  {
    shcore::Value records = _resultset->all(shcore::Argument_list());

    (*data)["row_count"] = _resultset->get_member("fetchedRowCount");
    (*data)["rows"] = records;
  }
  else
  {
    (*data)["affected_rows"] = _resultset->get_member("affectedRows");
  }

  (*data)["duration"] = _resultset->get_member("executionTime");
  (*data)["warning_count"] = _resultset->get_member("warningCount");
  (*data)["info"] = _resultset->get_member("info");

  if ((*data)["warning_count"].as_int() && show_warnings)
    (*data)["warnings"] = _resultset->get_member("warnings");

  shcore::Value map(data);

  shcore::print(map.json(format != "json/raw") + "\n");
}

void ResultsetDumper::dump_normal(bool interactive, const std::string& format, bool show_warnings)
{
  std::string output;

  do
  {
    if (_resultset->get_member("hasData").as_bool())
    {
      shcore::Argument_list args;
      shcore::Value records = _resultset->all(args);
      shcore::Value::Array_type_ref array_records = records.as_array();

      // Gets the first row to determine there is data to print
      // Base_row * row = next_row();

      if (array_records->size())
      {
        // print rows from result, with stats etc
        if (interactive || format == "table")
          dump_table(array_records);
        else
          dump_tabbed(array_records);

        int row_count = int(array_records->size());
        output = (boost::format("%lld %s in set") % row_count % (row_count == 1 ? "row" : "rows")).str();
      }
      else
        output = "Empty set";
    }
    else
    {
      // Some queries return -1 since affected rows do not apply to them
      uint64_t affected_rows = _resultset->get_member("affectedRows").as_uint();
      if (affected_rows == (uint64_t)-1)
        output = "Query OK";
      else
        // In case of Query OK, prints the actual number of affected rows.
        output = (boost::format("Query OK, %lld %s affected") % affected_rows % (affected_rows == 1 ? "row" : "rows")).str();
    }

    // This information output is only printed in interactive mode
    int warning_count = 0;
    if (interactive)
    {
      warning_count = _resultset->get_member("warningCount").as_uint();

      if (warning_count)
        output.append((boost::format(", %d warning%s") % warning_count % (warning_count == 1 ? "" : "s")).str());

      output.append(" ");
      output.append((boost::format("(%s)") % _resultset->get_member("executionTime").as_string()).str());
      output.append("\n");

      shcore::print(output);
    }

    std::string info = _resultset->get_member("info").as_string();
    if (!info.empty())
      shcore::print("\n" + info + "\n");

    // Prints the warnings if there were any
    if (warning_count && show_warnings)
      dump_warnings();
  } while (_resultset->next_result(shcore::Argument_list()).as_bool());
}

void ResultsetDumper::dump_tabbed(shcore::Value::Array_type_ref records)
{
  boost::shared_ptr<shcore::Value::Array_type> metadata = _resultset->get_member("columnMetadata").as_array();

  size_t index = 0;
  size_t field_count = metadata->size();
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
    boost::shared_ptr<mysh::Row> row = (*records)[row_index].as_object<mysh::Row>();

    for (size_t field_index = 0; field_index < field_count; field_index++)
    {
      std::string raw_value = row->get_member(field_index).descr();
      shcore::print(raw_value);
      shcore::print(field_index < (field_count - 1) ? "\t" : "\n");
    }
  }
}

void ResultsetDumper::dump_table(shcore::Value::Array_type_ref records)
{
  boost::shared_ptr<shcore::Value::Array_type> metadata = _resultset->get_member("columnMetadata").as_array();

  // Updates the metadata max_length field with the maximum length of the field values
  size_t row_index;
  for (row_index = 0; row_index < records->size(); row_index++)
  {
    boost::shared_ptr<mysh::Row> row = (*records)[row_index].as_object<mysh::Row>();
    for (size_t field_index = 0; field_index < metadata->size(); field_index++)
    {
      uint64_t field_length = row->get_member(field_index).repr().length();

      if (field_length >(*(*metadata)[field_index].as_map())["max_length"].as_uint())
        (*(*metadata)[field_index].as_map())["max_length"] = Value(field_length);
    }
  }
  //-----------

  size_t index = 0;
  size_t field_count = metadata->size();
  std::vector<std::string> formats(field_count, "%-");

  // Calculates the max column widths and constructs the separator line.
  std::string separator("+");
  for (index = 0; index < field_count; index++)
  {
    uint64_t max_field_length = 0;
    max_field_length = std::max<uint64_t>((*(*metadata)[index].as_map())["max_length"].as_uint(), (*(*metadata)[index].as_map())["name_length"].as_uint());
    max_field_length = std::max<uint64_t>(max_field_length, MIN_COLUMN_LENGTH);
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

    boost::shared_ptr<mysh::Row> row = (*records)[row_index].as_object<mysh::Row>();

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

void ResultsetDumper::dump_warnings()
{
  Value warnings = _resultset->get_member("warnings");

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
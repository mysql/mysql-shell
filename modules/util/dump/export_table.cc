/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#include "modules/util/dump/export_table.h"

#include <stdexcept>
#include <string>
#include <utility>

#include "mysqlshdk/include/scripting/naming_style.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dump {

Export_table::Export_table(const Export_table_options &options)
    : Dumper(options), m_options(options) {}

void Export_table::summary() const {
  if (nullptr == m_cache) {
    throw std::runtime_error("Internal error - table was not dumped!");
  }

  const auto quoted_filename =
      shcore::quote_string(m_options.output_url(), '"');
  const auto import_table =
      shcore::get_member_name("importTable", shcore::current_naming_style());
  shcore::Dictionary_t options = shcore::make_dict();

  // original table which was dumped
  options->emplace("schema", m_options.schema());
  options->emplace("table", m_options.table());

  // always include character set, we have a different default than import_table
  options->emplace("characterSet", m_options.character_set());

  // information about the columns
  const auto columns = shcore::make_array();
  const auto decode = shcore::make_dict();
  const auto mode = m_options.use_base64() ? "FROM_BASE64" : "UNHEX";

  for (const auto &c : m_cache->columns) {
    columns->emplace_back(c->name);

    if (c->csv_unsafe) {
      decode->emplace(c->name, mode);
    }
  }

  // add this information only if a column needs to be decoded
  if (!decode->empty()) {
    options->emplace("columns", std::move(columns));
    options->emplace("decodeColumns", std::move(decode));
  }

  // copy the original options (if there were any)
  const auto &original = m_options.original_options();

  if (original) {
    const auto add_nonempty = [&original, &options](const std::string &name) {
      const auto value = original->find(name);

      if (original->end() != value && !value->second.as_string().empty()) {
        options->emplace(name, value->second);
      }
    };
    const auto add_if_present = [&original, &options](const std::string &name) {
      const auto value = original->find(name);

      if (original->end() != value) {
        options->emplace(name, value->second);
      }
    };

    // dialect
    add_nonempty("dialect");
    add_if_present("fieldsTerminatedBy");
    add_if_present("fieldsEnclosedBy");
    add_if_present("fieldsEscapedBy");
    add_if_present("fieldsOptionallyEnclosed");
    add_if_present("linesTerminatedBy");

    // OCI options
    constexpr auto k_os_bucket_name = "osBucketName";
    add_nonempty(k_os_bucket_name);

    if (options->end() != options->find(k_os_bucket_name)) {
      add_nonempty("osNamespace");
      add_nonempty("ociConfigFile");
      add_nonempty("ociProfile");
    }
  }

  shcore::JSON_dumper dumper{true};
  dumper.append_value(shcore::Value(options));

  const auto console = current_console();

  console->print_status("");
  console->print_status("The dump can be loaded using:");
  console->print_status("util." + import_table + "(" + quoted_filename + ", " +
                        dumper.str() + ")");
}

void Export_table::on_create_table_task(const std::string &,
                                        const std::string &,
                                        const Instance_cache::Table *cache) {
  m_cache = cache;
}

}  // namespace dump
}  // namespace mysqlsh

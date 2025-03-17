/*
 * Copyright (c) 2020, 2025, Oracle and/or its affiliates.
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

#include "modules/util/dump/export_table.h"

#include <stdexcept>
#include <string>
#include <utility>

#include "mysqlshdk/include/scripting/naming_style.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/aws/s3_bucket_options.h"
#include "mysqlshdk/libs/azure/blob_storage_options.h"
#include "mysqlshdk/libs/oci/oci_bucket_options.h"
#include "mysqlshdk/libs/storage/backend/object_storage_options.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/common/utils.h"

namespace mysqlsh {
namespace dump {

Export_table::Export_table(const Export_table_options &options)
    : Dumper(options), m_options(options) {}

void Export_table::summary() const {
  if (nullptr == m_cache) {
    throw std::logic_error("Internal error - table was not dumped!");
  }

  const auto quoted_filename =
      shcore::quote_string(m_options.output_url(), '"');
  const auto import_table = mysqlsh::common::member_name("importTable");
  shcore::Dictionary_t options = shcore::make_dict();

  // original table which was dumped
  options->emplace("schema", m_options.schema());
  options->emplace("table", m_options.table());

  // always include character set, we have a different default than import_table
  options->emplace("characterSet", m_options.character_set());

  // information about the columns
  const auto columns = shcore::make_array();
  const auto decode = shcore::make_dict();

  for (const auto &c : m_cache->columns) {
    columns->emplace_back(c->name);

    if (auto decoded = decode_column(m_cache, c); !decoded.empty()) {
      decode->emplace(c->name, std::move(decoded));
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
      bool added = false;

      if (original->end() != value && !value->second.as_string().empty()) {
        options->emplace(name, value->second);
        added = true;
      }

      return added;
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

    {
      // handle remote options (OCI, AWS, Azure, etc.)
      std::vector<std::unique_ptr<
          mysqlshdk::storage::backend::object_storage::Object_storage_options>>
          remote_options;

      remote_options.emplace_back(
          std::make_unique<mysqlshdk::oci::Oci_bucket_options>());
      remote_options.emplace_back(
          std::make_unique<mysqlshdk::aws::S3_bucket_options>());
      remote_options.emplace_back(
          std::make_unique<mysqlshdk::azure::Blob_storage_options>(
              mysqlshdk::azure::Blob_storage_options::Operation::WRITE));

      for (const auto &remote : remote_options) {
        if (add_nonempty(remote->get_main_option())) {
          for (const auto name : remote->get_secondary_options()) {
            add_nonempty(name);
          }
        }
      }
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

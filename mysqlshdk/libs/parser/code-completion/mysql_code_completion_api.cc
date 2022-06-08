/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/parser/code-completion/mysql_code_completion_api.h"

#include <set>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/parser/code-completion/mysql_code_completion_context.h"
#include "mysqlshdk/libs/parser/server/sql_modes.h"

namespace mysqlshdk {

using utils::Version;

const Version k_current_version{MYSH_VERSION};

const shcore::Option_pack_def<Auto_complete_sql_options>
    &Auto_complete_sql_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Auto_complete_sql_options>()
          .required(server_version_option(),
                    &Auto_complete_sql_options::set_server_version)
          .required(sql_mode_option(), &Auto_complete_sql_options::m_sql_mode)
          .optional(statement_offset_option(),
                    &Auto_complete_sql_options::m_statement_offset)
          .optional(uppercase_keywords_option(),
                    &Auto_complete_sql_options::m_uppercase_keywords)
          .optional(filtered_option(), &Auto_complete_sql_options::m_filtered)
          .on_done(&Auto_complete_sql_options::on_unpacked_options);

  return opts;
}

void Auto_complete_sql_options::on_unpacked_options() {
  if (m_server_version < Version(5, 7, 0)) {
    m_server_version = Version(5, 7, 0);
  } else if (m_server_version > k_current_version) {
    m_server_version = k_current_version;
  }

  const auto modes = shcore::str_split(m_sql_mode, ",");
  std::set<std::string_view> missing_modes;
  const auto &available_modes =
      m_server_version < Version(8, 0, 0) ? k_sql_modes_57 : k_sql_modes_80;

  for (const auto &mode : modes) {
    if (available_modes.count(mode) == 0) {
      missing_modes.emplace(mode);
    }
  }

  if (!missing_modes.empty()) {
    throw std::invalid_argument("MySQL server " + m_server_version.get_full() +
                                " does not support the following SQL modes: " +
                                shcore::str_join(missing_modes, ", "));
  }
}

namespace {

std::string to_string(parsers::Sql_completion_result::Candidate candidate) {
  using Candidate = parsers::Sql_completion_result::Candidate;

  switch (candidate) {
    case Candidate::SCHEMA:
      return "schemas";

    case Candidate::TABLE:
      return "tables";

    case Candidate::VIEW:
      return "views";

    case Candidate::COLUMN:
      return "columns";

    case Candidate::INTERNAL_COLUMN:
      return "internalColumns";

    case Candidate::PROCEDURE:
      return "procedures";

    case Candidate::FUNCTION:
      return "functions";

    case Candidate::TRIGGER:
      return "triggers";

    case Candidate::EVENT:
      return "events";

    case Candidate::ENGINE:
      return "engines";

    case Candidate::UDF:
      return "udfs";

    case Candidate::RUNTIME_FUNCTION:
      return "runtimeFunctions";

    case Candidate::LOGFILE_GROUP:
      return "logfileGroups";

    case Candidate::USER_VAR:
      return "userVars";

    case Candidate::SYSTEM_VAR:
      return "systemVars";

    case Candidate::TABLESPACE:
      return "tablespaces";

    case Candidate::USER:
      return "users";

    case Candidate::CHARSET:
      return "charsets";

    case Candidate::COLLATION:
      return "collations";

    case Candidate::PLUGIN:
      return "plugins";

    case Candidate::LABEL:
      return "labels";
  }

  throw std::logic_error("Unknown value of Sql_completion_result::Candidate");
}

}  // namespace

shcore::Dictionary_t auto_complete_sql(
    const std::string &sql, const Auto_complete_sql_options &options) {
  // use a static variable, so that we maintain the context between calls, if
  // the same server version / SQL mode combination is used, the code completion
  // will work faster
  static thread_local Sql_completion_context completion(Version(8, 0, 0));

  completion.set_server_version(options.server_version());
  completion.set_sql_mode(options.sql_mode());
  completion.set_uppercase_keywords(options.uppercase_keywords());
  completion.set_filtered(options.filtered());
  // We don't know if the active schema is set or not, we set it here to some
  // value. This way we'll get more results, caller has to figure out if they
  // are applicable or not.
  completion.set_active_schema("unused");

  const auto result = completion.complete(
      sql, options.statement_offset().value_or(sql.length()));
  auto dict = shcore::make_dict();

  {
    auto context = shcore::make_dict();

    context->emplace("prefix", result.context.prefix.full);

    if (!result.context.qualifier.empty()) {
      context->emplace("qualifier",
                       shcore::make_array(result.context.qualifier));
    }

    if (!result.context.references.empty()) {
      auto references = shcore::make_array();

      for (const auto &ref : result.context.references) {
        auto reference = shcore::make_dict();

        if (!ref.schema.empty()) {
          reference->emplace("schema", ref.schema);
        }

        assert(!ref.table.empty());
        reference->emplace("table", ref.table);

        if (!ref.alias.empty()) {
          reference->emplace("alias", ref.alias);
        }

        references->emplace_back(std::move(reference));
      }

      context->emplace("references", std::move(references));
    }

    if (!result.context.labels.empty()) {
      context->emplace("labels", shcore::make_array(result.context.labels));
    }

    dict->emplace("context", std::move(context));
  }

  if (!result.keywords.empty()) {
    dict->emplace("keywords", shcore::make_array(result.keywords));
  }

  if (!result.system_functions.empty()) {
    dict->emplace("functions", shcore::make_array(result.system_functions));
  }

  if (!result.candidates.empty()) {
    auto candidates = shcore::make_array();

    for (const auto &candidate : result.candidates) {
      candidates->emplace_back(to_string(candidate));
    }

    dict->emplace("candidates", std::move(candidates));
  }

  return dict;
}

}  // namespace mysqlshdk

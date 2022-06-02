/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
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

#include "src/mysqlsh/json_shell.h"

#include <rapidjson/document.h>
#include <algorithm>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/utils/utils_json.h"

namespace mysqlsh {
Json_shell::Json_shell(std::shared_ptr<Shell_options> options)
    : Command_line_shell(options) {
  options->set_json_output();
}

Json_shell::Json_shell(std::shared_ptr<Shell_options> cmdline_options,
                       std::unique_ptr<shcore::Interpreter_delegate> delegate)
    : Command_line_shell(cmdline_options, std::move(delegate)) {}

void Json_shell::process_line(const std::string &line) {
  rapidjson::Document doc;
  doc.Parse(line.c_str());

  auto console = mysqlsh::current_console();
  if (!doc.HasParseError() && doc.IsObject()) {
    if (doc.HasMember("execute")) {
      if (doc["execute"].IsString()) {
        std::string data = doc["execute"].GetString();
        log_debug2("CODE:\n%s", data.c_str());
        std::vector<std::string> lines = shcore::split_string(data, "\n");
        for (const auto &code : lines) {
          Mysql_shell::process_line(code);
        }

        // If at the end of the processing the input state is in continuation,
        // we force the execution as it is
        if (_input_mode == shcore::Input_state::ContinuedSingle ||
            _input_mode == shcore::Input_state::ContinuedBlock) {
          flush_input();
        }
      } else {
        console->print_error(
            "Invalid input for 'execute', string expected in value: " + line);
      }
    } else if (doc.HasMember("complete")) {
      if (doc["complete"].IsObject() && doc["complete"].HasMember("data") &&
          doc["complete"]["data"].IsString() &&
          (!doc["complete"].HasMember("offset") ||
           doc["complete"]["offset"].IsUint())) {
        // Default offset is 0 if not specified
        size_t completion_offset = 0;
        if (doc["complete"].HasMember("offset")) {
          completion_offset = doc["complete"]["offset"].GetUint();
        }

        std::vector<std::string> options(completer()->complete(
            shell_context()->interactive_mode(),
            doc["complete"]["data"].GetString(), &completion_offset));

        std::sort(options.begin(), options.end(),
                  [](const std::string &a, const std::string &b) -> bool {
                    return shcore::str_casecmp(a, b) < 0;
                  });
        auto last = std::unique(options.begin(), options.end());
        options.erase(last, options.end());

        shcore::JSON_dumper result;
        result.start_object();
        result.append_string("offset");
        result.append_uint(completion_offset);
        result.append_string("options");
        result.start_array();

        for (const auto &option : options) {
          result.append_string(option);
        }

        result.end_array();
        result.end_object();
        console->raw_print(result.str(), mysqlsh::Output_stream::STDOUT);
      } else {
        console->print_error(
            "Invalid input for 'complete', object with 'data' member (and "
            "optional uint 'offset' member) expected: " +
            line);
      }
    } else {
      console->print_error("Invalid command in input: " + line);
    }
  } else {
    console->print_error("Invalid input: " + line);
  }
}
}  // namespace mysqlsh

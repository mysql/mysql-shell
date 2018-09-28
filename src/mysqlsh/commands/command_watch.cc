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

#include "src/mysqlsh/commands/command_watch.h"

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {

bool Command_watch::execute(const std::vector<std::string> &args) {
  if (args.size() == 1 ||
      std::find(args.begin(), args.end(), "--help") != args.end()) {
    // no arguments -> display available reports and exit
    // contains --help -> display help and exit
    return Command_show::execute(args);
  } else {
    const auto new_args = parse_arguments(args);

    bool iterrupted = false;
    shcore::Interrupt_handler inth(
        [&iterrupted]() { return (iterrupted = true); });

    // convert seconds to milliseconds
    const auto interval = static_cast<uint32_t>(1000.0f * m_refresh_interval);

    if (m_clear_screen && !mysqlshdk::textui::supports_screen_control()) {
      current_console()->print_warning(
          "Terminal does not support ANSI escape sequences, screen will not "
          "be cleared.");
      m_clear_screen = false;
    }

    if (m_clear_screen) {
      mysqlshdk::textui::scroll_screen();
    }

    while (!iterrupted) {
      if (m_clear_screen) {
        mysqlshdk::textui::clear_screen();
      }

      Command_show::execute(new_args);

      shcore::sleep_ms(interval);
    }

    return true;
  }
}

std::vector<std::string> Command_watch::parse_arguments(
    const std::vector<std::string> &args) {
  // options handled by \watch
  static constexpr auto k_no_refresh = "--nocls";
  static constexpr auto k_interval_long = "--interval";
  static constexpr auto k_interval_short = "-i";

  // arguments passed to \show, options handled by \watch should be removed
  std::vector<std::string> new_args;

  // copy the full command, it's going to be ignored anyway
  new_args.emplace_back(args[0]);
  // first argument should be the report name, copy it
  new_args.emplace_back(args[1]);
  // rest of the input arguments needs to be analyzed
  auto current = args.begin() + 2;
  const auto end = args.end();

  // iterate over arguments, detect the ones handled by \watch and remove them
  while (current != end) {
    if (*current == k_no_refresh) {
      m_clear_screen = false;
    } else if (shcore::str_beginswith(*current, k_interval_long) ||
               shcore::str_beginswith(*current, k_interval_short)) {
      const char *value = nullptr;

      if (*current == k_interval_long || *current == k_interval_short) {
        // exact match, value should be in the next argument
        if (++current != end) {
          value = current->c_str();
        }
      } else if (shcore::str_beginswith(*current, k_interval_short)) {
        // value cannot be empty, as exact match is handled above
        value = current->c_str() + std::strlen(k_interval_short);
      } else if (shcore::str_beginswith(*current,
                                        k_interval_long + std::string{"="})) {
        value = current->c_str() + std::strlen(k_interval_long) + 1;

        if ('\0' == value[0]) {
          // empty value, set to nullptr so an exception can be thrown
          value = nullptr;
        }
      } else {
        // this is another long option which begins with "--interval"
        new_args.emplace_back(*current);
        ++current;
        continue;
      }

      if (nullptr == value) {
        throw shcore::Exception::argument_error(
            std::string{"The '"} + k_interval_long +
            "' option should have a value.");
      } else {
        bool error = false;

        try {
          m_refresh_interval = shcore::lexical_cast<float>(value);

          if (m_refresh_interval < 0.1f || m_refresh_interval > 86400.0f) {
            error = true;
          }
        } catch (const std::invalid_argument &) {
          error = true;
        }

        if (error) {
          throw shcore::Exception::argument_error(
              std::string{"The value of '"} + k_interval_long +
              "' option should be a float in range [0.1, 86400], got: '" +
              value + "'.");
        }
      }
    } else {
      new_args.emplace_back(*current);
    }

    ++current;
  }

  return new_args;
}

}  // namespace mysqlsh

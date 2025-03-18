/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_SCRIPTING_POLYGLOT_SHELL_POLYGLOT_COMMON_CONTEXT_H_
#define MYSQLSHDK_SCRIPTING_POLYGLOT_SHELL_POLYGLOT_COMMON_CONTEXT_H_

#include "mysqlshdk/scripting/polyglot/utils/polyglot_api_clean.h"

#include "mysqlshdk/scripting/polyglot/languages/polyglot_common_context.h"

namespace shcore {

/**
 * Specialization of the Polyglot_common_context to provide shell specific
 * logging functions and garbage collection settings
 */
class Shell_polyglot_common_context : public polyglot::Polyglot_common_context {
 public:
  Shell_polyglot_common_context() = default;
  ~Shell_polyglot_common_context() override = default;

 private:
  void fatal_error() override;
  void flush() override;
  void log(const char *bytes, size_t length) override;
};

}  // namespace shcore

#endif  // MYSQLSHDK_SCRIPTING_POLYGLOT_SHELL_POLYGLOT_COMMON_CONTEXT_H_
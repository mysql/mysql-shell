/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "scripting/shexcept.h"

namespace shcore {

database_error::database_error(const char *what, int code, const char *error)
    : std::runtime_error(what), code_(code), error_(error) {}

database_error::database_error(const char *what, int code, const char *error,
                               const char *sqlstate)
    : std::runtime_error(what),
      code_(code),
      error_(error),
      sqlstate_(sqlstate ? sqlstate : "") {}

user_input_error::user_input_error(const char *what, const char *parameter)
    : std::runtime_error(what), param_(parameter) {}

cancelled::cancelled(const char *what) : std::runtime_error(what) {}

script_error::script_error(const char *what) : std::runtime_error(what) {}

}  // namespace shcore

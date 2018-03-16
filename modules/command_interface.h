/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_COMMAND_INTERFACE_H_
#define MODULES_COMMAND_INTERFACE_H_

#include <string>
#include "scripting/lang_base.h"

namespace mysqlsh {

// Command (API) Pure Abstract Class / Interface
// Each API Class must implement this interface
class Command_interface {
 public:
  virtual ~Command_interface() {}

  // Prepare should validate the parameters and other validations.
  // Later, we'd make it acquire locks and save state for the undo (rollback).
  virtual void prepare() = 0;

  // Executes the API command low-level work.
  virtual shcore::Value execute() = 0;

  // Applies the undo.
  virtual void rollback() = 0;

  // Does the final clean-up, if necessary.
  virtual void finish() = 0;
};

}  // namespace mysqlsh

#endif  // MODULES_COMMAND_INTERFACE_H_

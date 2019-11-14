/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef _MOD_MYSQLXTEST_UTILS_H_
#define _MOD_MYSQLXTEST_UTILS_H_

#include "mysqlshdk/libs/db/session.h"
#include "scripting/common.h"
#include "scripting/types_cpp.h"

/*
 * Helper function to ensure the exceptions generated on the mysqlx_connector
 * are properly translated to the corresponding shcore::Exception type
 */
#define CATCH_AND_TRANSLATE_CRUD_EXCEPTION(op)                                \
  catch (...) {                                                               \
    std::string operation(op);                                                \
    try {                                                                     \
      throw;                                                                  \
    } catch (shcore::Exception & exc) {                                       \
      auto error = exc.error();                                               \
      (*error)["message"] = shcore::Value(operation + ": " + exc.what());     \
      throw;                                                                  \
    } catch (mysqlshdk::db::Error & exc) {                                    \
      throw shcore::Exception::mysql_error_with_code(exc.what(), exc.code()); \
    } catch (std::runtime_error & exc) {                                      \
      throw shcore::Exception::runtime_error(operation + ": " + exc.what());  \
    } catch (std::logic_error & exc) {                                        \
      throw shcore::Exception::logic_error(operation + ": " + exc.what());    \
    } catch (...) {                                                           \
      throw;                                                                  \
    }                                                                         \
  }

#define CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(operation) \
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(operation)

/*
 * Helper function to ensure the exceptions generated on the mysqlx_connector
 * are properly translated to the corresponding shcore::Exception type
 */
#define CATCH_AND_TRANSLATE()                                   \
  catch (...) {                                                 \
    try {                                                       \
      throw;                                                    \
    } catch (mysqlshdk::db::Error & exc) {                      \
      throw shcore::Exception::mysql_error_with_code_and_state( \
          exc.what(), exc.code(), exc.sqlstate());              \
    } catch (std::runtime_error & exc) {                        \
      throw shcore::Exception::runtime_error(exc.what());       \
    } catch (std::logic_error & exc) {                          \
      throw shcore::Exception::logic_error(exc.what());         \
    } catch (...) {                                             \
      throw;                                                    \
    }                                                           \
  }

#endif

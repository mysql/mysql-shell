/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef _MOD_MYSQLXTEST_UTILS_H_
#define _MOD_MYSQLXTEST_UTILS_H_

#include "shellcore/types_cpp.h"
#include "shellcore/common.h"
#include "mysqlx.h"

#ifdef __GNUC__
#define ATTR_UNUSED __attribute__((unused))
#else
#define ATTR_UNUSED
#endif

/*
* Helper function to ensure the exceptions generated on the mysqlx_connector
* are properly translated to the corresponding shcore::Exception type
*/
static void ATTR_UNUSED translate_crud_exception(const std::string& operation)
{
  try
  {
    throw;
  }
  catch (shcore::Exception &e)
  {
    throw shcore::Exception::argument_error(operation + ": " + e.what());
  }
  catch (::mysqlx::Error &e)
  {
    throw shcore::Exception::error_with_code("MySQL Error", e.what(), e.error());
  }
  catch (std::runtime_error &e)
  {
    throw shcore::Exception::runtime_error(operation + ": " + e.what());
  }
  catch (std::logic_error &e)
  {
    throw shcore::Exception::logic_error(operation + ": " + e.what());
  }
  catch (...)
  {
    throw;
  }
}

#define CATCH_AND_TRANSLATE_CRUD_EXCEPTION(operation)   \
  catch (...)                   \
{ translate_crud_exception(operation); }

/*
* Helper function to ensure the exceptions generated on the mysqlx_connector
* are properly translated to the corresponding shcore::Exception type
*/
static void ATTR_UNUSED translate_exception()
{
  try
  {
    throw;
  }
  catch (::mysqlx::Error &e)
  {
    throw shcore::Exception::error_with_code("MySQL Error", e.what(), e.error());
  }
  catch (std::runtime_error &e)
  {
    throw shcore::Exception::runtime_error(e.what());
  }
  catch (std::logic_error &e)
  {
    throw shcore::Exception::logic_error(e.what());
  }
  catch (...)
  {
    throw;
  }
}

#define CATCH_AND_TRANSLATE()   \
  catch (...)                   \
{ translate_exception(); }

#endif

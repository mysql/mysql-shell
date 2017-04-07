/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_mysql_schema.h"
#include "mod_mysql_table.h"

#include "utils/utils_help.h"

using namespace std::placeholders;
using namespace mysqlsh;
using namespace mysqlsh::mysql;
using namespace shcore;

// Documentation of ClassicTable class
REGISTER_HELP(CLASSICTABLE_BRIEF, "Represents a ClassicTable on an ClassicSchema, retrieved with a session created using the MySQL Protocol.");

ClassicTable::ClassicTable(std::shared_ptr<ClassicSchema> owner, const std::string &name, bool is_view)
  : DatabaseObject(owner->_session.lock(), std::static_pointer_cast<DatabaseObject>(owner), name), _is_view(is_view) {
  init();
}

ClassicTable::ClassicTable(std::shared_ptr<const ClassicSchema> owner, const std::string &name, bool is_view)
  : DatabaseObject(owner->_session.lock(), std::const_pointer_cast<ClassicSchema>(owner), name), _is_view(is_view) {
  init();
}

ClassicTable::~ClassicTable() {}

void ClassicTable::init() {
  add_method("isView", std::bind(&ClassicTable::is_view_, this, _1), NULL);
}

// Documentation of isView function
REGISTER_HELP(CLASSICTABLE_ISVIEW_BRIEF, "Indicates whether this ClassicTable object represents a View on the database.");
REGISTER_HELP(CLASSICTABLE_ISVIEW_RETURNS, "@returns True if the Table represents a View on the database, False if represents a Table.");

/**
* $(CLASSICTABLE_ISVIEW_BRIEF)
*
* $(CLASSICTABLE_ISVIEW_RETURNS)
*/
#if DOXYGEN_JS
Bool ClassicTable::isView() {}
#elif DOXYGEN_PY
bool ClassicTable::is_view() {}
#endif
shcore::Value ClassicTable::is_view_(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("isView").c_str());

  return Value(_is_view);
}

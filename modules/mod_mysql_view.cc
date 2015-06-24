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

#include "mod_mysql_schema.h"
#include "mod_mysql_view.h"

using namespace mysh;
using namespace mysh::mysql;
using namespace shcore;

View::View(boost::shared_ptr<Schema> owner, const std::string &name)
: DatabaseObject(owner->_session.lock(), owner, name)
{
}

View::View(boost::shared_ptr<const Schema> owner, const std::string &name) :
DatabaseObject(owner->_session.lock(), boost::const_pointer_cast<Schema>(owner), name)
{
}

View::~View()
{
}
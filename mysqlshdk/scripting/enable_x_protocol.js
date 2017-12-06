/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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


function enableXProtocol()
{
  // check if mysqlx installed
  try
  {
    var mysqlx_port = session.runSql('select @@mysqlx_port').fetchOne();
    print("enableXProtocol: X Protocol plugin is already enabled and listening for connections on port "+mysqlx_port[0]+"\n");
    return 0;
  }
  catch (error)
  {
    if (error["code"] != 1193) // unknown system variable
    {
      print("enableXProtocol: Error checking for X Protocol plugin: "+error["message"]+"\n");
      return 1;
    }
  }

  print("enableXProtocol: Installing plugin mysqlx...\n");
  // if not installed, then check OS
  var os = session.runSql('select @@version_compile_os').fetchOne();
  if (os[0] == "Win32" || os[0] == "Win64")
  {
    session.runSql("install plugin mysqlx soname 'mysqlx.dll';");
  }
  else
  {
    session.runSql("install plugin mysqlx soname 'mysqlx.so';")
  }
  print("enableXProtocol: done\n");
}

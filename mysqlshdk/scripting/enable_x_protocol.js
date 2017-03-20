/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

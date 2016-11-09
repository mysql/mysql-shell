@echo off
rem Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.

rem This program is free software; you can redistribute it and/or modify
rem it under the terms of the GNU General Public License as published by
rem the Free Software Foundation; version 2 of the License.

rem This program is distributed in the hope that it will be useful,
rem but WITHOUT ANY WARRANTY; without even the implied warranty of
rem MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
rem GNU General Public License for more details.

rem You should have received a copy of the GNU General Public License
rem along with this program; if not, write to the Free Software
rem Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

rem retrieves the path to the location of mysqlprovision.cmd
set mypath=%~dp0

rem Executes the mysqlprovision module using the shell binary
"%mypath%mysqlsh" --py -f "%mypath%mysqlprovision.zip" %*
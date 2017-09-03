/*
* Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_MOD_COMMON_H_
#define MODULES_MOD_COMMON_H_

#ifdef _WIN32
# ifdef _DLL
#  ifdef mysqlshmods_EXPORTS
#   define MOD_PUBLIC __declspec(dllexport)
#  else
#   define MOD_PUBLIC __declspec(dllimport)
#  endif
# else
#  define MOD_PUBLIC
# endif
#else
# define MOD_PUBLIC
#endif

#ifdef DOXYGEN
// These data types are needed for documentation of the Dev-API
#define String std::string
#define Integer int
#define Bool bool
#define Map void
#define List void
#define Undefined void
#define Resultset void
#define Null void
#define CollectionFindRef void
#define ExecuteOptions int
#endif

#endif  // MODULES_MOD_COMMON_H_

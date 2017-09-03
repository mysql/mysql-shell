/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _TYPES_COMMON_H_
#define _TYPES_COMMON_H_

// TODO: This definition should be removed from here
// The one on mysqlshdk_exports.h should be used for symbol exports
#define TYPES_COMMON_PUBLIC

#ifdef No_mysqlshtypes
# undef TYPES_COMMON_PUBLIC
# define TYPES_COMMON_PUBLIC SHCORE_PUBLIC
#endif

#endif

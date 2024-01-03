/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#ifdef _WIN32
#include <Windows.h>
#endif

#include "include/my_sys.h"

// we need to ensure that mysys is properly initialized
namespace {

inline void init() { my_init(); }

inline void fini() { my_end(0); }

}  // namespace

#ifdef _WIN32

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason,
                               LPVOID reserved) {
  BOOL status = TRUE;

  switch (reason) {
    case DLL_PROCESS_ATTACH:
      init();
      break;

    case DLL_PROCESS_DETACH:
      fini();
      break;

    case DLL_THREAD_ATTACH:
      break;

    case DLL_THREAD_DETACH:
      break;
  }

  return status;
}

#else  // !_WIN32

namespace {

void __attribute__((constructor)) constructor() { init(); }

void __attribute__((destructor)) destructor() { fini(); }

}  // namespace

#endif  // !_WIN32

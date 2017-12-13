/*
  * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/utils/utils_stacktrace.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include <cstring>
#include <cstdio>

#if defined(__APPLE__) || defined(__GLIBC__)
#include <execinfo.h>
#include <cxxabi.h>
#elif defined(_WIN32)
#include <windows.h>
#include <Dbghelp.h>
#endif

#define MAX_STACK_DEPTH 512

namespace mysqlshdk {
namespace utils {

void init_stacktrace() {
}

#ifdef _WIN32

void print_stacktrace() {
  void *callstack[MAX_STACK_DEPTH];
  USHORT nframes = CaptureStackBackTrace(0, MAX_STACK_DEPTH, callstack, NULL);
  HANDLE process = GetCurrentProcess();
  SymInitialize(process, NULL, TRUE);
  SYMBOL_INFO *symbol = (SYMBOL_INFO*)malloc(sizeof(SYMBOL_INFO) + 1024);
  symbol->MaxNameLen = 1023;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

  fprintf(stderr, "Programmatically Printed Stacktrace:\n");
  for (auto i = 0; i < nframes; i++) {
    if (SymFromAddr(process, (DWORD64)callstack[i], NULL, symbol))
      fprintf(stderr, "\t%p\t%s\n", callstack[i], symbol->Name);
    else
      fprintf(stderr, "???\n");
  }
  free(symbol);
}

std::vector<std::string> get_stacktrace() {
  std::vector<std::string> stack;
  void *callstack[MAX_STACK_DEPTH];
  USHORT nframes = CaptureStackBackTrace(0, MAX_STACK_DEPTH, callstack, NULL);
  HANDLE process = GetCurrentProcess();
  SymInitialize(process, NULL, TRUE);
  SYMBOL_INFO *symbol = (SYMBOL_INFO*)malloc(sizeof(SYMBOL_INFO) + 1024);
  symbol->MaxNameLen = 1023;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

  fprintf(stderr, "Programmatically Printed Stacktrace:\n");
  for (auto i = 0; i < nframes; i++) {
    if (SymFromAddr(process, (DWORD64)callstack[i], NULL, symbol))
      stack.push_back(std::string(symbol->Name));
    else
      stack.push_back("???");
  }
  free(symbol);
  return stack;
}

#elif defined(__APPLE__) || defined(__GLIBC__)

// 0   executable     address    symbol + offset
static std::string demangle(const char *line) {
  std::vector<std::string> parts = shcore::str_split(line, " ", -1, true);
  if (parts.size() == 6) {
    int status;
    char *buffer =
        __cxxabiv1::__cxa_demangle(parts[3].c_str(), nullptr, nullptr, &status);
    if (status == 0) {
      std::string tmp(shcore::str_format("%-4s %s  %s + %s", parts[0].c_str(),
                                         parts[2].c_str(), buffer,
                                         parts[5].c_str()));
      if (buffer)
        free(buffer);
      return tmp;
    }
  }
  return line;
}

void print_stacktrace() {
  void *callstack[MAX_STACK_DEPTH];
  int nframes = backtrace(callstack, MAX_STACK_DEPTH);
  char **strs = backtrace_symbols(callstack, nframes);
  fprintf(stderr, "Programmatically Printed Stacktrace:\n");
  for (int i = 0; i < nframes; i++) {
    fprintf(stderr, "\t%s\n", demangle(strs[i]).c_str());
  }
  free(strs);
}

std::vector<std::string> get_stacktrace() {
  void *callstack[MAX_STACK_DEPTH];
  std::vector<std::string> stack;
  int nframes = backtrace(callstack, MAX_STACK_DEPTH);
  char **strs = backtrace_symbols(callstack, nframes);
  for (int i = 0; i < nframes; i++) {
    stack.push_back(demangle(strs[i]));
  }
  free(strs);
  return stack;
}

#else  // !__APPLE__

void print_stacktrace() {
}

std::vector<std::string> get_stacktrace() {
  return {};
}
#endif

}  // namespace utils
}  // namespace mysqlshdk

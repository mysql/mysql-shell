/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/textui/term_vt100.h"

#ifdef __sun
#include <sys/termios.h>
#include <termios.h>
#endif

#include <cstdio>
#include <string>
#ifndef WIN32
#include <sys/ioctl.h>
#include <unistd.h>
#else
#include <Windows.h>
#include <io.h>
#endif
#include <fcntl.h>

namespace mysqlshdk {
namespace vt100 {
#ifndef WIN32

namespace {

int tty_fd() {
  static int fd = -1;
  if (fd >= 0) return fd;
  const char *ttydev = ttyname(0);  // NOLINT(runtime/threadsafe_fn)
  // suggests ttyname_r which is not portable and not needed

  if (!ttydev) return -1;
  fd = open(ttydev, O_RDWR | O_NOCTTY);
  return fd;
}

}  // namespace

bool is_available() { return true; }

bool get_screen_size(int *rows, int *columns) {
  struct winsize size;
  if (ioctl(fileno(stdout), TIOCGWINSZ, &size) == 0) {
    // Happens in some cases (under gdb, in jenkins), even if the fd is a tty
    if (size.ws_row == 0 && size.ws_col == 0) return false;
    *rows = size.ws_row;
    *columns = size.ws_col;
    return true;
  }
  return false;
}

void send_escape(const char *s) {
  if (s) {
    int fd = tty_fd();
    write(fd, s, strlen(s));
  }
}

#if 0
bool read_escape(const char *terminator, char *buffer, size_t buflen) {
  size_t i = 0;
  size_t tlen = strlen(terminator);
  int fd = tty_fd();

  // read the prefix esc[
  if (read(fd, buffer, 2) < 2)
    return false;
  if (strncmp(buffer, "\x1b" "[", 2) != 0)
    return false;
  while (i < buflen) {
    if (read(fd, buffer + i, 1) < 1)
      return false;
    ++i;
    if (i >= tlen && strncmp(terminator, buffer + i - tlen, tlen) == 0)
      break;
  }
  return true;
}
#endif
#else  // WIN32

bool is_available() {
#ifdef ENABLE_VIRTUAL_TERMINAL_PROCESSING
  {
    DWORD mode = 0;
    GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode);

    if (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) {
      return true;
    }
  }
#endif  // ENABLE_VIRTUAL_TERMINAL_PROCESSING

  return false;
}

bool get_screen_size(int *rows, int *columns) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
    *columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    return true;
  }
  return false;
}

void send_escape(const char *s) {
  if (s) {
    DWORD written = 0;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), s, strlen(s), &written, nullptr);
  }
}

#if 0
bool read_escape(const char *terminator, char *buffer, size_t buflen) {
  // size_t i = 0;
  // size_t tlen = strlen(terminator);
  // int fd = tty_fd();
  // // read the prefix esc[
  // if (read(fd, buffer, 2) < 2)
  //   return false;
  // if (strncmp(buffer, "\x1b" "[", 2) != 0)
  //   return false;
  // while (i < buflen) {
  //   if (read(fd, buffer + i, 1) < 1)
  //     return false;
  //   ++i;
  //   if (i >= tlen && strncmp(terminator, buffer + i - tlen, tlen) == 0)
  //     break;
  // }
  // return true;
  return false;  // not implemented for windows
}
#endif

#endif

}  // namespace vt100
}  // namespace mysqlshdk

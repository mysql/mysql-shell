/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/textui/term_vt100.h"

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

int tty_fd() {
  static int fd = -1;
  if (fd >= 0)
    return fd;
  const char *ttydev = ttyname(0);  // NOLINT(runtime/threadsafe_fn)
  // suggests ttyname_r which is not portable and not needed

  if (!ttydev)
    return -1;
  fd = open(ttydev, O_RDWR | O_NOCTTY);
  return fd;
}

bool get_screen_size(int *rows, int *columns) {
  struct winsize size;
  if (ioctl(fileno(stdout), TIOCGWINSZ, &size) == 0) {
    // Happens in some cases (under gdb, in jenkins), even if the fd is a tty
    if (size.ws_row == 0 && size.ws_col == 0)
      return false;
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
  if (strncmp(buffer, "\e[", 2) != 0)
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
  // not implemented
}

#if 0
bool read_escape(const char *terminator, char *buffer, size_t buflen) {
  // size_t i = 0;
  // size_t tlen = strlen(terminator);
  // int fd = tty_fd();
  // // read the prefix esc[
  // if (read(fd, buffer, 2) < 2)
  //   return false;
  // if (strncmp(buffer, "\e[", 2) != 0)
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

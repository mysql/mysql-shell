/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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

#include <cassert>
#include <cstdio>
#include <string>
#ifndef _WIN32
#include <sys/ioctl.h>
#include <termios.h>
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

void set_echo(bool flag) {
  static int noecho_count = 0;
  static int old_flags = 0;

  if (flag) {
    assert(noecho_count > 0);
    if (noecho_count > 0) noecho_count--;
    if (noecho_count > 0) return;
  } else {
    noecho_count++;
    if (noecho_count > 1) return;
  }

  if (flag) {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag = old_flags;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
  } else {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    old_flags = tty.c_lflag;
    tty.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
  }
}

void send_escape(const char *s) {
  if (s) {
    int fd = tty_fd();
    const auto ignore = write(fd, s, strlen(s));
    (void)ignore;
  }
}

bool read_escape(const char *terminator, char *buffer, size_t buflen) {
  size_t i = 0;
  size_t tlen = strlen(terminator);
  int fd = tty_fd();

  // read the prefix esc[
  if (read(fd, buffer, 2) < 2) return false;
  if (strncmp(buffer, "\x1b[", 2) != 0) return false;
  while (i < buflen) {
    if (read(fd, buffer + i, 1) < 1) return false;
    ++i;
    if (i >= tlen && strncmp(terminator, buffer + i - tlen, tlen) == 0) break;
  }
  return true;
}

void query_cursor_position(int *row, int *column) {
  send_escape("\033[6n");
  char buffer[100];
  // Depending on the platform's implementation of sscanf this might cause a
  // crash if uninitialized data is used. To avoid it we memset the buffer with
  // null terminating char
  memset(buffer, '\0', sizeof(char) * 100);
  if (read_escape("R", buffer, sizeof(buffer) - 1)) {
    sscanf(buffer, "%i;%i", row, column);
  }
}

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

void set_echo(bool flag) {
  HANDLE console = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode;

  bool error = true;
  if (GetConsoleMode(console, &mode)) {
    DWORD new_mode =
        flag ? (mode | ENABLE_ECHO_INPUT) : (mode & ~ENABLE_ECHO_INPUT);

    error = false;
    if (new_mode != mode) {
      error = SetConsoleMode(console, mode);
    }
  }

  if (error) {
    // TODO(rennox): Should we really do anything in case of failure?
    // i.e. nothing seems to be done in linux..
  }
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

/**
 * TODO(rennox): This function was being used to get the cursor position
 * using terminal sequences, however the windows API has it's own function
 * for this purpose.
 *
 * Since this function might be required for other functios currently commented
 * in the header file, I'll let this implementation here with some annotations
 * Of what I found.
 */
bool read_escape(const char *terminator, char *buffer, size_t buflen) {
  size_t i = 0;
  size_t tlen = strlen(terminator);
  DWORD read;

  auto fd = GetStdHandle(STD_INPUT_HANDLE);

  /*
   * NOTE: Reading the response to a specific sequence is another sequence that
   * will be written into the console INPUT handle.
   *
   * The imput handle might come with 2 flags turned ON, these are:
   * - ENABLE_ECHO_INPUT: Which causes the input to be printed on the screen
   * buffer at the moment it is read
   * - ENABLE_LINE_INPUT: Which causes the Read operations to NOT return unless
   * a carriage return is found.
   *
   * My theory was that disabling such flags, I would be able to read from STDIN
   * without even if no carriage return was available and without printing the
   * read data on the screen buffer, so the alforithm to be implemented was: 1)
   * Backup the input mode 2) Disable both flags 3) Read the terminal sequence
   * 4) Restore the original input mode
   *
   * The problem I found is that even steps 1 and 2 "suceeded", the input mode
   * did not change at all, causing the read operation to not be doable as
   * expected.
   */

  // 1) Backup the input mode
  DWORD original_mode;
  if (!GetConsoleMode(fd, &original_mode)) return false;

  // 2) Disable ENABLE_LINE_INPUT and  ENABLE_ECHO_INPUT flags so we can:
  // - Read the sequence even not EOL is found
  // - Read the sequence without it getting printed on the screen
  bool restore = false;
  if (original_mode & (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT)) {
    DWORD read_mode{original_mode & ~(ENABLE_LINE_INPUT & ENABLE_ECHO_INPUT)};
    if (!SetConsoleMode(fd, read_mode)) return false;
    restore = true;
  }

  DWORD new_mode;
  // NOTE(rennox): Since the read below continued working as if the flags were
  // not cleaned up, I added this as a debug step, confirming that the input
  // mode was the original again.
  if (!GetConsoleMode(fd, &new_mode)) return false;

  // 3) Read the terminal sequence
  if (ReadFile(fd, buffer, 2, &read, nullptr)) {
    // read the prefix esc[
    if (read < 2) return false;
    if (strncmp(buffer, "\x1b[", 2) != 0) return false;

    while (i < buflen) {
      if (ReadFile(fd, buffer + 1, 1, &read, nullptr)) {
        if (read < 1) return false;
        ++i;
        if (i >= tlen && strncmp(terminator, buffer + i - tlen, tlen) == 0)
          break;
      } else {
        printf("Something went bad");
      }
    }
  } else {
    printf("Something went too bad");
  }

  // 4) Restore the original input mode
  if (restore && !SetConsoleMode(fd, original_mode)) return false;

  return true;
}

void query_cursor_position(int *row, int *column) {
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
  if (GetConsoleScreenBufferInfo(h, &bufferInfo)) {
    // The dwCursorPosition is coordinates in the screen buffer,
    // we need to return them in windows coordinates
    *row = bufferInfo.dwCursorPosition.Y - bufferInfo.srWindow.Top + 1;
    *column = bufferInfo.dwCursorPosition.X - bufferInfo.srWindow.Left;
  }
}

#endif

}  // namespace vt100
}  // namespace mysqlshdk

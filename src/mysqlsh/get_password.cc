/* Copyright (c) 2000, 2019, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms, as
   designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.
   This program is distributed in the hope that it will be useful,  but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
   the GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

/*
** Ask for a password from tty
** This is an own file to avoid conflicts with curses
*/

// modified to be standalone for mysqlsh

#include "my_config.h"

#if defined(HAVE_GETPASS) && !defined(__APPLE__)
// Don't use getpass() since it doesn't handle ^C (and it's deprecated)
// Except in macos, where it works
#undef HAVE_GETPASS
#endif

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <cstdio>
#include <cstring>

#include "mysqlsh/get_password.h"
#include "mysqlshdk/libs/utils/utils_general.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_GETPASS
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif  // HAVE_PWD_H
#else   // ! HAVE_GETPASS
#ifdef _WIN32
#include <conio.h>
#else  // ! _WIN32
#include <assert.h>
#include <fcntl.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#define TERMIO struct termios
#else  // ! HAVE_TERMIOS_H
#ifdef HAVE_TERMIO_H
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif  // HAVE_SYS_IOCTL_H
#include <termio.h>
#define TERMIO struct termio
#else  // ! HAVE_TERMIO_H
#include <sgtty.h>
#define TERMIO struct sgttyb
#endif  // ! HAVE_TERMIO_H
#endif  // ! HAVE_TERMIOS_H
#endif  // ! _WIN32
#endif  // ! HAVE_GETPASS

#ifdef HAVE_GETPASSPHRASE  // For Solaris
#define getpass(A) getpassphrase(A)
#endif  // HAVE_GETPASSPHRASE

namespace {

const size_t k_password_length = 80;

#ifndef HAVE_GETPASS
const char k_end_of_text = 3;  // Ctrl+C
#endif                         // ! HAVE_GETPASS

const char *get_password_prompt(const char *prompt) {
  return prompt ? prompt : "Enter password: ";
}

}  // namespace

#ifdef _WIN32
// we're just going to fake it here and get input from the keyboard
char *mysh_get_tty_password(const char *opt_message) {
  char to[k_password_length] = {0};
  char *pos = to, *end = to + sizeof(to) - 1;
  int tmp;

  _cputs(get_password_prompt(opt_message));

  for (;;) {
    tmp = _getch();

    // A second call must be read with control arrows
    // and control keys
    if (tmp == 0 || tmp == 0xE0) tmp = _getch();

    if (tmp == '\b' || tmp == 127) {
      if (pos != to) {
        _cputs("\b \b");
        pos--;
        continue;
      }
    }

    if (tmp == '\n' || tmp == '\r' || tmp == k_end_of_text) break;

    if (iscntrl(tmp) || pos == end) continue;

    _cputs("*");
    *(pos++) = tmp;
  }

  *pos = 0;

  _cputs("\n");

  char *ret_val = nullptr;
  if (tmp != k_end_of_text) {
    ret_val = strdup(to);
    // BUG#28915716: Cleans up the memory containing the password
    shcore::clear_buffer(to, k_password_length);
  }

  return ret_val;
}

#else  // !_WIN32
#ifndef HAVE_GETPASS

namespace {

class Disable_echo {
 public:
  explicit Disable_echo(int fd, bool close_fd = false)
      : m_fd{fd}, m_close_fd{close_fd} {
    TERMIO tmp;

#if defined(HAVE_TERMIOS_H)
    tcgetattr(m_fd, &m_original);
    tmp = m_original;
    tmp.c_lflag &= ~(ECHO | ISIG | ICANON);
    tmp.c_cc[VMIN] = 1;
    tmp.c_cc[VTIME] = 0;
    tcsetattr(m_fd, TCSADRAIN, &tmp);
#elif defined(HAVE_TERMIO_H)
    ioctl(m_fd, static_cast<int>(TCGETA), &m_original);
    tmp = m_original;
    tmp.c_lflag &= ~(ECHO | ISIG | ICANON);
    tmp.c_cc[VMIN] = 1;
    tmp.c_cc[VTIME] = 0;
    ioctl(m_fd, static_cast<int>(TCSETA), &tmp);
#else
    gtty(m_fd, &m_original);
    tmp = m_original;
    tmp.sg_flags &= ~ECHO;
    tmp.sg_flags |= RAW;
    stty(m_fd, &tmp);
#endif
  }

  ~Disable_echo() {
#if defined(HAVE_TERMIOS_H)
    tcsetattr(m_fd, TCSADRAIN, &m_original);
#elif defined(HAVE_TERMIO_H)
    ioctl(m_fd, static_cast<int>(TCSETA), &m_original);
#else
    stty(m_fd, &m_original);
#endif

    if (m_close_fd) {
      close(m_fd);
    }
  }

  Disable_echo(const Disable_echo &) = delete;
  Disable_echo(Disable_echo &&) = delete;
  Disable_echo &operator=(const Disable_echo &) = delete;
  Disable_echo &operator=(Disable_echo &&) = delete;

 private:
  int m_fd;
  bool m_close_fd;
  TERMIO m_original;
};

class Prompt_password {
 public:
  explicit Prompt_password(const char *msg) : m_msg{msg} { assert(m_msg); }

  bool get(char *buffer, int length) {
    setup_input_output();

    Disable_echo disabled{m_input_fd, m_close_input_fd};

    print(m_msg);
    bool ret = get_password(buffer, length);
    print('\n');

    return ret;
  }

 private:
  void setup_input_output() {
    const int fd = open("/dev/tty", O_RDWR | O_NOCTTY);

    if (fd < 0) {
      m_input_fd = fileno(stdin);
      m_output_fd = fileno(stderr);
      m_close_input_fd = false;
    } else {
      m_input_fd = m_output_fd = fd;
      m_close_input_fd = true;
    }

    m_echo = isatty(m_output_fd);
  }

  bool get_password(char *to, int length) const {
    char *pos = to, *end = to + length - 1;

    for (;;) {
      char tmp;

      if (read(m_input_fd, &tmp, 1) != 1) break;

      if (tmp == '\b' || static_cast<int>(tmp) == 127) {
        if (pos != to) {
          print("\b \b");
          pos--;
          continue;
        }
      }

      if (tmp == '\n' || tmp == '\r') break;

      if (tmp == k_end_of_text) return false;

      if (iscntrl(tmp) || pos == end) continue;

      print('*');
      *(pos++) = tmp;
    }

    *pos = 0;
    return true;
  }

  void print(const char *msg) const {
    if (m_echo) {
      write(m_output_fd, msg, strlen(msg));
    }
  }

  void print(const char c) const {
    if (m_echo) {
      write(m_output_fd, &c, 1);
    }
  }

  const char *m_msg = nullptr;
  bool m_echo = false;
  bool m_close_input_fd = false;
  int m_input_fd = -1;
  int m_output_fd = -1;
};

}  // namespace

#endif  // ! HAVE_GETPASS

char *mysh_get_tty_password(const char *opt_message) {
  char buff[k_password_length] = {0};
  const char *message = get_password_prompt(opt_message);

#ifdef HAVE_GETPASS
  char *passbuff = getpass(message);
  // copy the password to buff and clear original (static) buffer
  strncpy(buff, passbuff, sizeof(buff) - 1);
#ifdef _PASSWORD_LEN
  shcore::clear_buffer(passbuff, _PASSWORD_LEN);
#endif  // _PASSWORD_LEN
#else   // ! HAVE_GETPASS
  Prompt_password prompt{message};

  if (!prompt.get(buff, sizeof(buff))) {
    return nullptr;
  }
#endif
  char *ret_val = strdup(buff);
  shcore::clear_buffer(buff, k_password_length);
  return ret_val;
}

#endif  // !_WIN32

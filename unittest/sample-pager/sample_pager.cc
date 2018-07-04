/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstring>
#include <string>

#ifdef _WIN32

#include <io.h>

#include <BaseTsd.h>
using ssize_t = SSIZE_T;

#define open _open
#define read _read
#define write _write
#define close _close
#define fileno _fileno
#define O_CREAT _O_CREAT
#define O_WRONLY _O_WRONLY
#define O_APPEND _O_APPEND
#define O_TRUNC _O_TRUNC
#define S_IRUSR _S_IREAD
#define S_IWUSR _S_IWRITE
#define S_IRGRP _S_IREAD
#define S_IROTH _S_IREAD

#else  // !_WIN32

#include <unistd.h>

#endif  // !_WIN32

namespace {

class Pager {
 public:
  int run(int argc, char *argv[]) {
    if (!parse_args(argc, argv)) {
      usage(argv[0]);
      return 1;
    }

    if (!open_file()) {
      fprintf(stderr, "Failed to open file \"%s\" for writing.\n",
              m_file.c_str());
      return 1;
    }

    input_output();
    close_file();

    return 0;
  }

 private:
  static void usage(const char *exe) {
    fprintf(stderr, "Usage: %s [args] --file=/path/to/output/file\n\n", exe);
    fprintf(stderr, "Arguments:\n");
    fprintf(stderr, "  %s\tWrite stdin to specified file\n", k_file_command);
    fprintf(stderr, "  %s\tEcho stdin to stdout\n", k_echo_command);
    fprintf(stderr, "  %s\tAppend to file instead of overwriting it\n",
            k_append_command);
    fprintf(stderr, "  Unknown arguments are ignored.\n");
  }

  bool parse_args(int argc, char *argv[]) {
    const auto file_command_length = strlen(k_file_command);

    m_command_line = argv[0];

    for (int i = 1; i < argc; ++i) {
      if (strcmp(argv[i], k_echo_command) == 0) {
        m_echo = true;
      } else if (strcmp(argv[i], k_append_command) == 0) {
        m_append = true;
      } else if (strncmp(argv[i], k_file_command, file_command_length) == 0) {
        m_file = argv[i] + file_command_length;
      }

      m_command_line += std::string{" "} + argv[i];
    }

    return !m_file.empty();
  }

  bool open_file() {
    m_fd = ::open(m_file.c_str(),
                  O_CREAT | O_WRONLY | (m_append ? O_APPEND : O_TRUNC),
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    return m_fd >= 0;
  }

  void input_output() {
    {
      const std::string msg =
          "Running with arguments: " + m_command_line + "\n\n";
      write(msg.c_str(), msg.length());
    }

    char buffer[k_buffer_size];
    ssize_t bytes_read;

    while ((bytes_read = ::read(::fileno(stdin), buffer, k_buffer_size)) > 0) {
      write(buffer, bytes_read);
    }
  }

  void write(const char *data, size_t length) {
    ::write(m_fd, data, length);

    if (m_echo) {
      ::write(::fileno(stdout), data, length);
    }
  }

  void close_file() {
    if (m_fd >= 0) {
      ::close(m_fd);
      m_fd = -1;
    }
  }

  static constexpr auto k_echo_command = "--echo";
  static constexpr auto k_append_command = "--append";
  static constexpr auto k_file_command = "--file=";
  static constexpr size_t k_buffer_size = 512;

  std::string m_file;
  std::string m_command_line;
  bool m_echo = false;
  bool m_append = false;
  int m_fd = -1;
};

}  // namespace

int main(int argc, char *argv[]) { return Pager().run(argc, argv); }

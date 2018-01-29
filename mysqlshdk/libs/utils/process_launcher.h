/* Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.

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

#ifndef MYSQLSHDK_LIBS_UTILS_PROCESS_LAUNCHER_H_
#define MYSQLSHDK_LIBS_UTILS_PROCESS_LAUNCHER_H_

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#ifdef UNICODE
#undef UNICODE
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdint.h>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace shcore {
// Launches a process as child of current process and exposes the stdin & stdout
// of the child process (implemented thru pipelines) so the client of this class
// can read from the child's stdout and write to the child's stdin. For usage,
// see unit tests.
//
// TODO(fer): Make scenario 4 work correctly:
//   spawn
//   while not eof :
//     stdin.write
//     stdout.read
//   wait
class Process {
 public:
  /**
   * Creates a new process and launch it.
   * Argument 'args' must have a last entry that is NULL.
   * If redirect_stderr is true, the child's stderr is redirected to the same
   * stream than child's stdout.
   *
   * NOTE: if the called process is cmd.exe, additional
   * quoting would be required, which is currently not supported.
   * For that reason, a logic_error will be thrown if cmd.exe is argv[0]
   */
  explicit Process(const char *const *argv, bool redirect_stderr = true);

  ~Process() {
    if (is_alive) {
      close();
    }

#ifdef __APPLE__
    if (m_use_pseudo_tty) {
      stop_reader_thread();
    }
#endif  // __APPLE__
  }

#ifdef _WIN32
  void set_create_process_group() { create_process_group = true; }
#endif

  /** Launches the child process, and makes pipes available for read/write. */
  void start();

  /**
   * Reads a single line from stdout
   */
  std::string read_line(bool *eof = nullptr);

  /**
   * Reads the whole remaining output.
   */
  std::string read_all();

  /**
   * Read up to a 'count' bytes from the stdout of the child process.
   * This method blocks until the amount of bytes is read.
   * @param buf already allocated buffer where the read data will be stored.
   * @param count the maximum amount of bytes to read.
   * @return the real number of bytes read.
   * Returns an shcore::Exception in case of error when reading.
   */
  int read(char *buf, size_t count);

  /**
   * Writes several bytes into stdin of child process.
   * Returns an shcore::Exception in case of error when writing.
   */
  int write(const char *buf, size_t count);

  /**
   * Closes handle to the output stream, effectively sending an EOF to the
   * child process.
   */
  void finish_writing();

  /**
   * Writes bytes into terminal of the child process.
   *
   * This method is not available on Windows.
   *
   * @param buf Buffer containing bytes to be written.
   * @param count Size of the buffer.
   *
   * @return Number of bytes written.
   * @throws std::system_error in case of writing error.
   * @throws std::logic_error if enable_child_terminal() was not called before
   *         starting the child process
   */
  int write_to_terminal(const char *buf, size_t count);

  /**
   * Reads output from terminal of the child process.
   *
   * This method is not available on Windows.
   *
   * @return Number of bytes written.
   * @throws std::system_error in case of reading error.
   * @throws std::logic_error if enable_child_terminal() was not called before
   *         starting the child process
   */
  std::string read_from_terminal();

  /**
   * Kills the child process.
   */
  void kill();

  /**
   * Returns the child process handle.
   */
#ifdef _WIN32
  HANDLE get_pid();

  DWORD get_process_id() const { return pi.dwProcessId; }
#else
  pid_t get_pid();
#endif

  /**
   * Check whether the child process has already exited.
   * @return true if the process already exited.
   */
  bool check();

  /**
   * Wait for the child process to exists and returns its exit code.
   * If the child process is already dead, wait() it just returns.
   * Returns the exit code of the process. If the process was terminated
   * by a signal, returns 128 + SIGNAL
   */
  int wait();

  /**
   * The given file is going to be used as standard input of the launched
   * process. Needs to be called before starting the child process.
   *
   * @param input_file File to be redirected to standard input of the launched
   *        process.
   *
   * @throws std::runtime_error if input file does not exist
   * @throws std::logic_error if child process is already running
   */
  void redirect_file_to_stdin(const std::string &input_file);

  /**
   * Enables writing to child terminal using write_to_terminal(). Needs to be
   * called before staring the process.
   *
   * This method is not available on Windows.
   *
   * @throws std::logic_error if child process is already running
   */
  void enable_child_terminal();

  /** Perform Windows specific quoting of args and build a command line */
  static std::string make_windows_cmdline(const char *const *argv);

  /** Sets environment variables of new process.
   *
   * Needs to be called before start.
   * String need to be in format VARIABLE=<value>.
   */
  void set_environment(const std::vector<std::string> &env);

 private:
  /** Closes child process */
  void close();

  int do_read(char *buf, size_t count);

  bool is_write_allowed() const;

  void ensure_write_is_allowed() const;

  const char *const *argv;
  bool is_alive;
#ifdef WIN32
  HANDLE child_in_rd = nullptr;
  HANDLE child_in_wr = nullptr;
  HANDLE child_out_rd = nullptr;
  HANDLE child_out_wr = nullptr;
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  bool create_process_group = false;
  std::unique_ptr<char[]> new_environment;
#else
  pid_t childpid;
  int fd_in[2] = {-1, -1};
  int fd_out[2] = {-1, -1};
  int m_master_device = -1;
  bool m_use_pseudo_tty = false;
  std::vector<std::string> new_environment;
#endif
  int _pstatus = 0;
  bool _wait_pending = false;
  bool redirect_stderr;

  std::string m_input_file;

#ifdef __APPLE__
  void start_reader_thread() {
    if (!m_reader_thread) {
      m_reader_thread.reset(new std::thread{[this]() {
        static constexpr size_t k_buffer_size = 512;
        char buffer[k_buffer_size];
        int n = 0;
        while ((n = ::read(m_master_device, buffer, k_buffer_size)) > 0) {
          std::lock_guard<std::mutex> lock{m_read_buffer_mutex};
          m_read_buffer.insert(m_read_buffer.end(), buffer, buffer + n);
        }
      }});
    }
  }

  void stop_reader_thread() {
    if (m_reader_thread) {
      m_reader_thread->join();
      m_reader_thread.reset(nullptr);
    }
  }

  std::unique_ptr<std::thread> m_reader_thread;
  std::mutex m_read_buffer_mutex;
  std::deque<char> m_read_buffer;
#endif  // __APPLE__
};

using Process_launcher = Process;

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_PROCESS_LAUNCHER_H_

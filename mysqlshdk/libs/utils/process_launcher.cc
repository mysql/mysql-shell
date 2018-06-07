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

#include "mysqlshdk/libs/utils/process_launcher.h"

#include <algorithm>
#include <cassert>
#include <mutex>
#include <string>
#include <system_error>

#ifdef WIN32
#include <stdio.h>
#include <tchar.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#endif

#ifdef LINUX
#include <sys/prctl.h>
#endif

// openpty():
#ifdef __FreeBSD__
#include <libutil.h>
#elif defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
#include <util.h>
#elif !defined(WIN32)
#include <pty.h>
#endif

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace {

/**
 * Throws an exception with the specified message, if msg == NULL, the
 * exception's message is specific of the platform error. (errno in Linux /
 * GetLastError in Windows).
 */
void report_error(const char *msg) {
#ifdef WIN32
  DWORD error_code = GetLastError();
#else
  int error_code = errno;
#endif

  if (msg == nullptr) {
    throw std::system_error(error_code, std::generic_category(),
                            shcore::get_last_error());
  } else {
    throw std::system_error(error_code, std::generic_category(), msg);
  }
}

#ifndef WIN32

int write_fd(int fd, const char *buf, size_t count) {
  int n;
  if ((n = ::write(fd, buf, count)) >= 0) return n;
  if (errno == EPIPE) return 0;
  report_error(NULL);
  return -1;
}

void change_controlling_terminal(int new_ctty) {
  // detach from current controlling terminal (if there's one)
  int fd = open("/dev/tty", O_RDWR | O_NOCTTY);

  if (fd >= 0) {
    if (ioctl(fd, TIOCNOTTY, 0) < 0) {
      report_error("ioctl(TIOCNOTTY)");
    }

    ::close(fd);
  }

  // move process to a new session
  if (setsid() < 0) {
    // EPERM means process was already a session leader - not an error;
    if (EPERM != errno) {
      report_error("setsid()");
    }
  }

  // make sure there is no controlling terminal
  fd = open("/dev/tty", O_RDWR | O_NOCTTY);
  if (fd >= 0) {
    ::close(fd);
    report_error("Process should not have controlling terminal");
  }

  // make pseudo terminal the new controlling terminal
  if (ioctl(new_ctty, TIOCSCTTY, 0) < 0) {
    report_error("ioctl(TIOCSCTTY)");
  }

  char device_name[255];

  if (ttyname_r(new_ctty, device_name, sizeof(device_name)) != 0) {
    report_error("ttyname_r()");
  }

  // open tty, this will also set the controlling terminal
  fd = open(device_name, O_RDONLY);
  if (fd < 0) {
    report_error("open() slave by name");
  } else {
    ::close(fd);
  }

  // verify if everything's OK
  fd = open("/dev/tty", O_RDONLY | O_NOCTTY);
  if (fd < 0) {
    report_error("open() controlling terminal");
  } else {
    ::close(fd);
  }
}

void redirect_fd(int from, int to) {
  while (dup2(from, to) == -1) {
    if (errno == EINTR)
      continue;
    else
      report_error("redirect_fd()");
  }
}

void redirect_input_output(int fd_in[2], int fd_out[2], bool redirect_stderr) {
  // connect pipes to stdin, stdout and stderr
  ::close(fd_out[0]);
  ::close(fd_in[1]);

  redirect_fd(fd_out[1], STDOUT_FILENO);

  if (redirect_stderr) {
    redirect_fd(fd_out[1], STDERR_FILENO);
  }

  redirect_fd(fd_in[0], STDIN_FILENO);

  fcntl(fd_out[1], F_SETFD, FD_CLOEXEC);
  fcntl(fd_in[0], F_SETFD, FD_CLOEXEC);
}

#endif

}  // namespace

namespace shcore {

Process::Process(const char *const *argv, bool redirect_stderr)
    : is_alive(false) {
#ifdef WIN32
  if (strstr(argv[0], "cmd.exe"))
    throw std::logic_error("launching cmd.exe currently not supported");
// To suport cmd.exe, we need to handle special quoting rules
// required by it
#endif
  this->argv = argv;
  this->redirect_stderr = redirect_stderr;
}

/** Joins list of strings into a command line that is suitable for Windows

  https://msdn.microsoft.com/en-us/library/17w5ykft(v=vs.85).aspx
 */
std::string Process::make_windows_cmdline(const char *const *argv) {
  assert(argv[0]);
  std::string cmd = argv[0];

  for (int i = 1; argv[i]; i++) {
    cmd.push_back(' ');
    // if there are any whitespaces or quotes in the arg, we need to quote
    if (strcspn(argv[i], "\" \t\n\r") < strlen(argv[i])) {
      cmd.push_back('"');
      for (const char *s = argv[i]; *s; ++s) {
        // The trick is that backslashes behave different when they have a "
        // after them from when they don't
        int nbackslashes = 0;
        while (*s == '\\') {
          nbackslashes++;
          s++;
        }
        if (nbackslashes > 0) {
          if (*(s + 1) == '"' || *(s + 1) == '\0') {
            // if backslashes appear before a ", they need to be escaped
            // if a backslsh appears before the end of the string
            // the next char will be the final ", so we also need to escape
            // them
            cmd.append(nbackslashes * 2, '\\');
          } else {
            // otherwise, just add them as singles
            cmd.append(nbackslashes, '\\');
          }
        }
        // if a quote appears, we have to escape it
        if (*s == '"') {
          cmd.append("\\\"");
        } else {
          cmd.push_back(*s);
        }
      }
      cmd.push_back('"');
    } else {
      cmd.append(argv[i]);
    }
  }
  return cmd;
}

bool Process::is_write_allowed() const { return m_input_file.empty(); }

void Process::ensure_write_is_allowed() const {
  if (!is_write_allowed()) {
    throw std::logic_error(
        "Writing is not allowed as process is using file "
        "for input");
  }
}

void Process::redirect_file_to_stdin(const std::string &input_file) {
  if (is_alive) {
    throw std::logic_error(
        "This method needs to be called before starting "
        "the child process.");
  }

  if (!shcore::file_exists(input_file)) {
    throw std::runtime_error(input_file + ": Input file does not exist");
  }

  m_input_file = input_file;
}

#ifdef WIN32

void Process::start() {
  SECURITY_ATTRIBUTES saAttr;

  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  if (!CreatePipe(&child_out_rd, &child_out_wr, &saAttr, 0))
    report_error("Failed to create child_out_rd");

  if (!SetHandleInformation(child_out_rd, HANDLE_FLAG_INHERIT, 0))
    report_error("Failed to create child_out_rd");

  // force non blocking IO in Windows
  DWORD mode = PIPE_NOWAIT;
  // BOOL res = SetNamedPipeHandleState(child_out_rd, &mode, NULL, NULL);

  if (is_write_allowed()) {
    if (!CreatePipe(&child_in_rd, &child_in_wr, &saAttr, 0))
      report_error("Failed to create child_in pipe");

    if (!SetHandleInformation(child_in_wr, HANDLE_FLAG_INHERIT, 0))
      report_error("Failed to disable inheritance of child_in_wr");
  } else {
    child_in_rd = CreateFile(m_input_file.c_str(), GENERIC_READ, 0, &saAttr,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (INVALID_HANDLE_VALUE == child_in_rd) {
      report_error("CreateFile(m_input_file)");
    }
  }

  // Create Process
  std::string cmd = make_windows_cmdline(argv);
  LPTSTR lp_cmd = const_cast<char *>(cmd.c_str());
  DWORD creation_flags = 0;
  BOOL bSuccess = FALSE;

  if (create_process_group) creation_flags |= CREATE_NEW_PROCESS_GROUP;

  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  if (redirect_stderr) si.hStdError = child_out_wr;
  si.hStdOutput = child_out_wr;
  si.hStdInput = child_in_rd;
  si.dwFlags |= STARTF_USESTDHANDLES;

  bSuccess = CreateProcess(NULL,            // lpApplicationName
                           lp_cmd,          // lpCommandLine
                           NULL,            // lpProcessAttributes
                           NULL,            // lpThreadAttributes
                           TRUE,            // bInheritHandles
                           creation_flags,  // dwCreationFlags
                           NULL,            // lpEnvironment
                           NULL,            // lpCurrentDirectory
                           &si,             // lpStartupInfo
                           &pi);            // lpProcessInformation

  if (!bSuccess) {
    report_error(NULL);
  } else {
    is_alive = true;
    _wait_pending = true;
  }
  CloseHandle(child_out_wr);
  CloseHandle(child_in_rd);

  // DWORD res1 = WaitForInputIdle(pi.hProcess, 100);
  // res1 = WaitForSingleObject(pi.hThread, 100);
}

HANDLE Process::get_pid() { return pi.hProcess; }

bool Process::check() {
  DWORD dwExit = 0;
  if (!_wait_pending) return true;
  if (GetExitCodeProcess(pi.hProcess, &dwExit)) {
    if (dwExit == STILL_ACTIVE) return false;
    _wait_pending = false;
    _pstatus = dwExit;
    return true;
  }
  _wait_pending = false;
  assert(0);
  return true;
}

int Process::wait() {
  if (!_wait_pending) return _pstatus;
  DWORD dwExit = 0;

  // wait_loop will be set to true only if the process is
  // found to continue active (STILL_ACTIVE)
  bool wait_loop = false;

  do {
    if (GetExitCodeProcess(pi.hProcess, &dwExit)) {
      wait_loop = (dwExit == STILL_ACTIVE);
      if (wait_loop) WaitForSingleObject(pi.hProcess, INFINITE);
    } else {
      DWORD dwError = GetLastError();
      if (dwError != ERROR_INVALID_HANDLE)  // not closed already?
        report_error(NULL);
      else
        dwExit = 128;  // Invalid handle
    }
  } while (wait_loop);

  return dwExit;
}

void Process::close() {
  DWORD dwExit;
  if (GetExitCodeProcess(pi.hProcess, &dwExit)) {
    if (dwExit == STILL_ACTIVE) {
      if (!TerminateProcess(pi.hProcess, 0)) report_error(NULL);
      // TerminateProcess is async, wait for process to end.
      WaitForSingleObject(pi.hProcess, INFINITE);
    }
  } else {
    if (is_alive) report_error(NULL);
  }

  if (!CloseHandle(pi.hProcess)) report_error(NULL);
  if (!CloseHandle(pi.hThread)) report_error(NULL);

  if (child_out_rd && !CloseHandle(child_out_rd)) report_error(NULL);
  if (child_in_wr && !CloseHandle(child_in_wr)) report_error(NULL);

  is_alive = false;
}

int Process::do_read(char *buf, size_t count) {
  BOOL bSuccess = FALSE;
  DWORD dwBytesRead, dwCode;
  int i = 0;

  while (!(bSuccess = ReadFile(child_out_rd, buf, count, &dwBytesRead, NULL))) {
    dwCode = GetLastError();
    if (dwCode == ERROR_NO_DATA) continue;
    if (dwCode == ERROR_BROKEN_PIPE)
      return EOF;
    else
      report_error(NULL);
  }

  return dwBytesRead;
}

int Process::write(const char *buf, size_t count) {
  ensure_write_is_allowed();

  DWORD dwBytesWritten;
  BOOL bSuccess = FALSE;
  bSuccess = WriteFile(child_in_wr, buf, count, &dwBytesWritten, NULL);
  if (!bSuccess) {
    if (GetLastError() != ERROR_NO_DATA)  // otherwise child process just died.
      report_error(NULL);
  } else {
    // When child input buffer is full, this returns zero in NO_WAIT mode.
    return dwBytesWritten;
  }
  return 0;  // so the compiler does not cry
}

void Process::finish_writing() {
  ensure_write_is_allowed();
  if (child_in_wr) {
    if (!CloseHandle(child_in_wr)) {
      report_error(NULL);
    }
    child_in_wr = nullptr;
  }
}

void Process::enable_child_terminal() {
  throw std::runtime_error("This method is not available on Windows.");
}

int Process::write_to_terminal(const char *buf, size_t count) {
  throw std::runtime_error("This method is not available on Windows.");
}

std::string Process::read_from_terminal() {
  throw std::runtime_error("This method is not available on Windows.");
}

#else  // !WIN32

void Process::start() {
  assert(!is_alive);

  if (is_write_allowed()) {
    if (pipe(fd_in) < 0) {
      report_error("pipe(fd_in)");
    }
  } else {
    fd_in[0] = open(m_input_file.c_str(), O_RDONLY);
  }

  if (pipe(fd_out) < 0) {
    ::close(fd_in[0]);
    ::close(fd_in[1]);
    report_error("pipe(fd_out)");
  }

  int slave_device = 0;

  if (m_use_pseudo_tty) {
    // create pseudo terminal which is going to be used as controlling terminal
    // of the new process
    if (openpty(&m_master_device, &slave_device, nullptr, nullptr, nullptr) <
        0) {
      ::close(fd_in[0]);
      ::close(fd_in[1]);
      ::close(fd_out[0]);
      ::close(fd_out[1]);
      report_error("openpty()");
    }
  }

  childpid = fork();
  if (childpid == -1) {
    ::close(fd_in[0]);
    ::close(fd_in[1]);
    ::close(fd_out[0]);
    ::close(fd_out[1]);
    ::close(m_master_device);
    ::close(slave_device);
    report_error("fork()");
  }

  if (childpid == 0) {
#ifdef LINUX
    prctl(PR_SET_PDEATHSIG, SIGHUP);
#endif

    try {
      if (m_use_pseudo_tty) {
        change_controlling_terminal(slave_device);

        // close the file descriptors, they're not needed anymore
        ::close(m_master_device);
        ::close(slave_device);
      }

      redirect_input_output(fd_in, fd_out, redirect_stderr);
    } catch (std::exception &e) {
      fprintf(stderr, "Exception while preparing the child process: %s\n",
              e.what());
      fflush(stderr);
      exit(128);
    }

    execvp(argv[0], (char *const *)argv);

    int my_errno = errno;
    fprintf(stderr, "%s could not be executed: %s (errno %d)\n", argv[0],
            strerror(my_errno), my_errno);
    fflush(stderr);

    // we need to identify an ENOENT and since some programs return 2 as
    // exit-code we need to return a non-existent code, 128 is a general
    // convention used to indicate a failure to execute another program in a
    // subprocess
    if (my_errno == 2) my_errno = 128;

    exit(my_errno);
  } else {
    ::close(fd_out[1]);
    ::close(fd_in[0]);

    if (m_use_pseudo_tty) {
      ::close(slave_device);

#ifdef __APPLE__
      // need to drain output from controlling terminal, otherwise launched
      // process will hang on exit
      start_reader_thread();
#endif  // __APPLE__
    }

    is_alive = true;
    _wait_pending = true;
  }
}

void Process::close() {
  is_alive = false;
  ::close(fd_out[0]);
  if (fd_in[1] >= 0) ::close(fd_in[1]);
  if (m_master_device >= 0) ::close(m_master_device);

  if (::kill(childpid, SIGTERM) < 0 && errno != ESRCH) report_error(NULL);
  if (errno != ESRCH) {
    sleep(1);
    if (::kill(childpid, SIGKILL) < 0 && errno != ESRCH) report_error(NULL);
  }
}

int Process::do_read(char *buf, size_t count) {
  int n;
  do {
    if ((n = ::read(fd_out[0], buf, count)) >= 0) return n;
    if (errno == EAGAIN || errno == EINTR) continue;
    if (errno == EPIPE) return 0;
    break;
  } while (true);
  report_error(NULL);
  return -1;
}

int Process::write(const char *buf, size_t count) {
  ensure_write_is_allowed();

  return write_fd(fd_in[1], buf, count);
}

void Process::finish_writing() {
  ensure_write_is_allowed();

  if (fd_in[1] >= 0) {
    ::close(fd_in[1]);
    fd_in[1] = -1;
  }
}

void Process::enable_child_terminal() {
  if (is_alive) {
    throw std::logic_error(
        "This method needs to be called before starting "
        "the child process.");
  }

  m_use_pseudo_tty = true;
}

int Process::write_to_terminal(const char *buf, size_t count) {
  if (!m_use_pseudo_tty) {
    throw std::logic_error(
        "Call enable_child_terminal() before calling this "
        "method.");
  }

  return write_fd(m_master_device, buf, count);
}

std::string Process::read_from_terminal() {
  if (!m_use_pseudo_tty) {
    throw std::logic_error(
        "Call enable_child_terminal() before calling this "
        "method.");
  }
#ifdef __APPLE__
  bool has_output = false;

  while (!has_output) {
    {
      std::lock_guard<std::mutex> lock{m_read_buffer_mutex};
      has_output = !m_read_buffer.empty();
    }

    if (!has_output) {
      shcore::sleep_ms(10);
    }
  }

  std::string output;

  {
    std::lock_guard<std::mutex> lock{m_read_buffer_mutex};
    output = {m_read_buffer.begin(), m_read_buffer.end()};
    m_read_buffer.clear();
  }

  return output;
#else   // ! __APPLE__
  fd_set fd_in;
  char buffer[512];
  struct timeval timeout;
  int64_t retry_count = 0;

  do {
    timeout.tv_sec = 0;
    timeout.tv_usec = 100 * 1000;

    FD_ZERO(&fd_in);
    FD_SET(m_master_device, &fd_in);

    int ret = ::select(m_master_device + 1, &fd_in, nullptr, nullptr, &timeout);

    if (ret == -1) {
      report_error(nullptr);
    } else if (ret == 0) {
      report_error("Timeout on select()");
    } else {
      int n = ::read(m_master_device, buffer, sizeof(buffer));

      if (n >= 0) {
        return std::string(buffer, n);
      } else if (errno == EIO) {
        // This error means that the other side of pseudo terminal is closed.
        // Since child does not duplicate it to stdin/stdout/stderr, it will
        // remain closed until the child process opens the controlling terminal
        // (i.e. writes the password prompt and waits for user input).
        if (++retry_count > 1000000) {
          report_error("Controlling terminal seems to be closed");
        } else {
          continue;
        }
      } else {
        report_error(nullptr);
      }
    }
  } while (true);

  return "";
#endif  // ! __APPLE__
}

pid_t Process::get_pid() { return childpid; }

bool Process::check() {
  if (!_wait_pending) return true;
  if (waitpid(childpid, &_pstatus, WNOHANG) <= 0) return false;
  _wait_pending = false;
  return true;
}

/*
 * Waits for the child process to finish.
 * throws an error if the wait fails, or the child return code if wait syscall
 * does not fail.
 */
int Process::wait() {
  int ret;

  if (_wait_pending) {
    do {
      ret = ::waitpid(childpid, &_pstatus, 0);
      if (ret == -1) {
        if (errno == ECHILD) {
          assert(0);
          _pstatus = -1;
          break;  // no children left
        }
      }
    } while (ret == -1);
  }
  _wait_pending = false;
  assert(WIFEXITED(_pstatus) || WIFSIGNALED(_pstatus));
  if (WIFEXITED(_pstatus))
    return WEXITSTATUS(_pstatus);
  else
    return WTERMSIG(_pstatus) + 128;
}

#endif  // !_WIN32

void Process::kill() { close(); }

int Process::read(char *buf, size_t count) { return do_read(buf, count); }

std::string Process::read_line(bool *eof) {
  std::string s;
  int c = 0;
  char buf[1];
  while ((s.empty() || s.back() != '\n') && (c = read(buf, 1)) > 0) {
    s.push_back(buf[0]);
  }
  if (c <= 0 && eof) *eof = true;
  return s;
}

std::string Process::read_all() {
  static constexpr size_t k_buffer_size = 512;
  std::string s;
  char buffer[k_buffer_size];
  int c = 0;

  while ((c = read(buffer, k_buffer_size)) > 0) {
    s += std::string(buffer, c);
  }

  return s;
}

}  // namespace shcore

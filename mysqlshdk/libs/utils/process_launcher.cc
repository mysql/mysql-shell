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
#include <string>
#include <mutex>
#include <system_error>

#ifdef WIN32
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifdef LINUX
#include <sys/prctl.h>
#endif

#include "mysqlshdk/libs/utils/utils_string.h"

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
std::string Process::make_windows_cmdline(const char * const*argv) {
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

void Process::start_output_reader() {
  if (_reader_thread)
    throw std::logic_error("start_output_reader() was already called");

  _reader_thread.reset(new std::thread([this]() {
    char c;
    try {
      while (do_read(&c, 1) > 0) {
        std::lock_guard<std::mutex> lock(_read_buffer_mutex);
        _read_buffer.push_back(c);
      }
    } catch (std::system_error) {
    }
  }));
}

void Process::stop_output_reader() {
  if (_reader_thread) {
    // This function is supposed to be called after close() kills the process
    _reader_thread->join();
    _reader_thread.reset();
  }
}

bool Process::has_output(bool full_line) {
  if (!_reader_thread)
    throw std::logic_error("start_output_reader() must be called first");

  std::lock_guard<std::mutex> lock(_read_buffer_mutex);
  if (full_line) {
    return (std::find(_read_buffer.begin(), _read_buffer.end(), '\n') !=
            _read_buffer.end());
  } else {
    return !_read_buffer.empty();
  }
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

  if (!CreatePipe(&child_in_rd, &child_in_wr, &saAttr, 0))
    report_error("Failed to create child_in_rd");

  if (!SetHandleInformation(child_in_wr, HANDLE_FLAG_INHERIT, 0))
    report_error("Failed to created child_in_wr");

  // Create Process
  std::string cmd = make_windows_cmdline(argv);
  LPTSTR lp_cmd = const_cast<char *>(cmd.c_str());
  DWORD creation_flags = 0;
  BOOL bSuccess = FALSE;

  if (create_process_group)
    creation_flags |= CREATE_NEW_PROCESS_GROUP;

  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  if (redirect_stderr)
    si.hStdError = child_out_wr;
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

  //DWORD res1 = WaitForInputIdle(pi.hProcess, 100);
  //res1 = WaitForSingleObject(pi.hThread, 100);
}

HANDLE Process::get_pid() {
  return pi.hProcess;
}

bool Process::check() {
  DWORD dwExit = 0;
  if (!_wait_pending)
    return true;
  if (GetExitCodeProcess(pi.hProcess, &dwExit)) {
    if (dwExit == STILL_ACTIVE)
      return false;
    _wait_pending = false;
    _pstatus = dwExit;
    return true;
  }
  _wait_pending = false;
  assert(0);
  return true;
}

int Process::wait() {
  if (!_wait_pending)
    return _pstatus;
  DWORD dwExit = 0;
  if (GetExitCodeProcess(pi.hProcess, &dwExit)) {
    if (dwExit == STILL_ACTIVE) {
      WaitForSingleObject(pi.hProcess, INFINITE);
    }
  } else {
    DWORD dwError = GetLastError();
    if (dwError != ERROR_INVALID_HANDLE)  // not closed already?
      report_error(NULL);
    else
      dwExit = 128;  // Invalid handle
  }
  return dwExit;
}

void Process::close() {
  DWORD dwExit;
  if (GetExitCodeProcess(pi.hProcess, &dwExit)) {
    if (dwExit == STILL_ACTIVE) {
      if (!TerminateProcess(pi.hProcess, 0))
        report_error(NULL);
      // TerminateProcess is async, wait for process to end.
      WaitForSingleObject(pi.hProcess, INFINITE);
    }
  } else {
    if (is_alive)
      report_error(NULL);
  }

  if (!CloseHandle(pi.hProcess))
    report_error(NULL);
  if (!CloseHandle(pi.hThread))
    report_error(NULL);

  if (!CloseHandle(child_out_rd))
    report_error(NULL);
  if (child_in_wr && !CloseHandle(child_in_wr))
    report_error(NULL);

  is_alive = false;
}

int Process::do_read(char *buf, size_t count) {
  BOOL bSuccess = FALSE;
  DWORD dwBytesRead, dwCode;
  int i = 0;

  while (!(bSuccess = ReadFile(child_out_rd, buf, count, &dwBytesRead, NULL))) {
    dwCode = GetLastError();
    if (dwCode == ERROR_NO_DATA)
      continue;
    if (dwCode == ERROR_BROKEN_PIPE)
      return EOF;
    else
      report_error(NULL);
  }

  return dwBytesRead;
}

int Process::write(const char *buf, size_t count) {
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

void Process::close_write_fd() {
  if (child_in_wr)
    CloseHandle(child_in_wr);
  child_in_wr = 0;
}

void Process::report_error(const char *msg) {
  DWORD dwCode = GetLastError();
  LPTSTR lpMsgBuf;

  if (msg != NULL) {
    throw std::system_error(dwCode, std::generic_category(), msg);
  } else {
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, dwCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&lpMsgBuf, 0, NULL);
    std::string msgerr = "SystemError: ";
    msgerr += lpMsgBuf;
    msgerr += " with error code %d.";
    std::string fmt = str_format(msgerr.c_str(), dwCode);
    throw std::system_error(dwCode, std::generic_category(), fmt);
  }
}

HANDLE Process::get_fd_write() {
  return child_in_wr;
}

HANDLE Process::get_fd_read() {
  return child_out_rd;
}

#else  // !WIN32

void Process::start() {
  assert(!is_alive);

  if (pipe(fd_in) < 0) {
    report_error(NULL);
  }
  if (pipe(fd_out) < 0) {
    ::close(fd_in[0]);
    ::close(fd_in[1]);
    report_error(NULL);
  }

  childpid = fork();
  if (childpid == -1) {
    ::close(fd_in[0]);
    ::close(fd_in[1]);
    ::close(fd_out[0]);
    ::close(fd_out[1]);
    report_error(NULL);
  }

  if (childpid == 0) {
#ifdef LINUX
    prctl(PR_SET_PDEATHSIG, SIGHUP);
#endif

    ::close(fd_out[0]);
    ::close(fd_in[1]);
    while (dup2(fd_out[1], STDOUT_FILENO) == -1) {
      if (errno == EINTR)
        continue;
      else
        report_error(NULL);
    }

    if (redirect_stderr) {
      while (dup2(fd_out[1], STDERR_FILENO) == -1) {
        if (errno == EINTR)
          continue;
        else
          report_error(NULL);
      }
    }
    while (dup2(fd_in[0], STDIN_FILENO) == -1) {
      if (errno == EINTR)
        continue;
      else
        report_error(NULL);
    }

    fcntl(fd_out[1], F_SETFD, FD_CLOEXEC);
    fcntl(fd_in[0], F_SETFD, FD_CLOEXEC);

    execvp(argv[0], (char * const *)argv);

    int my_errno = errno;
    fprintf(stderr, "%s could not be executed: %s (errno %d)\n", argv[0],
            strerror(my_errno), my_errno);

    // we need to identify an ENOENT and since some programs return 2 as
    // exit-code we need to return a non-existent code, 128 is a general
    // convention used to indicate a failure to execute another program in a
    // subprocess
    if (my_errno == 2)
      my_errno = 128;

    exit(my_errno);
  } else {
    ::close(fd_out[1]);
    ::close(fd_in[0]);
    is_alive = true;
    _wait_pending = true;
  }
}

void Process::close() {
  is_alive = false;
  ::close(fd_out[0]);
  if (fd_in[1] >= 0)
    ::close(fd_in[1]);

  if (::kill(childpid, SIGTERM) < 0 && errno != ESRCH)
    report_error(NULL);
  if (errno != ESRCH) {
    sleep(1);
    if (::kill(childpid, SIGKILL) < 0 && errno != ESRCH)
      report_error(NULL);
  }
}

int Process::do_read(char *buf, size_t count) {
  int n;
  do {
    if ((n = ::read(fd_out[0], buf, count)) >= 0)
      return n;
    if (errno == EAGAIN || errno == EINTR)
      continue;
    if (errno == EPIPE)
      return 0;
    break;
  } while (true);
  report_error(NULL);
  return -1;
}

int Process::write(const char *buf, size_t count) {
  int n;
  if ((n = ::write(fd_in[1], buf, count)) >= 0)
    return n;
  if (errno == EPIPE)
    return 0;
  report_error(NULL);
  return -1;
}

void Process::close_write_fd() {
  if (fd_in[1] >= 0)
    ::close(fd_in[1]);
  fd_in[1] = -1;
}

void Process::report_error(const char *msg) {
  char sys_err[128];
  int errnum = errno;
  if (msg == NULL) {
    strerror_r(errno, sys_err, sizeof(sys_err));
    std::string s = sys_err;
    s += " with errno %d.";
    std::string fmt = str_format(s.c_str(), errnum);
    throw std::system_error(errnum, std::generic_category(), fmt);
  } else {
    throw std::system_error(errnum, std::generic_category(), msg);
  }
}

pid_t Process::get_pid() {
  return childpid;
}

bool Process::check() {
  if (!_wait_pending)
    return true;
  if (waitpid(childpid, &_pstatus, WNOHANG) <= 0)
    return false;
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

int Process::get_fd_write() {
  return fd_in[1];
}

int Process::get_fd_read() {
  return fd_out[0];
}

#endif  // !_WIN32

void Process::kill() {
  close();
}

int Process::read(char *buf, size_t count) {
  if (_reader_thread) {
    std::lock_guard<std::mutex> lock(_read_buffer_mutex);
    count = std::min(_read_buffer.size(), count);
    std::copy(_read_buffer.begin(), _read_buffer.begin() + count, buf);
    _read_buffer.erase(_read_buffer.begin(), _read_buffer.begin() + count);
    return static_cast<int>(count);
  } else {
    return do_read(buf, count);
  }
}

std::string Process::read_line(bool *eof) {
  std::string s;
  int c = 0;
  char buf[1];
  while ((s.empty() || s.back() != '\n') && (c = read(buf, 1)) > 0) {
    s.push_back(buf[0]);
  }
  if (c <= 0 && eof)
    *eof = true;
  return s;
}

}  // namespace shcore

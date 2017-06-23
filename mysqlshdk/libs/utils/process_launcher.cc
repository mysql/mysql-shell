/* Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include "mysqlshdk/libs/utils/process_launcher.h"

#include <cassert>
#include <string>
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

Process_launcher::Process_launcher(const char * const *argv, bool redirect_stderr)
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
std::string Process_launcher::make_windows_cmdline(const char * const*argv) {
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


#ifdef WIN32

void Process_launcher::start() {
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

  BOOL bSuccess = FALSE;

  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  if (redirect_stderr)
    si.hStdError = child_out_wr;
  si.hStdOutput = child_out_wr;
  si.hStdInput = child_in_rd;
  si.dwFlags |= STARTF_USESTDHANDLES;

  bSuccess = CreateProcess(
    NULL,          // lpApplicationName
    lp_cmd,        // lpCommandLine
    NULL,          // lpProcessAttributes
    NULL,          // lpThreadAttributes
    TRUE,          // bInheritHandles
    0,             // dwCreationFlags
    NULL,          // lpEnvironment
    NULL,          // lpCurrentDirectory
    &si,           // lpStartupInfo
    &pi);          // lpProcessInformation

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

HANDLE Process_launcher::get_pid() {
  return pi.hProcess;
}

bool Process_launcher::check() {
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

int Process_launcher::wait() {
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

void Process_launcher::close() {
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
  if (!CloseHandle(child_in_wr))
    report_error(NULL);

  is_alive = false;
}

int Process_launcher::read_one_char() {
  char buf[1];
  BOOL bSuccess = FALSE;
  DWORD dwBytesRead, dwCode;

  while (!(bSuccess = ReadFile(child_out_rd, buf, 1, &dwBytesRead, NULL))) {
    dwCode = GetLastError();
    if (dwCode == ERROR_NO_DATA)
      continue;
    if (dwCode == ERROR_BROKEN_PIPE)
      return EOF;
    else
      report_error(NULL);
  }

  return buf[0];
}

int Process_launcher::read(char *buf, size_t count) {
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

int Process_launcher::write_one_char(int c) {
  CHAR buf[1];
  BOOL bSuccess = FALSE;
  DWORD dwBytesWritten;

  bSuccess = WriteFile(child_in_wr, buf, 1, &dwBytesWritten, NULL);
  if (!bSuccess) {
    if (GetLastError() != ERROR_NO_DATA)  // otherwise child process just died.
      report_error(NULL);
  } else {
    return 1;
  }
  return 0;  // so the compiler does not cry
}

int Process_launcher::write(const char *buf, size_t count) {
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

void Process_launcher::report_error(const char *msg) {
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
    msgerr += "with error code %d.";
    std::string fmt = str_format(msgerr.c_str(), dwCode);
    throw std::system_error(dwCode, std::generic_category(), fmt);
  }
}

HANDLE Process_launcher::get_fd_write() {
  return child_in_wr;
}

HANDLE Process_launcher::get_fd_read() {
  return child_out_rd;
}

#else  // !WIN32

void Process_launcher::start()
{
  assert(!is_alive);

  if (pipe(fd_in) < 0) {
    report_error(NULL);
  }
  if (pipe(fd_out) < 0) {
    ::close(fd_in[0]);
    ::close(fd_in[1]);
    report_error(NULL);
  }
  // Ignore broken pipe signal
  signal(SIGPIPE, SIG_IGN);

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
  }
  else
  {
    ::close(fd_out[1]);
    ::close(fd_in[0]);
    is_alive = true;
    _wait_pending = true;
  }
}

void Process_launcher::close() {
  is_alive = false;
  ::close(fd_out[0]);
  ::close(fd_in[1]);

  if (::kill(childpid, SIGTERM) < 0 && errno != ESRCH)
    report_error(NULL);
  if (errno != ESRCH) {
    sleep(1);
    if (::kill(childpid, SIGKILL) < 0 && errno != ESRCH)
      report_error(NULL);
  }
}

int Process_launcher::read_one_char() {
  int c;
  do {
    if ((c = ::read(fd_out[0], &c, 1)) >= 0)
      return c;
    if (errno == EAGAIN || errno == EINTR)
      continue;
    if (errno == EPIPE)
      return 0;
    break;
  } while (true);
  report_error(NULL);
  return -1;
}

int Process_launcher::read(char *buf, size_t count) {
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

int Process_launcher::write_one_char(int c) {
  int n;
  if ((n = ::write(fd_in[1], &c, 1)) >= 0)
    return n;
  if (errno == EPIPE)
    return 0;
  report_error(NULL);
  return -1;
}

int Process_launcher::write(const char *buf, size_t count) {
  int n;
  if ((n = ::write(fd_in[1], buf, count)) >= 0)
    return n;
  if (errno == EPIPE)
    return 0;
  report_error(NULL);
  return -1;
}

void Process_launcher::report_error(const char *msg) {
  char sys_err[128];
  int errnum = errno;
  if (msg == NULL) {
    strerror_r(errno, sys_err, sizeof(sys_err));
    std::string s = sys_err;
    s += "with errno %d.";
    std::string fmt = str_format(s.c_str(), errnum);
    throw std::system_error(errnum, std::generic_category(), fmt);
  } else {
    throw std::system_error(errnum, std::generic_category(), msg);
  }
}

pid_t Process_launcher::get_pid() {
  return childpid;
}

bool Process_launcher::check() {
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
int Process_launcher::wait() {
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

int Process_launcher::get_fd_write() {
  return fd_in[1];
}

int Process_launcher::get_fd_read() {
  return fd_out[0];
}

#endif

void Process_launcher::kill() {
  close();
}
}  // namespace shcore

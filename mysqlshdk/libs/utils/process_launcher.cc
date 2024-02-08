/*
 * Copyright (c) 2014, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"

#include <algorithm>
#include <cassert>
#include <mutex>
#include <string>
#include <system_error>
#include <utility>

#ifdef _WIN32
#include <Strsafe.h>
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
#elif defined(__sun)
// I_PUSH:
#include <stropts.h>
#elif defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
#include <util.h>
#elif !defined(WIN32)
#include <pty.h>
#endif

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

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

void change_controlling_terminal(int new_ctty, const char *new_ctty_name) {
  // open the current controlling terminal
  int fd = open("/dev/tty", O_RDWR | O_NOCTTY);

  if (fd >= 0) {
    // detach from current controlling terminal
    // this is not required, as setsid() will do this anyway, hence we ignore
    // potential errors
    // it's here as some older platforms may not work properly without this call
    ioctl(fd, TIOCNOTTY, 0);

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
  // this is not required, as opening the terminal will do the same, hence
  // we ignore potential errors
  // it's here as some older platforms may not work properly without this call
  ioctl(new_ctty, TIOCSCTTY, 0);

  // open tty, this will also set the controlling terminal
  fd = open(new_ctty_name, O_RDONLY);
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
    if (errno == EINTR) continue;
    report_error("redirect_fd()");
  }
}

void redirect_input_output(int fd_in[2], int fd_out[2], bool redirect_stderr) {
  // connect pipes to stdin, stdout and stderr
  if (fd_out[0] >= 0) ::close(fd_out[0]);
  if (fd_in[1] >= 0) ::close(fd_in[1]);

  redirect_fd(fd_out[1], STDOUT_FILENO);

  if (redirect_stderr) {
    redirect_fd(fd_out[1], STDERR_FILENO);
  }

  redirect_fd(fd_in[0], STDIN_FILENO);

  fcntl(fd_out[1], F_SETFD, FD_CLOEXEC);
  fcntl(fd_in[0], F_SETFD, FD_CLOEXEC);
}

int open_pseudo_terminal(int *in_master, int *in_slave, char *in_name) {
#ifdef __sun
  int master = posix_openpt(O_RDWR | O_NOCTTY);

  if (master < 0) {
    report_error("posix_openpt()");
    return -1;
  }

  const auto close_master = [master](const char *error) {
    close(master);
    report_error(error);
  };

  if (grantpt(master)) {
    close_master("grantpt()");
    return -1;
  }

  if (unlockpt(master)) {
    close_master("unlockpt()");
    return -1;
  }

  char *slave_name = ptsname(master);

  if (nullptr == slave_name) {
    close_master("ptsname()");
    return -1;
  }

  int slave = open(slave_name, O_RDWR | O_NOCTTY);

  if (slave < 0) {
    close_master("open(slave_name)");
    return -1;
  }

  if (ioctl(slave, I_PUSH, "ptem") < 0 || ioctl(slave, I_PUSH, "ldterm") < 0 ||
      ioctl(slave, I_PUSH, "ttcompat") < 0) {
    close(slave);
    close_master("ioctl(I_PUSH)");
    return -1;
  }

  *in_master = master;
  *in_slave = slave;

  if (in_name != nullptr) {
    strcpy(in_name, slave_name);
  }

  return 0;
#else
  return openpty(in_master, in_slave, in_name, nullptr, nullptr);
#endif
}

#endif

}  // namespace

namespace shcore {

Process::Process(const char *const *argv_, bool redirect_stderr_)
    : argv(argv_), is_alive(false), redirect_stderr(redirect_stderr_) {
#ifdef WIN32
  if (strstr(argv[0], "cmd.exe"))
    throw std::logic_error("launching cmd.exe currently not supported");
// To suport cmd.exe, we need to handle special quoting rules
// required by it
#endif
}

/** Joins list of strings into a command line that is suitable for Windows

  https://msdn.microsoft.com/en-us/library/17w5ykft(v=vs.85).aspx
 */
std::string Process::make_windows_cmdline(const char *const *argv) {
  assert(argv[0]);
  std::string cmd;

  for (int i = 0; argv[i]; i++) {
    if (!cmd.empty()) cmd.push_back(' ');
    // if there are any whitespaces or quotes in the arg, we need to quote
    if (strcspn(argv[i], "\" \t\n\r") < strlen(argv[i])) {
      cmd.push_back('"');
      for (const char *s = argv[i]; *s; ++s) {
        // The trick is that backslashes behave different when they have a "
        // after them from when they don't
        int nbackslashes = 0;
        while (*s == '\\') {
          ++nbackslashes;
          ++s;
        }
        if (nbackslashes > 0) {
          // we've moved past backslashes and stopped at first non-backslash
          if (*s == '"' || *s == '\0') {
            // if backslashes appear before a ", they need to be escaped
            //
            // if backslashes appear before the end of the string, the next
            // char will be the final ", so we also need to escape them
            cmd.append(nbackslashes * 2, '\\');
          } else {
            // otherwise, just add them as is
            cmd.append(nbackslashes, '\\');
          }
        }
        // if a quote appears, we have to escape it
        if (*s == '"') {
          cmd.append("\\\"");
        } else if (*s) {
          cmd.push_back(*s);
        } else if (nbackslashes > 0) {
          // there were backslashes at the end of string
          // we're currently at NULL character, need to step back
          --s;
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

  if (!shcore::is_file(input_file)) {
    throw std::runtime_error(input_file + ": Input file does not exist");
  }

  m_input_file = input_file;
}

#ifdef WIN32

void Process::do_start() {
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
  auto cmd = utf8_to_wide(make_windows_cmdline(argv));
  // lpEnvironment uses Unicode characters
  DWORD creation_flags = CREATE_UNICODE_ENVIRONMENT;
  BOOL bSuccess = FALSE;

  if (create_process_group) creation_flags |= CREATE_NEW_PROCESS_GROUP;

  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  ZeroMemory(&si, sizeof(STARTUPINFOW));
  si.cb = sizeof(STARTUPINFOW);
  if (redirect_stderr) si.hStdError = child_out_wr;
  si.hStdOutput = child_out_wr;
  si.hStdInput = child_in_rd;
  si.dwFlags |= STARTF_USESTDHANDLES;

  bSuccess = CreateProcessW(NULL,            // lpApplicationName
                            &cmd[0],         // lpCommandLine
                            NULL,            // lpProcessAttributes
                            NULL,            // lpThreadAttributes
                            TRUE,            // bInheritHandles
                            creation_flags,  // dwCreationFlags
                            (LPVOID)new_environment.get(),  // lpEnvironment
                            NULL,  // lpCurrentDirectory
                            &si,   // lpStartupInfo
                            &pi);  // lpProcessInformation
  if (!bSuccess) {
    report_error(NULL);
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
      if (!TerminateProcess(pi.hProcess, 129)) report_error(NULL);
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

void Process::set_environment(const std::vector<std::string> &env) {
  if (env.empty()) return;

  std::vector<std::wstring> wenv;
  std::vector<std::wstring> vars_to_set;
  std::size_t nsize = 0;

  for (const auto &var : env) {
    auto wvar = utf8_to_wide(var);
    const auto eq = wvar.find(L'=');

    assert(eq != std::string::npos);

    // include equals sign to ensure that exact match is found
    vars_to_set.emplace_back(wvar.substr(0, eq + 1));
    nsize += wvar.length() + 1;
    wenv.emplace_back(std::move(wvar));
  }

  const auto eblock = GetEnvironmentStringsW();
  std::size_t env_size = 0;
  std::vector<std::pair<std::size_t, std::size_t>> offsets;
  std::size_t offset = 0;

  if (eblock != nullptr) {
    auto bIt = eblock;

    while (*bIt != 0) {
      bool found = false;
      std::size_t len = wcslen(bIt);

      for (const auto &var : vars_to_set) {
        // names of environment variables are case insensitive on Windows
        if (_wcsnicmp(var.c_str(), bIt, var.length()) == 0) {
          found = true;
          offsets.emplace_back(offset, bIt - eblock);
          offset = bIt - eblock + len + 1;
          break;
        }
      }

      if (!found) env_size += len + 1;
      bIt += len + 1;
    }

    if (offset != bIt - eblock) offsets.emplace_back(offset, bIt - eblock);
    env_size++;
  }

  new_environment.reset(new wchar_t[env_size + nsize]);

  offset = 0;

  if (eblock != nullptr) {
    for (const auto &op : offsets) {
      wmemcpy(new_environment.get() + offset, eblock + op.first,
              op.second - op.first);
      offset += op.second - op.first;
    }

    FreeEnvironmentStringsW(eblock);
  }

  for (const auto &e : wenv) {
    wmemcpy(new_environment.get() + offset, e.c_str(), e.length());
    new_environment[offset + e.length()] = 0;
    offset += e.length() + 1;
  }
  new_environment[offset] = 0;
}

#else  // !WIN32

void Process::do_start() {
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

  int slave_device = -1;
  char slave_device_name[512];

  if (m_use_pseudo_tty) {
    // create pseudo terminal which is going to be used as controlling terminal
    // of the new process
    if (open_pseudo_terminal(&m_master_device, &slave_device,
                             slave_device_name) < 0) {
      ::close(fd_in[0]);
      ::close(fd_in[1]);
      ::close(fd_out[0]);
      ::close(fd_out[1]);
      report_error("open_pseudo_terminal()");
    }
  }

  childpid = fork();
  if (childpid == -1) {
    ::close(fd_in[0]);
    ::close(fd_in[1]);
    ::close(fd_out[0]);
    ::close(fd_out[1]);
    if (m_use_pseudo_tty) {
      ::close(m_master_device);
      ::close(slave_device);
    }
    report_error("fork()");
  }

  if (childpid == 0) {
#ifdef LINUX
    prctl(PR_SET_PDEATHSIG, SIGHUP);
#endif

    try {
      if (m_use_pseudo_tty) {
        change_controlling_terminal(slave_device, slave_device_name);

        // close the file descriptors, they're not needed anymore
        ::close(m_master_device);
        ::close(slave_device);
      }

      redirect_input_output(fd_in, fd_out, redirect_stderr);
    } catch (const std::exception &e) {
      fprintf(stderr, "Exception while preparing the child process: %s\n",
              e.what());
      fflush(stderr);
      _exit(128);
    }

    for (const auto &str : new_environment) shcore::setenv(str);
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

    _exit(my_errno);
  } else {
    ::close(fd_out[1]);
    ::close(fd_in[0]);

    if (m_use_pseudo_tty) {
      ::close(slave_device);
    }
  }
}

void Process::close() {
  is_alive = false;
  ::close(fd_out[0]);
  if (fd_in[1] >= 0) ::close(fd_in[1]);
  if (m_master_device >= 0) ::close(m_master_device);

  if (::kill(childpid, SIGTERM) < 0 && errno != ESRCH) report_error(NULL);
  if (errno != ESRCH) {
    ::sleep(1);
    if (::kill(childpid, SIGKILL) < 0 && errno != ESRCH) report_error(NULL);
  }
}

int Process::do_read(char *buf, size_t count) {
  do {
    if (int n = ::read(fd_out[0], buf, count); n >= 0) return n;

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
      std::lock_guard<std::mutex> lock{m_terminal_buffer_mutex};
      has_output = !m_terminal_buffer.empty();
    }

    if (!has_output) {
      shcore::sleep_ms(10);
    }
  }

  std::string output;

  {
    std::lock_guard<std::mutex> lock{m_terminal_buffer_mutex};
    output = {m_terminal_buffer.begin(), m_terminal_buffer.end()};
    m_terminal_buffer.clear();
  }

  return output;
#else   // ! __APPLE__
  char buffer[512];
  int64_t retry_count = 0;

  pollfd input;
  input.fd = m_master_device;
  input.events = POLLIN;

  do {
    input.revents = 0;

    int ret = ::poll(&input, 1, 500);  // 500ms

    if (ret == -1) {
      report_error(nullptr);
    } else if (ret == 0) {
      report_error("Timeout on poll()");
    } else if (input.revents & POLLIN) {
      int n = ::read(m_master_device, buffer, sizeof(buffer));

      if (n >= 0) {
        return std::string(buffer, n);
      } else {
        report_error(nullptr);
      }
    } else if (input.revents & POLLHUP) {
      // This event means that the other side of pseudo terminal is closed.
      // Since child does not duplicate it to stdin/stdout/stderr, it will
      // remain closed until the child process opens the controlling terminal
      // (i.e. writes the password prompt and waits for user input).
      if (++retry_count > 1000000) {
        report_error("Controlling terminal seems to be closed");
      } else {
        continue;
      }
    } else {
      const auto msg = "No data to read, poll() returned an error event: " +
                       std::to_string(input.revents);
      report_error(msg.c_str());
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
  if (_wait_pending) {
    int ret;
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
  if (WIFEXITED(_pstatus)) return WEXITSTATUS(_pstatus);
  return WTERMSIG(_pstatus) + 128;
}

void Process::set_environment(const std::vector<std::string> &env) {
  new_environment = env;
}

#endif  // !_WIN32

void Process::kill() { close(); }

int Process::read(char *buf, size_t count) {
  if (!m_reader_thread) return do_read(buf, count);

  std::lock_guard lock(m_read_buffer_mutex);
  count = std::min(m_read_buffer.size(), count);

  if (count > 0) {
    std::copy(m_read_buffer.begin(), m_read_buffer.begin() + count, buf);
    m_read_buffer.erase(m_read_buffer.begin(), m_read_buffer.begin() + count);
  }

  return static_cast<int>(count);
}

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
  constexpr size_t k_buffer_size = 512;
  std::string s;
  char buffer[k_buffer_size];
  int c = 0;

  while ((c = read(buffer, k_buffer_size)) > 0) {
    s.append(buffer, c);
  }

  return s;
}

void Process::enable_reader_thread() {
  if (is_alive) {
    throw std::logic_error(
        "This method needs to be called before starting "
        "the child process.");
  }

  m_use_reader_thread = true;
}

void Process::start_reader_threads() {
  if (m_use_reader_thread && !m_reader_thread) {
    // Read process output in a background thread, this makes all read
    // operations non-blocking. This may be required i.e. on Windows, where we
    // use anonymous pipes to transfer output from child processes. If output is
    // not read, pipe's buffer will become full and child process will hang.
    m_reader_thread =
        std::make_unique<std::thread>(mysqlsh::spawn_scoped_thread([this]() {
          try {
            constexpr size_t k_buffer_size = 512;
            char buffer[k_buffer_size];
            int n = 0;
            while ((n = do_read(buffer, k_buffer_size)) > 0) {
              std::lock_guard<std::mutex> lock(m_read_buffer_mutex);
              m_read_buffer.insert(m_read_buffer.end(), buffer, buffer + n);
            }
          } catch (...) {
          }
        }));
  }

#ifdef __APPLE__
  // need to drain output from controlling terminal, otherwise launched
  // process will hang on exit
  if (m_use_pseudo_tty && !m_terminal_reader_thread) {
    m_terminal_reader_thread =
        std::make_unique<std::thread>(mysqlsh::spawn_scoped_thread([this]() {
          constexpr size_t k_buffer_size = 512;
          char buffer[k_buffer_size];
          int n = 0;
          while ((n = ::read(m_master_device, buffer, k_buffer_size)) > 0) {
            std::lock_guard<std::mutex> lock{m_terminal_buffer_mutex};
            m_terminal_buffer.insert(m_terminal_buffer.end(), buffer,
                                     buffer + n);
          }
        }));
  }
#endif  // __APPLE__
}

void Process::stop_reader_threads() {
  if (m_reader_thread) {
    m_reader_thread->join();
    m_reader_thread.reset(nullptr);
  }

#ifdef __APPLE__
  if (m_terminal_reader_thread) {
    m_terminal_reader_thread->join();
    m_terminal_reader_thread.reset(nullptr);
  }
#endif  // __APPLE__
}

void Process::start() {
  do_start();
  start_reader_threads();

  is_alive = true;
  _wait_pending = true;
}

}  // namespace shcore

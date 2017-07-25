/* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.

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

#include "unittest/test_utils/command_line_test.h"
#ifndef _WIN32
#include <signal.h>
#endif
#include <system_error>
#include "mysqlshdk/libs/utils/process_launcher.h"

namespace tests {

void Command_line_test::SetUp() {
  _mysqlsh_path = get_path_to_mysqlsh();
  _mysqlsh = _mysqlsh_path.c_str();
  _process = nullptr;

  Shell_base_test::SetUp();
}

std::string Command_line_test::get_path_to_mysqlsh() {
  std::string command;

#ifdef _WIN32
  // For now, on windows the executable is expected to be on the same path as
  // the unit tests
  command = "mysqlsh.exe";
#else
  std::string prefix = g_argv0;
  // strip unittest/run_unit_tests
  size_t pos = prefix.rfind('/');
  prefix = prefix.substr(0, pos);
  pos = prefix.rfind('/');
  if (pos == std::string::npos)
    prefix = ".";
  else
    prefix = prefix.substr(0, pos);

  command = prefix + "/mysqlsh";
#endif

  return command;
}

int Command_line_test::execute(const std::vector<const char *> &args) {
  // There MUST be arguments (at least _mysqlsh, and the last must be NULL
  assert(args.size() > 0);
  assert(args[args.size() - 1] == NULL);

  char c;
  int exit_code = 1;
  _output.clear();

  bool debug = getenv("TEST_DEBUG") != nullptr;

  {
    std::lock_guard<std::mutex> lock(_process_mutex);
    _process = new shcore::Process_launcher(&args[0]);
  }
#ifdef _WIN32
  _process->set_create_process_group();
#endif
  try {
    // Starts the process
    _process->start();

    // Reads all produced output, until stdout is closed
    while (_process->read(&c, 1) > 0) {
      if (debug)
        std::cout << c << std::flush;
      _output_mutex.lock();
      _output += c;
      _output_mutex.unlock();
    }

    // Wait until it finishes
    exit_code = _process->wait();

  } catch (const std::system_error &e) {
    _output = e.what();
    exit_code = 256;  // This error code will indicate an error happened
                      // launching the process
  }
  {
    std::lock_guard<std::mutex> lock(_process_mutex);
    delete _process;
    _process = nullptr;
  }
  return exit_code;
}

bool Command_line_test::grep_stdout(const std::string &s) {
  std::lock_guard<std::mutex> lock(_output_mutex);
  return _output.find(s) != std::string::npos;
}

void Command_line_test::send_ctrlc() {
#ifdef _WIN32
  std::lock_guard<std::mutex> lock(_process_mutex);
  if (_process) {
    GenerateConsoleCtrlEvent(CTRL_C_EVENT, _process->get_process_id());
  }
#else
  std::lock_guard<std::mutex> lock(_process_mutex);
  if (_process) {
    ::kill(_process->get_pid(), SIGINT);
  }
#endif
}

}  // namespace tests

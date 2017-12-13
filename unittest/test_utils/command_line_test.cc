/* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.

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

#include "unittest/test_utils/command_line_test.h"
#ifndef _WIN32
#include <signal.h>
#endif
#include <system_error>
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace tests {

void Command_line_test::SetUp() {
  _mysqlsh_path = get_path_to_mysqlsh();
  _mysqlsh = _mysqlsh_path.c_str();
  _process = nullptr;

  Shell_base_test::SetUp();
}

/**
 * Execute the mysqlsh binary passing the provided command line arguments.
 * @param args array of command line arguments to be passed on the call to the
 * mysqlsh.
 * @param password password to be given to the mysqlsh when prompted.
 * @returns the return code from the mysqlsh call.
 * This function calls the mysqlsh binary with the provided parameters and saves
 * the output coming from the mysqlsh.
 *
 */
int Command_line_test::execute(const std::vector<const char *> &args,
              const char *password) {
  // There MUST be arguments (at least _mysqlsh, and the last must be NULL
  assert(args.size() > 0);
  assert(args[args.size() - 1] == NULL);

  char c;
  int exit_code = 1;
  _output.clear();

  bool debug = getenv("TEST_DEBUG") != nullptr;
  if (debug) {
    std::cerr << shcore::str_join(&args[0], &args[args.size()-1], " ") << "\n";
  }

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

    // The password should be provided when it is expected that the Shell
    // will prompt for it, in such case, we give it on the stdin
    if (password) {
      std::string pwd(password);
      pwd.append("\n");
      _process->write(pwd.c_str(), pwd.size());
    }

    // Reads all produced output, until stdout is closed
    while (_process->read(&c, 1) > 0) {
      if (debug)
        std::cerr << c << std::flush;
      if (c == '\r' && _strip_carriage_returns)
        continue;
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

/**
 * Verifies if the received string is present on the grabbed output.
 */
bool Command_line_test::grep_stdout(const std::string &s) {
  std::lock_guard<std::mutex> lock(_output_mutex);
  return _output.find(s) != std::string::npos;
}

/**
 * Sends the interruption signal to the mysqlsh process.
 */
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

/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#include "unittest/test_utils/shell_test_wrapper.h"
#include <memory>
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_time.h"
#include "shellcore/interrupt_handler.h"

extern char* g_mppath;

namespace tests {
Shell_test_wrapper::Shell_test_wrapper() {
  reset();
}

/**
 * Causes the enclosed instance of Mysql_shell to be re-created with using the
 * options defined at _opts.
 */
void Shell_test_wrapper::reset() {
  _opts = std::make_shared<mysqlsh::Shell_options>();
  const_cast<mysqlsh::Shell_options::Storage&>(_opts->get()).gadgets_path =
      g_mppath;
  _interactive_shell.reset(
      new mysqlsh::Mysql_shell(_opts, &output_handler.deleg));

  _interactive_shell->finish_init();
}

/**
 * Turns on debugging on the inner Shell_test_output_handler
 */
void Shell_test_wrapper::enable_debug() {
  output_handler.debug = true;
}

/**
 * Returns the inner instance of the Shell_test_output_handler
 */
Shell_test_output_handler& Shell_test_wrapper::get_output_handler() {
  return output_handler;
}

/**
 * Executes the given instruction.
 * @param line the instruction to be executed as would be given i.e. using
 * the mysqlsh application.
 */
void Shell_test_wrapper::execute(const std::string& line) {
  if (output_handler.debug)
    std::cout << line << std::endl;

  _interactive_shell->process_line(line);
}

/**
 * Returns a reference to the options that configure the inner Mysql_shell
 */
mysqlsh::Shell_options::Storage& Shell_test_wrapper::get_options() {
  return const_cast<mysqlsh::Shell_options::Storage&>(_opts->get());
}

/**
 * Enables X protocol tracing.
 */
void Shell_test_wrapper::trace_protocol() {
  _interactive_shell->shell_context()->get_dev_session()->set_option(
      "trace_protocol", 1);
}

#if 0
/*Asynch_shell_test_wrapper::Asynch_shell_test_wrapper(bool trace)
    : _trace(trace), _state("IDLE"), _started(false) {
}

Asynch_shell_test_wrapper::~Asynch_shell_test_wrapper() {
  if (_started)
    shutdown();
}

mysqlsh::Shell_options& Asynch_shell_test_wrapper::get_options() {
  return _shell.get_options();
}

void Asynch_shell_test_wrapper::execute(const std::string& line) {
  _input_lines.push(line);
}

bool Asynch_shell_test_wrapper::execute_sync(const std::string& line,
                                             float timeup) {
  _input_lines.push(line);

  return wait_for_state("IDLE", timeup);
}

void Asynch_shell_test_wrapper::dump_out() {
  std::cout << _shell.get_output_handler().std_out << std::endl;
  _shell.get_output_handler().wipe_out();
}

void Asynch_shell_test_wrapper::dump_err() {
  std::cerr << _shell.get_output_handler().std_err << std::endl;
  _shell.get_output_handler().wipe_err();
}
*/
/**
 * This causes the call to wait until the thread state is as specified
 * @param state The state that must be reached to exit the wait loop
 * @param timeup The max number of seconds to wait for the state to be reached
 * @return true if the state is reached, false if the state is not reached
 * within the specified time.
 *
 * If no timeup is specified, the loop will continue until the state is reached
 */
/*
bool Asynch_shell_test_wrapper::wait_for_state(const std::string& state,
                                               float timeup) {
  bool ret_val = false;
  MySQL_timer timer;
  auto limit = timer.seconds_to_duration(timeup);
  timer.start();

  bool exit = false;
  while (!exit) {
    timer.end();

    auto current_duration = timer.raw_duration();
    if (limit > 0 && limit <= current_duration) {
      // Exits if timeup is set and was reached
      exit = true;
    } else if (!_input_lines.empty() || _state != state) {
      // Continues waiting for the desired state
      shcore::sleep_ms(50);
    } else {
      ret_val = true;
      // Exits because the state was reached as expected
      exit = true;
    }
  }

  return ret_val;
}

void Asynch_shell_test_wrapper::trace_protocol() {
  _shell.trace_protocol();
}

std::string Asynch_shell_test_wrapper::get_state() {
  while (!_input_lines.empty())
    shcore::sleep_ms(500);

  return _state;
}

Shell_test_output_handler& Asynch_shell_test_wrapper::get_output_handler() {
  return _shell.get_output_handler();
}

void Asynch_shell_test_wrapper::start() {
  _thread = std::shared_ptr<std::thread>(new std::thread([this]() {
    shcore::Interrupts::ignore_thread();
    _shell.reset();

    while (true) {
      std::string _next_line = _input_lines.pop();
      if (_next_line == "\\q") {
        break;
      } else {
        _state = "PROCESSING";

        if (_trace)
          std::cout << _state << ": " << _next_line << std::endl;

        _shell.execute(_next_line);
        _state = "IDLE";

        if (_trace)
          std::cout << _state << std::endl;
      }
    }
  }));

  _started = true;
}

void Asynch_shell_test_wrapper::shutdown() {
  _thread->join();
}*/
#endif
}  // namespace tests

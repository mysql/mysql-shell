/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include "unittest/test_utils/shell_test_wrapper.h"

#include <map>
#include <memory>
#include <vector>

#include "mysqlshdk/libs/utils/trandom.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "shellcore/interrupt_handler.h"
#include "unittest/test_utils/shell_test_env.h"
#include "unittest/test_utils/test_net_utilities.h"

extern char *g_mppath;
extern mysqlshdk::db::replay::Mode g_test_recording_mode;
extern mysqlshdk::utils::Version g_target_server_version;

using TestingMode = mysqlshdk::db::replay::Mode;

namespace tests {

namespace {

class My_random : public mysqlshdk::utils::Random {
 public:
  virtual std::string get_time_string() {
    return "000000000" + std::to_string(++ts);
  }

 private:
  int ts = 0;
};

Test_net_utilities test_net_utilities;

}  // namespace

Shell_test_wrapper::Shell_test_wrapper(bool disable_dummy_sandboxes) {
  const char *tmpdir = getenv("TMPDIR");
  if (tmpdir) {
    _sandbox_dir.assign(tmpdir);
  } else {
    // If not specified, the tests will create the sandboxes on the
    // binary folder
    _sandbox_dir = shcore::get_binary_folder();
  }

  _dummy_sandboxes = disable_dummy_sandboxes
                         ? false
                         : g_test_recording_mode == TestingMode::Replay;

  _recording_enabled = g_test_recording_mode == TestingMode::Record;

  const char *port = getenv("MYSQL_PORT");
  if (port) {
    _mysql_port_number = atoi(port);
    _mysql_port.assign(port);
  }

  for (int i = 0; i < sandbox::k_num_ports; ++i) {
    m_sandbox_ports[i] = sandbox::k_ports[i];
  }

  _hostname = getenv("MYSQL_HOSTNAME");
  _hostname_ip = mysqlshdk::utils::Net::resolve_hostname_ipv4(_hostname);

  reset();
}

// This can be use to reinitialize the interactive shell with different
// options First set the options on _options
void Shell_test_wrapper::reset_options() {
  _opts.reset(new mysqlsh::Shell_options());
  _options = const_cast<mysqlsh::Shell_options::Storage *>(&_opts->get());
  _options->gadgets_path = g_mppath;
  _options->db_name_cache = false;

  // Allows derived classes configuring specific options
  // set_options();
}

/**
 * Causes the enclosed instance of Command_line_shell to be re-created with
 * using the options defined at _opts.
 */
void Shell_test_wrapper::reset() {
  // previous shell needs to be destroyed before a new one can be created
  _interactive_shell.reset();
  _interactive_shell.reset(new mysqlsh::Command_line_shell(
      get_options(),
      std::unique_ptr<shcore::Interpreter_delegate>{
          new shcore::Interpreter_delegate(output_handler.deleg)}));

  _interactive_shell->finish_init();

  enable_testutil();
}

/**
 * Turns on debugging on the inner Shell_test_output_handler
 */
void Shell_test_wrapper::enable_debug() { output_handler.debug = true; }

/**
 * Returns the inner instance of the Shell_test_output_handler
 */
Shell_test_output_handler &Shell_test_wrapper::get_output_handler() {
  return output_handler;
}

/**
 * Executes the given instruction.
 * @param line the instruction to be executed as would be given i.e. using
 * the mysqlsh application.
 */
void Shell_test_wrapper::execute(const std::string &line) {
  if (output_handler.debug) std::cout << line << std::endl;

  _interactive_shell->process_line(line);
}

/**
 * Returns a reference to the options that configure the inner
 * Command_line_shell
 */
std::shared_ptr<mysqlsh::Shell_options> Shell_test_wrapper::get_options() {
  if (!_opts) reset_options();

  return _opts;
}

/**
 * Enables X protocol tracing.
 */
void Shell_test_wrapper::trace_protocol() {
  _interactive_shell->shell_context()->get_dev_session()->set_option(
      "trace_protocol", 1);
}

void Shell_test_wrapper::enable_testutil() {
  // Dummy sandboxes may be used i.e. while replying tests, unless it is
  // specified that they should never be used
  _testutil.reset(new tests::Testutils(_sandbox_dir, _dummy_sandboxes,
                                       _interactive_shell,
                                       Shell_test_env::get_path_to_mysqlsh()));
  _testutil->set_test_callbacks(
      [this](const std::string &prompt, const std::string &text) {
        output_handler.prompts.push_back({prompt, text});
      },
      [this](const std::string &prompt, const std::string &pass) {
        output_handler.passwords.push_back({prompt, pass});
      },
      [this](bool eat_one) -> std::string {
        if (eat_one) {
          return shcore::str_partition_after_inpl(&output_handler.std_out,
                                                  "\n");
        } else {
          return output_handler.std_out;
        }
      },
      [this](bool eat_one) -> std::string {
        if (eat_one) {
          return shcore::str_partition_after_inpl(&output_handler.std_err,
                                                  "\n");
        } else {
          return output_handler.std_err;
        }
      },
      [this]() -> void {
        EXPECT_EQ(0, output_handler.prompts.size());
        EXPECT_EQ(0, output_handler.passwords.size());
      },
      [this]() -> void { output_handler.wipe_all(); });

  if (g_test_recording_mode != mysqlshdk::db::replay::Mode::Direct)
    _testutil->set_sandbox_snapshot_dir(
        mysqlshdk::db::replay::current_recording_dir());

  _interactive_shell->set_global_object("testutil", _testutil);
}

std::string Shell_test_wrapper::setup_recorder(const char *sub_test_name) {
  mysqlshdk::db::replay::set_mode(g_test_recording_mode);
  _recording_enabled = true;

  bool is_recording =
      (g_test_recording_mode == mysqlshdk::db::replay::Mode::Record);

  std::string tracedir;

  std::string context;
  const ::testing::TestInfo *const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();

  if (test_info) {
    context.append(test_info->test_case_name());
    context.append("/");
    if (sub_test_name) {
      // test_info->name() can be in the form test/<index> if this is a
      // parameterized test. For such cases, we replace the index with the
      // given sub_test_name, so that we can have a stable test name that
      // doesn't change if the execution order changes
      context.append(shcore::path::dirname(test_info->name()) + "/" +
                     sub_test_name);
    } else {
      context.append(test_info->name());
    }
  } else {
    if (sub_test_name) context.append(sub_test_name);
  }

  tracedir = shcore::path::join_path(
      {mysqlshdk::db::replay::g_recording_path_prefix, context});
  context.append("/");

  if (g_test_recording_mode == mysqlshdk::db::replay::Mode::Replay &&
      !shcore::is_folder(tracedir)) {
    ADD_FAILURE()
        << "Test running in replay mode but trace directory is missing: "
        << tracedir << "\n";
    throw std::logic_error("Missing tracedir");
  }

  mysqlshdk::db::replay::begin_recording_context(context);

  if (is_recording) {
    if (shcore::is_folder(mysqlshdk::db::replay::current_recording_dir()) &&
        !shcore::is_file(mysqlshdk::db::replay::current_recording_dir() +
                         "/FAILED")) {
      ADD_FAILURE() << "Test running in record mode, but trace directory "
                       "already exists (and is not FAILED). Delete directory "
                       "to re-record it: "
                    << mysqlshdk::db::replay::current_recording_dir() << "\n";
      throw std::logic_error("Tracedir already exists");
    }

    try {
      // Delete old data
      shcore::remove_directory(tracedir, true);
    } catch (...) {
    }
    shcore::create_directory(tracedir);
    shcore::create_file(
        mysqlshdk::db::replay::current_recording_dir() + "/FAILED", "");

    // TODO(rennox): This session monitoring is not available here
    // Set up hooks for keeping track of opened sessions
    /*mysqlshdk::db::replay::on_recorder_connect_hook = std::bind(
        &Shell_test_env::on_session_connect, this, std::placeholders::_1);
    mysqlshdk::db::replay::on_recorder_close_hook = std::bind(
        &Shell_test_env::on_session_close, this, std::placeholders::_1);*/
  }

  if (g_test_recording_mode == mysqlshdk::db::replay::Mode::Replay) {
    // Some environmental or random data can change between recording and
    // replay time. Such data must be ensured to match between both.
    auto info = mysqlshdk::db::replay::load_test_case_info();

    // Override environment dependent data with the same values that were
    // used and saved during recording
    for (int i = 0; i < sandbox::k_num_ports; ++i) {
      const auto port = info.find("sandbox_port" + std::to_string(i));

      if (info.end() != port) {
        m_sandbox_ports[i] = std::stoi(port->second);
      } else {
        m_sandbox_ports[i] = sandbox::k_ports[i];
      }
    }

    _hostname = info["hostname"];
    _hostname_ip = info["hostname_ip"];

    // Inject the recorded environment, so that tests that call things like
    // Net::is_loopback() will replay with the same environment as where
    // traces were recorded
    test_net_utilities.inject(_hostname, shcore::Value::parse(info["net_data"]),
                              mysqlshdk::db::replay::Mode::Replay);
  } else if (g_test_recording_mode == mysqlshdk::db::replay::Mode::Record) {
    std::map<std::string, std::string> info;

    for (int i = 0; i < sandbox::k_num_ports; ++i) {
      info["sandbox_port" + std::to_string(i)] =
          std::to_string(m_sandbox_ports[i]);
    }

    info["hostname"] = _hostname;
    info["hostname_ip"] = _hostname_ip;

    test_net_utilities.inject(_hostname, {},
                              mysqlshdk::db::replay::Mode::Record);

    mysqlshdk::db::replay::save_test_case_info(info);
  }

  if (g_test_recording_mode != mysqlshdk::db::replay::Mode::Direct) {
    // Ensure randomly generated strings are not so random when
    // recording/replaying
    mysqlshdk::utils::Random::set(new My_random());
  }

  return tracedir;
}

void Shell_test_wrapper::teardown_recorder() {
  mysqlshdk::db::replay::on_recorder_connect_hook = {};
  mysqlshdk::db::replay::on_recorder_close_hook = {};

  if (g_test_recording_mode == mysqlshdk::db::replay::Mode::Record) {
    // If the test failed during recording mode, put a fail marker in the
    // trace dir, no conditions set right now
    shcore::delete_file(mysqlshdk::db::replay::current_recording_dir() +
                        "/FAILED");

    auto info = mysqlshdk::db::replay::load_test_case_info();
    info["net_data"] = test_net_utilities.get_recorded().repr();
    mysqlshdk::db::replay::save_test_case_info(info);
  }

  test_net_utilities.remove();

  mysqlshdk::db::replay::end_recording_context();
}

static int find_column_in_select_stmt(const std::string &sql,
                                      const std::string &column) {
  std::string s = shcore::str_rstrip(shcore::str_lower(sql), ";");
  // sanity checks for things we don't support
  assert(s.find(" from ") == std::string::npos);
  assert(s.find(" where ") == std::string::npos);
  assert(s.find("select ") == 0);
  assert(column.find(" ") == std::string::npos);
  // other not supported things...: ``, column names with special chars,
  // aliases, stuff in comments etc
  s = s.substr(strlen("select "));

  // trim out stuff inside parenthesis which can confuse the , splitter and
  // are not supported anyway, like:
  // select (select a, b from something), c
  // select concat(a, b), c
  std::string::size_type p = s.find("(");
  while (p != std::string::npos) {
    std::string::size_type pp = s.find(")", p);
    if (pp != std::string::npos) {
      s = s.substr(0, p + 1) + s.substr(pp);
    } else {
      break;
    }
    p = s.find("(", pp);
  }

  std::vector<std::string> columns(shcore::str_split(s, ","));
  // the last column name can contain other stuff
  columns[columns.size() - 1] =
      shcore::str_split(shcore::str_strip(columns.back()), " \t\n\r").front();

  int i = 0;
  for (const auto &c : columns) {
    if (shcore::str_strip(c) == column) return i;
    ++i;
  }
  return -1;
}

void Shell_test_wrapper::reset_replayable_shell(const char *sub_test_name) {
  setup_recorder(sub_test_name);  // must be called before set_defaults()
  reset();

#ifdef _WIN32
  mysqlshdk::db::replay::set_replay_query_hook([](const std::string &sql) {
    return shcore::str_replace(sql, ".dll", ".so");
  });
#endif

  // Intercept queries and hack their results so that we can have
  // recorded local sessions that match the actual local environment
  mysqlshdk::db::replay::set_replay_row_hook(
      [this](const mysqlshdk::db::Connection_options &, const std::string &sql,
             std::unique_ptr<mysqlshdk::db::IRow> source)
          -> std::unique_ptr<mysqlshdk::db::IRow> {
        int datadir_column = -1;

        if (sql.find("@@datadir") != std::string::npos &&
            shcore::str_ibeginswith(sql, "select ")) {
          // find the index for @@datadir in the query
          datadir_column = find_column_in_select_stmt(sql, "@@datadir");
          assert(datadir_column >= 0);
        } else {
          assert(sql.find("@@datadir") == std::string::npos);
        }

        // replace sandbox @@datadir from results with actual datadir
        if (datadir_column >= 0) {
          std::string prefix = shcore::path::dirname(
              shcore::path::dirname(source->get_string(datadir_column)));
          std::string suffix =
              source->get_string(datadir_column).substr(prefix.length() + 1);
          std::string datadir = shcore::path::join_path(_sandbox_dir, suffix);
#ifdef _WIN32
          datadir = shcore::str_replace(datadir, "/", "\\");
#endif
          return std::unique_ptr<mysqlshdk::db::IRow>{
              new tests::Override_row_string(
                  std::move(source),
                  std::vector<uint32_t>{(uint32_t)datadir_column},
                  std::vector<std::string>{datadir})};
        }

#ifdef __sun
        return std::move(source);
#else
        return source;
#endif
      });
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

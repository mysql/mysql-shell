/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
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

#include "modules/util/copy/copy_operation.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <string_view>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/include/shellcore/shell_init.h"
#include "mysqlshdk/libs/storage/backend/in_memory/virtual_config.h"
#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace copy {

namespace {

std::unique_ptr<mysqlshdk::storage::IDirectory> directory(
    const std::shared_ptr<mysqlshdk::storage::Config> &config) {
  constexpr auto k_directory_name = "memory";
  return mysqlshdk::storage::make_directory(k_directory_name, config);
}

class Console_with_prefix final : public IConsole {
 public:
  Console_with_prefix() = delete;

  explicit Console_with_prefix(const std::string &prefix)
      : m_prefix(prefix), m_console(current_console()) {}

  Console_with_prefix(const Console_with_prefix &) = delete;
  Console_with_prefix(Console_with_prefix &&) = delete;

  Console_with_prefix &operator=(const Console_with_prefix &) = delete;
  Console_with_prefix &operator=(Console_with_prefix &&) = delete;

  ~Console_with_prefix() override = default;

  void raw_print(const std::string &text, Output_stream stream,
                 bool format_json = true) const override {
    m_console->raw_print(get_text(text), stream, format_json);
  }

  void print(const std::string &text) const override {
    m_console->print(get_text(text));
  }

  void println(const std::string &text = "") const override {
    m_console->println(get_text(text));
  }

  void print_error(const std::string &text) const override {
    m_console->print_error(get_text(text));
  }

  void print_warning(const std::string &text) const override {
    m_console->print_warning(get_text(text));
  }

  void print_note(const std::string &text) const override {
    m_console->print_note(get_text(text));
  }

  void print_status(const std::string &text) const override {
    m_console->print_status(get_text(text));
  }

  void print_info(const std::string &text = "") const override {
    m_console->print_info(get_text(text));
  }

  void print_para(const std::string &text) const override {
    m_console->print_para(get_text(text));
  }

  void print_value(const shcore::Value &, const std::string &) const override {
    throw std::logic_error("Console_with_prefix::print_value() not supported");
  }

  void print_diag(const std::string &text) const override {
    m_console->print_diag(get_text(text));
  }

  shcore::Prompt_result prompt(const std::string &prompt,
                               const shcore::prompt::Prompt_options &options,
                               std::string *out_val) const override {
    return m_console->prompt(get_text(prompt), options, out_val);
  }

  shcore::Prompt_result prompt(
      const std::string &prompt, std::string *out_val,
      Validator validator = nullptr,
      shcore::prompt::Prompt_type type = shcore::prompt::Prompt_type::TEXT,
      const std::string &title = "",
      const std::vector<std::string> &description = {},
      const std::string &default_value = "") const override {
    return m_console->prompt(get_text(prompt), out_val, validator, type, title,
                             description, default_value);
  }

  Prompt_answer confirm(
      const std::string &prompt, Prompt_answer def = Prompt_answer::NO,
      const std::string &yes_label = "&Yes",
      const std::string &no_label = "&No", const std::string &alt_label = "",
      const std::string &title = "",
      const std::vector<std::string> &description = {}) const override {
    return m_console->confirm(get_text(prompt), def, yes_label, no_label,
                              alt_label, title, description);
  }

  shcore::Prompt_result prompt_password(
      const std::string &prompt, std::string *out_val,
      Validator validator = nullptr, const std::string &title = "",
      const std::vector<std::string> &description = {}) const override {
    return m_console->prompt_password(get_text(prompt), out_val, validator,
                                      title, description);
  }

  bool select(const std::string &prompt_text, std::string *result,
              const std::vector<std::string> &items, size_t default_option = 0,
              bool allow_custom = false, Validator validator = nullptr,
              const std::string &title = "",
              const std::vector<std::string> &description = {}) const override {
    return m_console->select(get_text(prompt_text), result, items,
                             default_option, allow_custom, validator, title,
                             description);
  }

  std::shared_ptr<IPager> enable_pager() override {
    return m_console->enable_pager();
  }

  void enable_global_pager() override { m_console->enable_global_pager(); }

  void disable_global_pager() override { m_console->disable_global_pager(); }

  bool is_global_pager_enabled() const override {
    return m_console->is_global_pager_enabled();
  }

  void add_print_handler(shcore::Interpreter_print_handler *handler) override {
    m_console->add_print_handler(handler);
  }

  void remove_print_handler(
      shcore::Interpreter_print_handler *handler) override {
    m_console->remove_print_handler(handler);
  }

 private:
  std::string get_text(const std::string &text) const {
    return m_prefix + ' ' + text;
  }

  std::string m_prefix;
  std::shared_ptr<IConsole> m_console;
};

}  // namespace

std::pair<std::shared_ptr<mysqlshdk::storage::in_memory::Virtual_config>,
          std::unique_ptr<mysqlshdk::storage::IDirectory>>
setup_virtual_storage() {
  auto config = std::make_shared<mysqlshdk::storage::in_memory::Virtual_config>(
      32 * 1024 * 1024);  // 32MB
  config->fs()->set_uses_synchronized_io([](std::string_view name) {
    // this is intended to be used by the copy*() utilities, data files are not
    // compressed and use the .tsv extension
    return shcore::str_iendswith(name, ".tsv");
  });

  auto dir = directory(config);
  dir->create();

  return std::make_pair(std::move(config), std::move(dir));
}

void copy(dump::Ddl_dumper *dumper, Dump_loader *loader,
          const std::shared_ptr<mysqlshdk::storage::in_memory::Virtual_config>
              &storage) {
  shcore::Interrupt_handler intr_handler([dumper, loader, &storage]() -> bool {
    storage->fs()->interrupt();
    dumper->interrupt();
    loader->hard_interrupt();
    return false;
  });

  enum class Status {
    DUMPER_DONE,
    DUMPER_EXCEPTION,
    LOADER_DONE,
    LOADER_EXCEPTION,
  };

  shcore::Synchronized_queue<Status> status;

  std::exception_ptr dumper_exception;
  auto dumper_thread = spawn_scoped_thread([&]() {
    mysqlsh::Mysql_thread mysql_thread;
    mysqlsh::Scoped_console console{std::make_shared<Console_with_prefix>(
        mysqlshdk::textui::green("SRC:"))};

    try {
      dumper->run();
      status.push(Status::DUMPER_DONE);
    } catch (...) {
      dumper_exception = std::current_exception();
      status.push(Status::DUMPER_EXCEPTION);
    }
  });

  // synchronization with the loader thread, we need to detect if an exception
  // occurred in the dumper thread while loader is waiting for the dump to start
  std::condition_variable cv;
  std::mutex mutex;
  std::exception_ptr current_exception;
  std::atomic<int> finished = 0;

  std::exception_ptr loader_exception;
  auto loader_thread = spawn_scoped_thread([&]() {
    mysqlsh::Mysql_thread mysql_thread;
    mysqlsh::Scoped_console console{
        std::make_shared<Console_with_prefix>(mysqlshdk::textui::blue("TGT:"))};

    try {
      // need to wait for dump to start
      const auto file = directory(storage)->file("@.json");

      while (!file->exists()) {
        using namespace std::chrono_literals;
        std::unique_lock lock(mutex);

        cv.wait_for(lock, 100ms, [&] { return !!current_exception; });

        if (current_exception) {
          // exception in the dumper, abort
          return;
        }

        if (finished > 0 && !file->exists()) {
          throw std::runtime_error(
              "Dumper finished, but '@.json' file was not found");
        }
      }

      loader->run();
      status.push(Status::LOADER_DONE);
    } catch (...) {
      loader_exception = std::current_exception();
      status.push(Status::LOADER_EXCEPTION);
    }
  });

  const auto set_current_exception = [&](std::exception_ptr e) {
    {
      std::lock_guard lock(mutex);
      current_exception = e;
    }

    storage->fs()->interrupt();
    cv.notify_all();
  };

  const auto toggle_progress_reporting = [](auto *src, auto *tgt) {
    if (src->progress()->visible() && !tgt->progress()->visible()) {
      tgt->progress()->show();
    }
  };

  while (true) {
    const auto s = status.pop();

    switch (s) {
      case Status::DUMPER_DONE:
      case Status::LOADER_DONE:
        if (2 == ++finished) {
          break;
        }

        if (s == Status::DUMPER_DONE) {
          toggle_progress_reporting(dumper, loader);
        } else {
          toggle_progress_reporting(loader, dumper);
        }

        continue;

      case Status::DUMPER_EXCEPTION:
        loader->abort();
        set_current_exception(dumper_exception);
        break;

      case Status::LOADER_EXCEPTION:
        dumper->abort();
        set_current_exception(loader_exception);
        break;
    }

    break;
  }

  dumper_thread.join();
  loader_thread.join();

  if (current_exception) {
    std::rethrow_exception(current_exception);
  }

  // show metadata at the end, making sure it doesn't disappear in the noise
  loader->show_metadata(true);
}

}  // namespace copy
}  // namespace mysqlsh

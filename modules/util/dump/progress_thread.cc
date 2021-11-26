/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#include "modules/util/dump/progress_thread.h"

#include <cinttypes>

#include <limits>

#include "mysqlshdk/libs/textui/progress.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/dump/console_with_progress.h"

namespace mysqlsh {
namespace dump {

namespace {

constexpr auto k_ellipsis = "...";
constexpr auto k_done = "- done";
constexpr uint32_t k_update_every_ms = 250;

std::string format_seconds(double seconds) {
  const auto sec = static_cast<unsigned long long int>(seconds);

  return shcore::str_format("%02llu:%02llu:%02llus", sec / 3600ull,
                            (sec % 3600ull) / 60ull, sec % 60ull);
}

class Spinner_progress : public Progress_thread::Stage {
 public:
  Spinner_progress(const std::string &description, bool show_progress,
                   bool use_json)
      : Stage(description, show_progress), m_spinner(description, use_json) {}

 protected:
  void draw() override { m_spinner.update(); }

  void on_display_done() override { m_spinner.done(k_done); }

  mysqlshdk::textui::Spinny_stick m_spinner;
};

class Numeric_progress : public Spinner_progress {
 public:
  Numeric_progress(const std::string &description, bool show_progress,
                   bool use_json, Progress_thread::Progress_config &&config)
      : Spinner_progress(description, show_progress, use_json),
        m_config(std::move(config)) {
    assert(m_config.current);
    assert(m_config.total);
  }

 protected:
  void on_update() override {
    const auto current = m_config.current();
    const auto total = m_config.total();
    const auto total_ready =
        m_config.is_total_known ? m_config.is_total_known() : true;

    m_label.resize(k_max_size);
    // space at the end is there to compensate for the optional tilde before the
    // total value
    const auto size = snprintf(
        &m_label[0], k_max_size, "%" PRIu64 " / %s%" PRIu64 "%s", current,
        total_ready ? "" : "~", total, total_ready ? " " : "");
    m_label.resize(size);

    m_spinner.set_right_label(m_label);

    if (total_ready && current >= total) {
      finish(false);
    }
  }

 private:
  // format is: current / ~total\0 or current / total \0
  // std::numeric_limits<T>::digits10 - number of decimal digits a type can
  // safely store, plus one to take into account values which are not safe
  // (i.e. 8-bit type can store two digit numbers safely, it can store three
  // digit numbers up to 255, but values greater than that are prone to
  // overflow)
  // 5: " / ~" + null byte
  static constexpr auto k_max_size =
      2 * (std::numeric_limits<uint64_t>::digits10 + 1) + 5;

  Progress_thread::Progress_config m_config;
  std::string m_label;
};

class Throughput_progress : public Progress_thread::Stage {
 public:
  Throughput_progress(const std::string &description, bool show_progress,
                      Progress_thread::Throughput_config &&config,
                      mysqlshdk::textui::Base_progress *progress,
                      std::recursive_mutex *mutex)
      : Stage(description, show_progress),
        m_config(std::move(config)),
        m_progress(progress),
        m_progress_mutex(mutex) {
    assert(m_config.current);
    assert(m_config.total);
  }

 private:
  void on_display_started() override {
    if (m_progress) {
      std::lock_guard<std::recursive_mutex> lock(*m_progress_mutex);
      m_progress->reset(m_config.items_full, m_config.items_abbrev,
                        m_config.item_singular, m_config.item_plural,
                        m_config.space_before_item, m_config.total_is_approx);
    }

    if (m_config.on_display_started) {
      m_config.on_display_started();
    }
  }

  void on_update() override {
    const auto initial = m_config.initial ? m_config.initial() : 0;
    const auto current = m_config.current();
    const auto total = m_config.total();
    const auto left_label = m_config.left_label ? m_config.left_label() : "";
    const auto right_label = m_config.right_label ? m_config.right_label() : "";

    if (m_progress) {
      std::lock_guard<std::recursive_mutex> lock(*m_progress_mutex);
      m_progress->set_total(total, initial);
      m_progress->set_current(current);
      m_progress->set_left_label(left_label);
      m_progress->set_right_label(right_label);
    }
  }

  void draw() override { draw(false); }

  void on_display_done() override {
    on_update();
    draw(true);
    on_display_terminated();
  }

  void on_display_terminated() override {
    if (m_progress) {
      std::lock_guard<std::recursive_mutex> lock(*m_progress_mutex);
      m_progress->shutdown();
    }
  }

  void draw(bool force) {
    if (m_progress) {
      const auto lock =
          force ? std::unique_lock<std::recursive_mutex>(*m_progress_mutex)
                : std::unique_lock<std::recursive_mutex>(*m_progress_mutex,
                                                         std::try_to_lock);

      if (lock.owns_lock()) {
        m_progress->show_status(force);
      }
    }
  }

  Progress_thread::Throughput_config m_config;
  mysqlshdk::textui::Base_progress *m_progress;
  std::recursive_mutex *m_progress_mutex;
};

}  // namespace

std::string Progress_thread::Duration::current_time() {
  return mysqlshdk::utils::fmttime("%Y-%m-%d %T");
}

const std::string &Progress_thread::Duration::started_at() const {
  return m_started_at;
}

const std::string &Progress_thread::Duration::finished_at() const {
  validate();

  return m_finished_at;
}

double Progress_thread::Duration::current() const {
  mysqlshdk::utils::Duration duration = *this;
  duration.finish();
  return duration.seconds_elapsed();
}

double Progress_thread::Duration::seconds() const {
  validate();

  return mysqlshdk::utils::Duration::seconds_elapsed();
}

std::string Progress_thread::Duration::to_string() const {
  return format_seconds(seconds());
}

void Progress_thread::Duration::start() {
  if (!m_started_at.empty()) {
    throw std::logic_error("Already started");
  }

  m_started_at = current_time();
  mysqlshdk::utils::Duration::start();
}

void Progress_thread::Duration::finish() {
  mysqlshdk::utils::Duration::finish();
  m_finished_at = current_time();
}

void Progress_thread::Duration::validate() const {
  if (m_finished_at.empty()) {
    throw std::logic_error("Still in progress");
  }
}

Progress_thread::Stage::Stage(const std::string &description,
                              bool show_progress)
    : m_description(description), m_show_progress(show_progress) {}

void Progress_thread::Stage::start() {
  log_debug("%s%s", description().c_str(), k_ellipsis);

  if (!m_show_progress) {
    current_console()->print_status(description() + k_ellipsis);
  }

  m_duration.start();
}

void Progress_thread::Stage::finish(bool wait) {
  bool has_finished = false;

  {
    std::lock_guard<std::mutex> lock(m_finished_mutex);

    if (!m_finished) {
      m_finished = true;
      has_finished = true;
    }
  }

  if (has_finished) {
    m_duration.finish();
    m_finished_cv.notify_one();

    if (wait) {
      wait_for_display_done();
    }

    if (!m_terminated) {
      log_info("%s %s, duration: %f seconds", description().c_str(), k_done,
               duration().seconds());

      if (!m_show_progress) {
        current_console()->print_status(description() + " " + k_done);
      }
    }
  }
}

void Progress_thread::Stage::display() {
  on_display_started();

  while (!m_finished) {
    on_update();

    if (m_show_progress) {
      draw();
    }

    wait_for_finish();
  }

  if (m_show_progress) {
    if (m_terminated) {
      on_display_terminated();
    } else {
      on_display_done();
    }
  }

  {
    std::lock_guard<std::mutex> lock(m_display_done_mutex);
    m_display_done = true;
  }

  m_display_done_cv.notify_one();
}

void Progress_thread::Stage::on_display_started() {}

void Progress_thread::Stage::on_update() {}

void Progress_thread::Stage::on_display_terminated() {}

void Progress_thread::Stage::on_display_done() {}

void Progress_thread::Stage::wait_for_finish() {
  std::unique_lock<std::mutex> lock(m_finished_mutex);

  m_finished_cv.wait_for(lock, std::chrono::milliseconds(k_update_every_ms),
                         [this]() { return m_finished; });
}

void Progress_thread::Stage::wait_for_display_done() {
  std::unique_lock<std::mutex> lock(m_display_done_mutex);

  m_display_done_cv.wait(lock, [this]() { return m_display_done; });
}

void Progress_thread::Stage::terminate() {
  m_terminated = true;
  finish(false);
}

Progress_thread::Progress_thread(const std::string &description,
                                 bool show_progress)
    : m_description(description), m_show_progress(show_progress) {
  m_json_output = "off" != mysqlsh::current_shell_options()->get().wrap_json;

  if (m_show_progress) {
    if (m_json_output) {
      m_progress = std::make_unique<mysqlshdk::textui::Json_progress>();
    } else {
      m_progress = std::make_unique<mysqlshdk::textui::Text_progress>();
    }
  }

  m_console =
      std::make_unique<Scoped_console>(std::make_shared<Console_with_progress>(
          m_progress.get(), &m_progress_mutex));
}

Progress_thread::~Progress_thread() { kill_thread(); }

void Progress_thread::start() {
  assert(!m_progress_thread);

  m_progress_thread =
      std::make_unique<std::thread>(mysqlsh::spawn_scoped_thread([this]() {
        try {
          while (true) {
            auto stage = m_schedule.pop();

            if (m_interrupt) {
              return;
            }

            if (!stage) {
              break;
            }

            m_current_stage = stage;

            stage->display();

            if (m_interrupt) {
              return;
            }
          }
        } catch (...) {
          m_exception = std::current_exception();
          emergency_shutdown();
        }
      }));

  m_total_duration.start();
}

Progress_thread::Stage *Progress_thread::start_stage(
    const std::string &description) {
  return start_stage<Spinner_progress>(description, m_json_output);
}

Progress_thread::Stage *Progress_thread::start_stage(
    const std::string &description, Progress_config &&config) {
  return start_stage<Numeric_progress>(description, m_json_output,
                                       std::move(config));
}

Progress_thread::Stage *Progress_thread::start_stage(
    const std::string &description, Throughput_config &&config) {
  return start_stage<Throughput_progress>(description, std::move(config),
                                          m_progress.get(), &m_progress_mutex);
}

void Progress_thread::finish() {
  shutdown();
  wait_for_thread();
  m_total_duration.finish();
  rethrow();
}

void Progress_thread::terminate() { emergency_shutdown(); }

template <class T, class... Args>
Progress_thread::Stage *Progress_thread::start_stage(
    const std::string &description, Args &&... args) {
  if (!m_progress_thread) {
    start();
  }

  std::unique_ptr<Stage> stage_ptr = std::make_unique<T>(
      description, m_show_progress, std::forward<Args>(args)...);
  auto stage = stage_ptr.get();

  m_stages.emplace_back(std::move(stage_ptr));

  stage->start();
  m_schedule.push(stage);

  return stage;
}

void Progress_thread::shutdown() {
  m_schedule.shutdown(1);

  for (const auto &stage : m_stages) {
    stage->finish(false);
  }
}

void Progress_thread::emergency_shutdown() {
  if (!m_interrupt) {
    m_interrupt = true;
    m_schedule.shutdown(1);

    for (const auto &stage : m_stages) {
      stage->terminate();
    }
  }
}

void Progress_thread::wait_for_thread() {
  if (m_progress_thread) {
    m_progress_thread->join();
  }

  m_progress_thread.reset();
}

void Progress_thread::rethrow() {
  if (m_exception) {
    std::rethrow_exception(m_exception);
  }
}

void Progress_thread::kill_thread() {
  emergency_shutdown();
  wait_for_thread();
}

}  // namespace dump
}  // namespace mysqlsh

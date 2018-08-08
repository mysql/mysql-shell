/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_TEXTUI_TEXT_PROGRESS_H_
#define MYSQLSHDK_LIBS_TEXTUI_TEXT_PROGRESS_H_

#include <chrono>
#include <string>

namespace mysqlshdk {
namespace textui {
/**
 * Class calculating throughput based on total bytes transferred.
 */
class Throughput final {
 public:
  Throughput();
  Throughput(const Throughput &other) = default;
  Throughput(Throughput &&other) = default;

  Throughput &operator=(const Throughput &other) = default;
  Throughput &operator=(Throughput &&other) = default;

  ~Throughput() = default;

  /**
   * Store time point with given total bytes
   *
   * @param total_bytes Current value of total bytes transferred.
   */
  void push(uint64_t total_bytes);

  /**
   * Calculate transfer rate using `k_moving_average_points`-point moving
   * average.
   *
   * @return Current transfer rate in bytes per second.
   */
  uint64_t rate() const;

 private:
  struct Snapshot {
    int64_t bytes;
    std::chrono::steady_clock::time_point time;
  };

  static constexpr const unsigned int k_moving_average_points = 16;
  Snapshot m_history[k_moving_average_points];
  uint16_t m_index = 0;
  uint16_t m_index_prev = 0;
};

inline void Throughput::push(uint64_t total_bytes) {
  const auto current_time = std::chrono::steady_clock::now();
  const auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                             current_time - m_history[m_index_prev].time)
                             .count();
  if (time_diff >= 500 /* ms */) {
    m_history[m_index].bytes = total_bytes;
    m_history[m_index].time = current_time;
    m_index_prev = m_index;
    m_index = (m_index + 1) % k_moving_average_points;
  }
}

/**
 * Class that represents No-operation progress.
 */
class IProgress {
 public:
  IProgress() = default;

  IProgress(const IProgress &other) = default;
  IProgress(IProgress &&other) = default;

  IProgress &operator=(const IProgress &other) = default;
  IProgress &operator=(IProgress &&other) = default;

  virtual ~IProgress() = default;

  virtual void show_status(bool force = false) { (void)force; }
  virtual void current(uint64_t /* value */) {}
  virtual void total(uint64_t /* value */) {}
  virtual void clear_status() {}
  virtual void shutdown() {}
};

/**
 * Class that generates one-line text progress information.
 */
class Text_progress final : public IProgress {
 public:
  Text_progress() : m_status(80, ' ') {}

  Text_progress(const Text_progress &other) = default;
  Text_progress(Text_progress &&other) = default;

  Text_progress &operator=(const Text_progress &other) = default;
  Text_progress &operator=(Text_progress &&other) = default;

  ~Text_progress() override = default;

  /**
   * Update and print progress information status.
   *
   * @param force If true, force status refresh and print.
   */
  void show_status(bool force = false) override;

  /**
   * Erase currently visible progress information status.
   */
  void clear_status() override;

  /**
   * Update done work value.
   *
   * @param value Absolute value of done work.
   */
  void current(uint64_t value) override {
    m_current = value;
    m_throughput.push(value);
    m_changed = true;
  }

  /**
   * Set total value of work to-do.
   *
   * @param value Absolute value of work to-do.
   */
  void total(uint64_t value) override { m_total = value; }

  /**
   * Finish progress status display.
   */
  void shutdown() override;

 private:
  unsigned long long m_current = 0;
  unsigned long long m_total = 0;
  bool m_changed = true;
  Throughput m_throughput;
  unsigned int m_last_status_size = 0;  //< Last displayed status length
  std::string m_status;
  std::chrono::steady_clock::time_point
      m_refresh_clock;  //< Last status refresh

  /**
   * Calculates work done to total work to-do ratio.
   *
   * @return Ratio of work done.
   */
  double ratio() const noexcept {
    return (m_total > 0) ? (static_cast<double>(m_current) / m_total) : 0;
  }

  /**
   * Calculates work done to total work to-do ratio in percent.
   *
   * @return Ratio of work done in percent.
   */
  int percent() const noexcept { return ratio() * 100; }

  /**
   * Renders progress information status string.
   */
  void render_status();
};

class Json_progress final : public IProgress {
 public:
  Json_progress() : m_status(80, ' ') {}
  Json_progress(const Json_progress &other) = default;
  Json_progress(Json_progress &&other) = default;

  Json_progress &operator=(const Json_progress &other) = default;
  Json_progress &operator=(Json_progress &&other) = default;

  ~Json_progress() = default;

  /**
   * Update and print progress information status.
   *
   * @param force If true, force status refresh and print.
   */
  void show_status(bool force = false) override;

  /**
   * Update done work value.
   *
   * @param value Absolute value of done work.
   */
  void current(uint64_t value) override {
    m_current = value;
    m_throughput.push(value);
    m_changed = true;
  }

  /**
   * Set total value of work to-do.
   *
   * @param value Absolute value of work to-do.
   */
  void total(uint64_t value) override { m_total = value; }

 private:
  unsigned long long m_current = 0;
  unsigned long long m_total = 0;
  bool m_changed = true;
  Throughput m_throughput;
  std::string m_status;
  std::chrono::steady_clock::time_point
      m_refresh_clock;  //< Last status refresh

  double ratio() const noexcept {
    return (m_total > 0) ? (static_cast<double>(m_current) / m_total) : 0;
  }

  int percent() const noexcept { return ratio() * 100; }

  /**
   * Renders progress information status string.
   */
  void render_status();
};
}  // namespace textui
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_TEXTUI_TEXT_PROGRESS_H_

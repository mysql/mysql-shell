/*
 * Copyright (c) 2020, 2020, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_REST_RETRY_STRATEGY_H_
#define MYSQLSHDK_LIBS_REST_RETRY_STRATEGY_H_

#include <chrono>
#include <string>

#include "mysqlshdk/libs/rest/response.h"
#include "mysqlshdk/libs/utils/nullable.h"

namespace mysqlshdk {
namespace rest {

/**
 * Utility class to setup and keep track of a retry strategy when errors are
 * reported by the server when processing a request.
 *
 * The retry strategy is automatically enabled for any error during the request.
 *
 * In addition, retry can be enabled for:
 *
 * - Server error responses (status>=500)
 * - Specific server responses
 *
 * A successful request is the first stop criteria (unless configured to NOT be
 * that way).
 *
 * In addition the following stop criterias can be defined:
 *
 * - Max number of retries.
 * - Max ellapsed time.
 *
 * A base sleep time must be provided to wait between each retry.
 */
class Retry_strategy {
 public:
  explicit Retry_strategy(uint32_t base_sleep_time)
      : m_base_sleep_time(std::chrono::seconds(base_sleep_time)),
        m_retry_count(0),
        m_next_sleep_time(std::chrono::seconds(0)),
        m_ellapsed_time(std::chrono::seconds(0)) {}

  void set_max_attempts(uint32_t value) { m_max_attempts = value; }
  void set_max_ellapsed_time(uint32_t seconds) {
    m_max_ellapsed_time = std::chrono::seconds(seconds);
  }
  void add_retriable_status(Response::Status_code code) {
    m_retriable_status.insert(code);
  }
  void set_retry_on_server_errors(bool value) {
    m_retry_on_server_errors = value;
  }

  bool should_retry(const mysqlshdk::utils::nullable<Response::Status_code>
                        &response_status_code = {});

  void wait_for_retry();

  void init();

  uint32_t get_retry_count() const { return m_retry_count; }
  std::chrono::seconds get_ellapsed_time() const { return m_ellapsed_time; }
  std::chrono::seconds get_next_sleep_time() const { return m_next_sleep_time; }
  std::chrono::seconds get_max_ellapsed_time() const {
    return m_max_ellapsed_time.is_null() ? std::chrono::seconds(0)
                                         : *m_max_ellapsed_time;
  }

 protected:
  std::chrono::seconds m_base_sleep_time;
  uint32_t m_retry_count;

 private:
  virtual std::chrono::seconds next_sleep_time(
      const mysqlshdk::utils::nullable<Response::Status_code>
          &response_status_code = {});

  // Retry criteria members
  mysqlshdk::utils::nullable<uint32_t> m_max_attempts;
  mysqlshdk::utils::nullable<std::chrono::seconds> m_max_ellapsed_time;
  std::set<Response::Status_code> m_retriable_status;
  bool m_retry_on_server_errors = false;

  // Tracking members
  std::chrono::system_clock::time_point m_start_time;
  std::chrono::seconds m_next_sleep_time;
  std::chrono::seconds m_ellapsed_time;
};

/**
 * Exponential back-off retry strategy: it varies the wait time between each
 * retry. The wait time is calculated with full jitter as:
 *
 * exponential_wait_time = base_wait_time * (exponent_grow_factor ^ attemp)
 * exponential_wait_time = min(exponential_wait_time, max_time_between_retries)
 *
 * wait_time = random(0, exponential_wait_time)
 *
 * It can handle guarantee wait time for throttling scenarios calculating wait
 * time as follows:
 *
 * wait_time = exponential_wait_time/2 + random(0, exponential_wait_time/2)
 */
class Exponential_backoff_retry : public Retry_strategy {
 public:
  Exponential_backoff_retry(uint32_t base_sleep_time,
                            uint32_t exponent_grow_factor,
                            uint32_t max_wait_between_calls)
      : Retry_strategy(base_sleep_time),
        m_exponent_grow_factor(exponent_grow_factor),
        m_max_wait_between_calls(max_wait_between_calls) {}

  void set_equal_jitter_for_throttling(bool value);

 private:
  uint32_t m_exponent_grow_factor;
  uint32_t m_max_wait_between_calls;
  bool m_equal_jitter_for_throttling = false;

  std::chrono::seconds get_wait_time_with_equal_jitter() const;
  std::chrono::seconds get_wait_time_with_full_jitter() const;

  std::chrono::seconds next_sleep_time(
      const mysqlshdk::utils::nullable<Response::Status_code>
          &response_status_code = {}) override;
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_REST_RETRY_STRATEGY_H_

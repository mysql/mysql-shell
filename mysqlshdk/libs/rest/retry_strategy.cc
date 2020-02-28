/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#include <stdlib.h>
#include <chrono>
#include <cmath>
#include <thread>

#include "mysqlshdk/libs/rest/retry_strategy.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace rest {

void Retry_strategy::init() {
  if (m_max_attempts.is_null() && m_max_ellapsed_time.is_null()) {
    throw std::logic_error(
        "A stop criteria must be defined to avoid infinite retries.");
  }

  m_start_time = std::chrono::system_clock::now();
  m_retry_count = 0;
}

bool Retry_strategy::should_retry(
    const mysqlshdk::utils::nullable<Response::Status_code>
        &response_status_code) {
  // The next sleep time may vary depending on the retry strategy, so it has to
  // be calculated here
  m_next_sleep_time = next_sleep_time(response_status_code);

  // If max attempts criteria is set, validates we are still on the allowed
  // number of attempts
  if (!m_max_attempts.is_null() && m_retry_count >= *m_max_attempts) {
    return false;
  }

  // If max ellapsed time critera is set, validates the next call is still on
  // the expected time frame
  if (!m_max_ellapsed_time.is_null()) {
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - m_start_time);

    // Considering the next sleep time on the criteria
    duration += m_base_sleep_time;

    // Only allow if the next retry is still on the max time frame
    if (duration >= *m_max_ellapsed_time) return false;
  }

  // Validation for status codes return FALSE if:
  // - The status code is NOT registered to allow retries AND
  // - Retry on server errors is disabled OR
  // - Retry on server errors is enabled but it is not a server error
  if (!response_status_code.is_null()) {
    // If the status code NOT specifically configured to allow retries...
    if (m_retriable_status.find(*response_status_code) ==
            m_retriable_status.end() &&
        (!m_retry_on_server_errors ||
         *response_status_code <
             Response::Status_code::INTERNAL_SERVER_ERROR)) {
      return false;
    }
  }

  // Any other situation would cause a retry logic to continue
  return true;
}

std::chrono::seconds Retry_strategy::next_sleep_time(
    const mysqlshdk::utils::nullable<Response::Status_code>
        & /*response_status_code*/) {
  return m_base_sleep_time;
}

void Retry_strategy::wait_for_retry() {
  std::this_thread::sleep_for(m_next_sleep_time);
  m_retry_count++;
}

std::chrono::seconds Exponential_backoff_retry::next_sleep_time(
    const mysqlshdk::utils::nullable<Response::Status_code>
        &response_status_code) {
  std::chrono::seconds ret_val;
  if (!response_status_code.is_null() &&
      Response::Status_code::TOO_MANY_REQUESTS == *response_status_code &&
      m_equal_jitter_for_throttling)
    ret_val = get_wait_time_with_equal_jitter();
  else
    ret_val = get_wait_time_with_full_jitter();

  return ret_val;
}

void Exponential_backoff_retry::set_equal_jitter_for_throttling(bool value) {
  // Ensures the throttling response is allowed for retries
  add_retriable_status(
      mysqlshdk::rest::Response::Status_code::TOO_MANY_REQUESTS);
  m_equal_jitter_for_throttling = value;
}

/**
 * Exponential wait time is calculated as follows:
 *
 * wait_time = base_wait_time * (exponential_grow_factor^attempts)
 *
 * This would cause every next attempt to wait more and more.
 *
 * On a multithreaded senario, X threads would be waiting at retry number n and
 * they all may do a retry at around the same time frame and only one of them
 * succeeding at each iteration.
 *
 * Solution, add jitter: the wait time is some random value between 0 and the
 * calculated wait_time.
 *
 * This distributes the real wait_time on such range, causing more threads to be
 * successful in the same amount of time as not all of them will be waiting the
 * same.
 */
std::chrono::seconds Exponential_backoff_retry::get_wait_time_with_full_jitter()
    const {
  auto max_sleep =
      std::min(m_base_sleep_time.count() *
                   std::pow(m_exponent_grow_factor, m_retry_count + 1),
               static_cast<double>(m_max_wait_between_calls));

  auto wait_time = fmod(rand(), max_sleep);

  return std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::duration<double>(wait_time));
}

/**
 * Using full jitter does not guarantee any wait time between attempts as the
 * random selection of the next wait time could result in a los wait time, even
 * 0.
 *
 * Equal jitter is abut making jitter be applied on a range equal to a
 * guaranteed wait time.
 *
 * This is desired i.e. when the server is too busy and sends: TOO_MANY_REQUESTS
 */
std::chrono::seconds
Exponential_backoff_retry::get_wait_time_with_equal_jitter() const {
  auto max_sleep =
      std::min(m_base_sleep_time.count() *
                   std::pow(m_exponent_grow_factor, m_retry_count + 1),
               static_cast<double>(m_max_wait_between_calls));

  auto wait_time = (max_sleep / 2) + fmod(rand(), max_sleep / 2);

  return std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::duration<double>(wait_time));
}

}  // namespace rest
}  // namespace mysqlshdk

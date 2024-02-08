/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/rest/retry_strategy.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <random>
#include <stdexcept>
#include <string_view>
#include <thread>

#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace rest {

namespace {

std::chrono::seconds to_seconds(double s) {
  return std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::duration<double>(s));
}

int random_int() {
  static thread_local std::mt19937 g{std::random_device()()};
  std::uniform_int_distribution<int> d(0, std::numeric_limits<int>::max());
  return d(g);
}

}  // namespace

std::unique_ptr<Retry_strategy> default_retry_strategy() {
  // Throttling handling: equal jitter guarantees some wait time before next
  // attempt
  return Retry_strategy_builder(1, 2, 60, true)
      // Retry up to 10 times
      .set_max_attempts(10)
      // Keep retrying for 10 minutes
      .set_max_elapsed_time(600)
      // Retry continues in responses with codes about server errors >=500
      .retry_on_server_errors()
      // retry if the authorization header got too old
      .retry_on(
          Response::Status_code::UNAUTHORIZED,
          "The Authorization header has a date that is either too early or too "
          "late, check that your local clock is correct")
      // retry in case of partial file error reported by CURL, can happen due to
      // a network error, when received data is shorter than reported
      .retry_on(Error_code::PARTIAL_FILE)
      // retry if operation times out, sometimes servers get stuck...
      .retry_on(Error_code::OPERATION_TIMEDOUT)
      // retry when server returns an empty response
      .retry_on(Error_code::GOT_NOTHING)
      // retry in case of failure in receiving network data
      .retry_on(Error_code::RECV_ERROR)
      .build();
}

void IRetry_strategy::IRetry_delay::reset() { m_next_sleep_time = {}; }

void IRetry_strategy::IRetry_delay::set_next_sleep_time(
    const Retry_request &request) {
  m_next_sleep_time = handle(request);
}

IRetry_strategy::IRetry_strategy(std::unique_ptr<IRetry_delay> delay)
    : m_delay(std::move(delay)) {
  assert(m_delay);
}

void IRetry_strategy::reset() {
  m_delay->reset();

  for (const auto &condition : m_conditions) {
    condition->reset();
  }
}

[[nodiscard]] bool IRetry_strategy::should_retry(const Retry_request &request) {
  m_delay->set_next_sleep_time(request);

  for (const auto &condition : m_conditions) {
    if (const auto handled = condition->should_retry(request);
        handled.has_value()) {
      return *handled;
    }
  }

  throw std::logic_error("Retry request was not handled");
}

void IRetry_strategy::wait_for_retry() const {
  std::this_thread::sleep_for(m_delay->next_sleep_time());
}

IRetry_strategy::Constant_delay::Constant_delay(uint32_t delay)
    : m_delay(delay) {}

[[nodiscard]] std::chrono::seconds IRetry_strategy::Constant_delay::handle(
    const Unknown_error &) {
  return delay();
}

[[nodiscard]] std::chrono::seconds IRetry_strategy::Constant_delay::handle(
    const Response_error &) {
  return delay();
}

[[nodiscard]] std::chrono::seconds IRetry_strategy::Constant_delay::handle(
    const Connection_error &) {
  return delay();
}

IRetry_strategy::Exponential_backoff_delay::Exponential_backoff_delay(
    uint32_t base_delay, uint32_t exponent_grow_factor,
    uint32_t max_wait_between_calls)
    : m_base_delay(base_delay),
      m_exponent_grow_factor(exponent_grow_factor),
      m_max_wait_between_calls(max_wait_between_calls) {}

[[nodiscard]] double
IRetry_strategy::Exponential_backoff_delay::next_max_delay() {
  return std::min(m_base_delay * std::pow(m_exponent_grow_factor, ++m_attempt),
                  m_max_wait_between_calls);
}

[[nodiscard]] std::chrono::seconds
IRetry_strategy::Exponential_backoff_delay::equal_jitter() {
  const auto max_delay = next_max_delay() / 2;
  return to_seconds(max_delay + fmod(random_int(), max_delay));
}

[[nodiscard]] std::chrono::seconds
IRetry_strategy::Exponential_backoff_delay::full_jitter() {
  return to_seconds(fmod(random_int(), next_max_delay()));
}

void IRetry_strategy::Exponential_backoff_delay::reset() {
  IRetry_delay::reset();
  m_attempt = 0;
}

[[nodiscard]] std::chrono::seconds
IRetry_strategy::Exponential_backoff_delay::handle(const Unknown_error &) {
  return full_jitter();
}

[[nodiscard]] std::chrono::seconds
IRetry_strategy::Exponential_backoff_delay::handle(const Response_error &) {
  return full_jitter();
}

[[nodiscard]] std::chrono::seconds
IRetry_strategy::Exponential_backoff_delay::handle(const Connection_error &) {
  return full_jitter();
}

[[nodiscard]] std::optional<bool> IRetry_strategy::Handle_all::handle(
    const Unknown_error &) {
  return handle_all();
}

[[nodiscard]] std::optional<bool> IRetry_strategy::Handle_all::handle(
    const Response_error &) {
  return handle_all();
}

[[nodiscard]] std::optional<bool> IRetry_strategy::Handle_all::handle(
    const Connection_error &) {
  return handle_all();
}

[[nodiscard]] std::optional<bool> IRetry_strategy::Ignore_all::handle_all() {
  return {};
}

[[nodiscard]] std::optional<bool> IRetry_strategy::Reject_all::handle_all() {
  return false;
}

[[nodiscard]] std::optional<bool> IRetry_strategy::Accept_all::handle_all() {
  return true;
}

IRetry_strategy::Retry_count::Retry_count(uint32_t max_attempts)
    : m_max_attempts(max_attempts) {}

void IRetry_strategy::Retry_count::reset() { m_retry_count = 0; }

[[nodiscard]] std::optional<bool> IRetry_strategy::Retry_count::handle_all() {
  // validates we are still on the allowed number of attempts
  if (m_retry_count >= m_max_attempts) {
    return false;
  }

  ++m_retry_count;

  return {};
}

IRetry_strategy::Retry_time::Retry_time(std::chrono::seconds max_elapsed_time,
                                        const IRetry_delay *delay)
    : m_delay(delay), m_max_elapsed_time(max_elapsed_time) {
  assert(m_delay);
}

void IRetry_strategy::Retry_time::reset() {
  m_start_time = std::chrono::system_clock::now();
}

[[nodiscard]] std::optional<bool> IRetry_strategy::Retry_time::handle_all() {
  // validates the next call is still on the expected time frame
  const auto elapsed = std::chrono::system_clock::now() - m_start_time;

  // Only allow if the next retry is still on the max time frame
  if ((elapsed + m_delay->next_sleep_time()) >= m_max_elapsed_time) {
    return false;
  }

  return {};
}

class Retry_strategy::Retry_unknown_errors : public Ignore_all {
 public:
  Retry_unknown_errors() = default;

  Retry_unknown_errors(const Retry_unknown_errors &) = default;
  Retry_unknown_errors(Retry_unknown_errors &&) = default;

  Retry_unknown_errors &operator=(const Retry_unknown_errors &) = default;
  Retry_unknown_errors &operator=(Retry_unknown_errors &&) = default;

  ~Retry_unknown_errors() override = default;

 private:
  [[nodiscard]] std::optional<bool> handle(const Unknown_error &) override {
    return true;
  }
};

class Retry_strategy::Retry_response_error : public Ignore_all {
 public:
  Retry_response_error() = default;

  Retry_response_error(const Retry_response_error &) = default;
  Retry_response_error(Retry_response_error &&) = default;

  Retry_response_error &operator=(const Retry_response_error &) = default;
  Retry_response_error &operator=(Retry_response_error &&) = default;

  ~Retry_response_error() override = default;

  void retry_on_server_errors() { m_retry_on_server_errors = true; }

  void retry_on(Response::Status_code code) {
    m_retriable_statuses.emplace(code);
  }

  void retry_on(Response::Status_code code, const std::string &msg) {
    m_retriable_messages[code].emplace(msg);
  }

 private:
  [[nodiscard]] std::optional<bool> handle(
      const Response_error &error) override {
    const auto code = error.status_code();

    // Retry on server errors if configured so
    if (m_retry_on_server_errors &&
        code >= Response::Status_code::INTERNAL_SERVER_ERROR) {
      return true;
    }

    // Retry on the configured response codes
    if (m_retriable_statuses.count(code)) {
      return true;
    }

    if (const auto msgs = m_retriable_messages.find(code);
        m_retriable_messages.end() != msgs) {
      const std::string_view error_msg = error.what();

      for (const auto &msg : msgs->second) {
        // Retry on the configured response errors
        if (std::string::npos != error_msg.find(msg)) {
          return true;
        }
      }
    }

    return {};
  }

  std::unordered_set<Response::Status_code> m_retriable_statuses;
  std::unordered_map<Response::Status_code, std::unordered_set<std::string>>
      m_retriable_messages;
  bool m_retry_on_server_errors = false;
};

class Retry_strategy::Retry_connection_error : public Ignore_all {
 public:
  Retry_connection_error() = default;

  Retry_connection_error(const Retry_connection_error &) = default;
  Retry_connection_error(Retry_connection_error &&) = default;

  Retry_connection_error &operator=(const Retry_connection_error &) = default;
  Retry_connection_error &operator=(Retry_connection_error &&) = default;

  ~Retry_connection_error() override = default;

  void retry_on(Error_code code) { m_retriable_codes.emplace(code); }

 private:
  [[nodiscard]] std::optional<bool> handle(
      const Connection_error &error) override {
    // Retry on the configured error codes
    if (m_retriable_codes.count(error.code())) {
      return true;
    }

    return {};
  }

  std::unordered_set<Error_code> m_retriable_codes;
};

Retry_strategy::Retry_strategy(std::unique_ptr<IRetry_delay> d)
    : IRetry_strategy(std::move(d)) {}

void Retry_strategy::retry_on_server_errors() {
  m_response_error->retry_on_server_errors();
}

void Retry_strategy::retry_on(Response::Status_code code) {
  m_response_error->retry_on(code);
}

void Retry_strategy::retry_on(Response::Status_code code,
                              const std::string &msg) {
  m_response_error->retry_on(code, msg);
}

void Retry_strategy::retry_on(Error_code code) {
  m_connection_error->retry_on(code);
}

class Retry_strategy_builder::Equal_jitter_for_throttling
    : public Retry_strategy::Exponential_backoff_delay {
 public:
  Equal_jitter_for_throttling() = delete;

  Equal_jitter_for_throttling(uint32_t base_delay,
                              uint32_t exponent_grow_factor,
                              uint32_t max_wait_between_calls)
      : Exponential_backoff_delay(base_delay, exponent_grow_factor,
                                  max_wait_between_calls) {}

  Equal_jitter_for_throttling(const Equal_jitter_for_throttling &) = default;
  Equal_jitter_for_throttling(Equal_jitter_for_throttling &&) = default;

  Equal_jitter_for_throttling &operator=(const Equal_jitter_for_throttling &) =
      default;
  Equal_jitter_for_throttling &operator=(Equal_jitter_for_throttling &&) =
      default;

  ~Equal_jitter_for_throttling() override = default;

 private:
  [[nodiscard]] std::chrono::seconds handle(
      const Response_error &error) override {
    if (Response::Status_code::TOO_MANY_REQUESTS == error.status_code()) {
      return equal_jitter();
    } else {
      return full_jitter();
    }
  }
};

Retry_strategy_builder::Retry_strategy_builder() {
  m_response_error = std::make_unique<Retry_strategy::Retry_response_error>();
  m_connection_error =
      std::make_unique<Retry_strategy::Retry_connection_error>();
}

Retry_strategy_builder::Retry_strategy_builder(uint32_t sleep_time)
    : Retry_strategy_builder() {
  m_delay = std::make_unique<Retry_strategy::Constant_delay>(sleep_time);
}

Retry_strategy_builder::Retry_strategy_builder(uint32_t base_delay,
                                               uint32_t exponent_grow_factor,
                                               uint32_t max_wait_between_calls,
                                               bool equal_jitter_for_throttling)
    : Retry_strategy_builder() {
  if (equal_jitter_for_throttling) {
    m_delay = std::make_unique<Equal_jitter_for_throttling>(
        base_delay, exponent_grow_factor, max_wait_between_calls);
    // Ensures the throttling response is going to be retried
    retry_on(Response::Status_code::TOO_MANY_REQUESTS);
  } else {
    m_delay = std::make_unique<Retry_strategy::Exponential_backoff_delay>(
        base_delay, exponent_grow_factor, max_wait_between_calls);
  }
}

// needs to be defined here as instance holds unique pointers of incomplete
// types
Retry_strategy_builder::~Retry_strategy_builder() = default;

Retry_strategy_builder &Retry_strategy_builder::set_max_attempts(
    uint32_t value) {
  m_retry_count = std::make_unique<Retry_strategy::Retry_count>(value);
  return *this;
}

Retry_strategy_builder &Retry_strategy_builder::set_max_elapsed_time(
    uint32_t seconds) {
  m_retry_time = std::make_unique<Retry_strategy::Retry_time>(
      std::chrono::seconds{seconds}, m_delay.get());
  return *this;
}

Retry_strategy_builder &Retry_strategy_builder::retry_on_server_errors() {
  m_response_error->retry_on_server_errors();
  return *this;
}

Retry_strategy_builder &Retry_strategy_builder::retry_on(
    Response::Status_code code) {
  m_response_error->retry_on(code);
  return *this;
}

Retry_strategy_builder &Retry_strategy_builder::retry_on(
    Response::Status_code code, const std::string &msg) {
  m_response_error->retry_on(code, msg);
  return *this;
}

Retry_strategy_builder &Retry_strategy_builder::retry_on(Error_code code) {
  m_connection_error->retry_on(code);
  return *this;
}

std::unique_ptr<Retry_strategy> Retry_strategy_builder::build() {
  if (!m_retry_count && !m_retry_time) {
    throw std::logic_error(
        "A stop criteria must be defined to avoid infinite retries.");
  }

  // this is a private constructor, std::make_unique cannot be used
  std::unique_ptr<Retry_strategy> strategy{
      new Retry_strategy(std::move(m_delay))};

  // stop criteria
  if (m_retry_count) {
    strategy->add_condition(std::move(m_retry_count));
  }

  if (m_retry_time) {
    strategy->add_condition(std::move(m_retry_time));
  }

  // always retry on unknown errors
  strategy->add_condition<Retry_strategy::Retry_unknown_errors>();

  // handle error codes
  strategy->m_response_error =
      strategy->add_condition(std::move(m_response_error));
  strategy->m_connection_error =
      strategy->add_condition(std::move(m_connection_error));

  // reject all others
  strategy->add_condition<Retry_strategy::Reject_all>();

  return strategy;
}

}  // namespace rest
}  // namespace mysqlshdk

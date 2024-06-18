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

#ifndef MYSQLSHDK_LIBS_REST_RETRY_STRATEGY_H_
#define MYSQLSHDK_LIBS_REST_RETRY_STRATEGY_H_

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "mysqlshdk/libs/rest/error.h"
#include "mysqlshdk/libs/rest/response.h"

namespace mysqlshdk {
namespace rest {

/**
 * Represents an unknown error.
 */
class Unknown_error final {};

/**
 * Strategy handles retry requests due to following reasons:
 */
using Retry_request =
    std::variant<Unknown_error, Response_error, Connection_error>;

/**
 * Strategy which decides whether or not to retry in response to a particular
 * situation.
 */
class IRetry_strategy {
 public:
  IRetry_strategy() = delete;

  IRetry_strategy(const IRetry_strategy &) = delete;
  IRetry_strategy(IRetry_strategy &&) = default;

  IRetry_strategy &operator=(const IRetry_strategy &) = delete;
  IRetry_strategy &operator=(IRetry_strategy &&) = default;

  virtual ~IRetry_strategy() = default;

  /**
   * Resets the strategy in preparation for a new series of retry decisions.
   */
  void reset();

  /**
   * Checks whether a retry should be attempted in response to the given
   * request.
   *
   * @param request A request to retry the operation.
   *
   * @returns true if request should be retried.
   */
  [[nodiscard]] bool should_retry(const Retry_request &request);

  /**
   * Provides the next sleep time.
   */
  [[nodiscard]] inline std::chrono::seconds next_sleep_time() const noexcept {
    return m_delay->next_sleep_time();
  }

  /**
   * Waits for the next retry.
   */
  void wait_for_retry() const;

 private:
  // this template needs to be before classes that inherit from it, but doesn't
  // need to be visible in the descendant classes
  template <typename T>
  class IRetry_handler {
   public:
    IRetry_handler() = default;

    IRetry_handler(const IRetry_handler &) = default;
    IRetry_handler(IRetry_handler &&) = default;

    IRetry_handler &operator=(const IRetry_handler &) = default;
    IRetry_handler &operator=(IRetry_handler &&) = default;

    virtual ~IRetry_handler() = default;

    [[nodiscard]] T handle(const Retry_request &request) {
      return std::visit(
          [this](auto &&r) {
            using R = std::decay_t<decltype(r)>;
            static_assert(std::is_same_v<R, Unknown_error> ||
                              std::is_same_v<R, Response_error> ||
                              std::is_same_v<R, Connection_error>,
                          "Unsupported retry request");
            return this->handle(r);
          },
          request);
    }

    virtual void reset() = 0;

   private:
    [[nodiscard]] virtual T handle(const Unknown_error &) = 0;
    [[nodiscard]] virtual T handle(const Response_error &) = 0;
    [[nodiscard]] virtual T handle(const Connection_error &) = 0;
  };

 protected:
  /**
   * Delay between subsequent retries.
   */
  class IRetry_delay : private IRetry_handler<std::chrono::seconds> {
   public:
    IRetry_delay() = default;

    IRetry_delay(const IRetry_delay &) = default;
    IRetry_delay(IRetry_delay &&) = default;

    IRetry_delay &operator=(const IRetry_delay &) = default;
    IRetry_delay &operator=(IRetry_delay &&) = default;

    ~IRetry_delay() override = default;

    /**
     * Resets the delay handler.
     */
    void reset() override;

    /**
     * Sets the next sleep time based on the given request.
     *
     * @param request A request to retry the operation.
     */
    void set_next_sleep_time(const Retry_request &request);

    /**
     * Provides the next sleep time.
     */
    [[nodiscard]] inline std::chrono::seconds next_sleep_time() const noexcept {
      return m_next_sleep_time;
    }

   private:
    std::chrono::seconds m_next_sleep_time{};
  };

  /**
   * Constant delay between retries.
   */
  class Constant_delay : public IRetry_delay {
   public:
    Constant_delay() = delete;

    /**
     * Initializes a constant delay.
     *
     * @param delay Delay between retries.
     */
    explicit Constant_delay(uint32_t delay);

    Constant_delay(const Constant_delay &) = default;
    Constant_delay(Constant_delay &&) = default;

    Constant_delay &operator=(const Constant_delay &) = default;
    Constant_delay &operator=(Constant_delay &&) = default;

    ~Constant_delay() override = default;

   private:
    /**
     * Provides the delay between retries.
     */
    [[nodiscard]] inline std::chrono::seconds delay() const noexcept {
      return m_delay;
    }

    [[nodiscard]] std::chrono::seconds handle(const Unknown_error &) override;

    [[nodiscard]] std::chrono::seconds handle(const Response_error &) override;

    [[nodiscard]] std::chrono::seconds handle(
        const Connection_error &) override;

    std::chrono::seconds m_delay;
  };

  /**
   * Exponential back-off delay: it varies the wait time between each retry. The
   * wait time is calculated with full jitter as:
   *
   * exponential_wait_time = base_wait_time * (exponent_grow_factor ^ attempt)
   * exponential_wait_time = min(exponential_wait_time,
   *                             max_time_between_retries)
   *
   * wait_time = random(0, exponential_wait_time)
   *
   * It can handle guarantee wait time for throttling scenarios calculating wait
   * time as follows:
   *
   * wait_time = exponential_wait_time/2 + random(0, exponential_wait_time/2)
   */
  class Exponential_backoff_delay : public IRetry_delay {
   public:
    Exponential_backoff_delay() = delete;

    /**
     * Initializes an exponential back-off delay.
     *
     * @param base_delay Base delay.
     * @param exponent_grow_factor Exponent grow factor.
     * @param max_wait_between_calls Max time to wait between calls.
     */
    Exponential_backoff_delay(uint32_t base_delay,
                              uint32_t exponent_grow_factor,
                              uint32_t max_wait_between_calls);

    Exponential_backoff_delay(const Exponential_backoff_delay &) = default;
    Exponential_backoff_delay(Exponential_backoff_delay &&) = default;

    Exponential_backoff_delay &operator=(const Exponential_backoff_delay &) =
        default;
    Exponential_backoff_delay &operator=(Exponential_backoff_delay &&) =
        default;

    ~Exponential_backoff_delay() override = default;

    /**
     * Using full jitter does not guarantee any wait time between attempts as
     * the random selection of the next wait time could result in a low wait
     * time, even 0.
     *
     * Equal jitter is about making jitter be applied on a range equal to a
     * guaranteed wait time.
     *
     * This is desired i.e. when the server is too busy and sends:
     * TOO_MANY_REQUESTS
     */
    [[nodiscard]] std::chrono::seconds equal_jitter();

    /**
     * Exponential wait time is calculated as follows:
     *
     * wait_time = base_wait_time * (exponential_grow_factor^attempts)
     *
     * This would cause every next attempt to wait more and more.
     *
     * On a multithreaded scenario, X threads would be waiting at retry number N
     * and they all may do a retry at around the same time frame and only one of
     * them succeeding at each iteration.
     *
     * Solution, add jitter: the wait time is some random value between 0 and
     * the calculated wait_time.
     *
     * This distributes the real wait_time on such range, causing more threads
     * to be successful in the same amount of time as not all of them will be
     * waiting the same.
     */
    [[nodiscard]] std::chrono::seconds full_jitter();

   private:
    void reset() override;

    [[nodiscard]] std::chrono::seconds handle(const Unknown_error &) override;

    [[nodiscard]] std::chrono::seconds handle(const Response_error &) override;

    [[nodiscard]] std::chrono::seconds handle(
        const Connection_error &) override;

    [[nodiscard]] double next_max_delay();

    uint32_t m_attempt = 0;
    uint32_t m_base_delay;
    uint32_t m_exponent_grow_factor;
    double m_max_wait_between_calls;
  };

  /**
   * Represents a condition which decides if a retry should happen.
   */
  class IRetry_condition : private IRetry_handler<std::optional<bool>> {
   public:
    IRetry_condition() = default;

    IRetry_condition(const IRetry_condition &) = default;
    IRetry_condition(IRetry_condition &&) = default;

    IRetry_condition &operator=(const IRetry_condition &) = default;
    IRetry_condition &operator=(IRetry_condition &&) = default;

    ~IRetry_condition() override = default;

    /**
     * Resets the condition handler.
     */
    using IRetry_handler::reset;

    /**
     * Checks whether a retry should be attempted in response to the given
     * request.
     *
     * @param request A request to retry the operation.
     *
     * @returns Not set if decision was not made, true if retry should happen.
     */
    [[nodiscard]] inline std::optional<bool> should_retry(
        const Retry_request &request) {
      return handle(request);
    }
  };

  /**
   * Handles all retry requests the same way.
   */
  class Handle_all : public IRetry_condition {
   public:
    Handle_all() = default;

    Handle_all(const Handle_all &) = default;
    Handle_all(Handle_all &&) = default;

    Handle_all &operator=(const Handle_all &) = default;
    Handle_all &operator=(Handle_all &&) = default;

    ~Handle_all() override = default;

   private:
    [[nodiscard]] virtual std::optional<bool> handle_all() = 0;

    [[nodiscard]] std::optional<bool> handle(const Unknown_error &) override;

    [[nodiscard]] std::optional<bool> handle(const Response_error &) override;

    [[nodiscard]] std::optional<bool> handle(const Connection_error &) override;
  };

  /**
   * Ignores all retry requests.
   */
  class Ignore_all : public Handle_all {
   public:
    Ignore_all() = default;

    Ignore_all(const Ignore_all &) = default;
    Ignore_all(Ignore_all &&) = default;

    Ignore_all &operator=(const Ignore_all &) = default;
    Ignore_all &operator=(Ignore_all &&) = default;

    ~Ignore_all() override = default;

   private:
    void reset() override {}

    [[nodiscard]] std::optional<bool> handle_all() override;
  };

  /**
   * Rejects all retry requests.
   */
  class Reject_all : public Handle_all {
   public:
    Reject_all() = default;

    Reject_all(const Reject_all &) = default;
    Reject_all(Reject_all &&) = default;

    Reject_all &operator=(const Reject_all &) = default;
    Reject_all &operator=(Reject_all &&) = default;

    ~Reject_all() override = default;

   private:
    void reset() override {}

    [[nodiscard]] std::optional<bool> handle_all() override;
  };

  /**
   * Accepts all retry requests.
   */
  class Accept_all : public Handle_all {
   public:
    Accept_all() = default;

    Accept_all(const Accept_all &) = default;
    Accept_all(Accept_all &&) = default;

    Accept_all &operator=(const Accept_all &) = default;
    Accept_all &operator=(Accept_all &&) = default;

    ~Accept_all() override = default;

   private:
    void reset() override {}

    [[nodiscard]] std::optional<bool> handle_all() override;
  };

  /**
   * Allows for specified number of retries.
   */
  class Retry_count : public Handle_all {
   public:
    Retry_count() = delete;

    /**
     * Sets the maximum number of retries.
     *
     * @param max_attempts Max number of retries.
     */
    explicit Retry_count(uint32_t max_attempts);

    Retry_count(const Retry_count &) = default;
    Retry_count(Retry_count &&) = default;

    Retry_count &operator=(const Retry_count &) = default;
    Retry_count &operator=(Retry_count &&) = default;

    ~Retry_count() override = default;

    /**
     * Provides number of retries so far.
     */
    [[nodiscard]] inline uint32_t retry_count() const noexcept {
      return m_retry_count;
    }

   private:
    void reset() override;

    [[nodiscard]] std::optional<bool> handle_all() override;

    uint32_t m_max_attempts;
    uint32_t m_retry_count = 0;
  };

  /**
   * Allows for retries for the specified amount of time.
   */
  class Retry_time : public Handle_all {
   public:
    Retry_time() = delete;

    /**
     * Sets the maximum elapsed time for the retries.
     *
     * @param max_elapsed_time Max elapsed time.
     * @param delay Provides waiting time for the next retry.
     */
    Retry_time(std::chrono::seconds max_elapsed_time,
               const IRetry_delay *delay);

    Retry_time(const Retry_time &) = delete;
    Retry_time(Retry_time &&) = default;

    Retry_time &operator=(const Retry_time &) = delete;
    Retry_time &operator=(Retry_time &&) = default;

    ~Retry_time() override = default;

   private:
    void reset() override;

    [[nodiscard]] std::optional<bool> handle_all() override;

    const IRetry_delay *m_delay;
    std::chrono::seconds m_max_elapsed_time;
    std::chrono::system_clock::time_point m_start_time{};
  };

  /**
   * Creates the strategy which is going to use given delay between retries.
   *
   * @param delay Delay between retries.
   */
  explicit IRetry_strategy(std::unique_ptr<IRetry_delay> delay);

  /**
   * Appends a condition to the list of conditions.
   *
   * NOTE: conditions are evaluated in FIFO order
   *
   * @param c New condition.
   *
   * @returns Pointer to the added condition.
   */
  template <typename T>
  T *add_condition(std::unique_ptr<T> c) {
    auto p = c.get();
    m_conditions.emplace_back(std::move(c));
    return p;
  }

  /**
   * Constructs a condition at the end of list of conditions.
   *
   * NOTE: conditions are evaluated in FIFO order
   *
   * @param args Arguments passed to the constructor.
   *
   * @returns Pointer to the added condition.
   */
  template <typename T, typename... Args>
  T *add_condition(Args &&...args) {
    return add_condition(std::make_unique<T>(std::forward<Args>(args)...));
  }

 private:
  std::unique_ptr<IRetry_delay> m_delay;
  std::vector<std::unique_ptr<IRetry_condition>> m_conditions;
};

/**
 * Utility class to setup and keep track of a retry strategy when errors are
 * reported by the server when processing a request.
 *
 * The retry strategy is automatically enabled for any error during the request.
 *
 * Use Retry_strategy_builder to configure and create this retry strategy.
 */
class Retry_strategy : public IRetry_strategy {
 public:
  Retry_strategy() = delete;

  Retry_strategy(const Retry_strategy &) = delete;
  Retry_strategy(Retry_strategy &&) = default;

  Retry_strategy &operator=(const Retry_strategy &) = delete;
  Retry_strategy &operator=(Retry_strategy &&) = default;

  ~Retry_strategy() override = default;

  /**
   * Enabled retries on status codes which represent server errors (5XX).
   */
  void retry_on_server_errors();

  /**
   * Enabled retries on given status code.
   *
   * @param code Status code.
   */
  void retry_on(Response::Status_code code);

  /**
   * Enabled retries on given status code, but only if error message contains
   * the given text.
   *
   * @param code Status code.
   * @param msg Expected contents of the error message.
   */
  void retry_on(Response::Status_code code, const std::string &msg);

  /**
   * Enabled retries on given error code.
   *
   * @param code Error code.
   */
  void retry_on(Error_code code);

 private:
  friend class Retry_strategy_builder;
  class Retry_unknown_errors;
  class Retry_response_error;
  class Retry_connection_error;

  explicit Retry_strategy(std::unique_ptr<IRetry_delay> delay);

  Retry_response_error *m_response_error;
  Retry_connection_error *m_connection_error;
};

/**
 * Builds a retry strategy.
 *
 * The following stop criteria can be defined:
 *
 * - Max number of retries.
 * - Max elapsed time.
 *
 * Retry can be enabled for:
 *
 * - Server error responses (status>=500)
 * - Specific server responses
 *
 * A successful request is the first stop criteria (unless configured to NOT be
 * that way).
 */
class Retry_strategy_builder {
 public:
  /**
   * Retry strategy with constant delay.
   */
  explicit Retry_strategy_builder(uint32_t sleep_time);

  /**
   * Retry strategy with exponential back-off delay.
   *
   * @param base_delay Base delay.
   * @param exponent_grow_factor Exponent grow factor.
   * @param max_wait_between_calls Max time to wait between calls.
   * @param equal_jitter_for_throttling If true, equal jitter is used in case of
   *        TOO_MANY_REQUESTS status code, also enables retries on this code.
   */
  Retry_strategy_builder(uint32_t base_delay, uint32_t exponent_grow_factor,
                         uint32_t max_wait_between_calls,
                         bool equal_jitter_for_throttling);

  Retry_strategy_builder(const Retry_strategy_builder &) = delete;
  Retry_strategy_builder(Retry_strategy_builder &&) = default;

  Retry_strategy_builder &operator=(const Retry_strategy_builder &) = delete;
  Retry_strategy_builder &operator=(Retry_strategy_builder &&) = default;

  ~Retry_strategy_builder();

  /**
   * Sets maximum number of retries.
   *
   * @param retries Maximum number of retries.
   *
   * @returns This instance.
   */
  Retry_strategy_builder &set_max_attempts(uint32_t retries);

  /**
   * Sets maximum time (in seconds) for the whole operation.
   *
   * @param retries Maximum elapsed time in seconds.
   *
   * @returns This instance.
   */
  Retry_strategy_builder &set_max_elapsed_time(uint32_t seconds);

  /**
   * Enabled retries on status codes which represent server errors (5XX).
   *
   * @returns This instance.
   */
  Retry_strategy_builder &retry_on_server_errors();

  /**
   * Enabled retries on given status code.
   *
   * @param code Status code.
   *
   * @returns This instance.
   */
  Retry_strategy_builder &retry_on(Response::Status_code code);

  /**
   * Enabled retries on given status code, but only if error message contains
   * the given text.
   *
   * @param code Status code.
   * @param msg Expected contents of the error message.
   *
   * @returns This instance.
   */
  Retry_strategy_builder &retry_on(Response::Status_code code,
                                   const std::string &msg);

  /**
   * Enabled retries on given error code.
   *
   * @param code Error code.
   *
   * @returns This instance.
   */
  Retry_strategy_builder &retry_on(Error_code code);

  /**
   * Creates the retry strategy.
   *
   * @returns Configured retry strategy.
   *
   * @throws std::logic_error If no stop criteria were defined.
   */
  [[nodiscard]] std::unique_ptr<Retry_strategy> build();

 private:
  class Equal_jitter_for_throttling;

  Retry_strategy_builder();

  std::unique_ptr<Retry_strategy::IRetry_delay> m_delay;

  std::unique_ptr<Retry_strategy::Retry_count> m_retry_count;
  std::unique_ptr<Retry_strategy::Retry_time> m_retry_time;

  std::unique_ptr<Retry_strategy::Retry_response_error> m_response_error;
  std::unique_ptr<Retry_strategy::Retry_connection_error> m_connection_error;
};

std::unique_ptr<Retry_strategy> default_retry_strategy();

std::unique_ptr<IRetry_strategy> retry_terminal_errors_strategy();

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_RETRY_STRATEGY_H_

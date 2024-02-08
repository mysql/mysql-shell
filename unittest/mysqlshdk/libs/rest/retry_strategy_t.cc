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

#include <stdexcept>

#include "unittest/gtest_clean.h"

#include "mysqlshdk/libs/utils/profiling.h"

#include "mysqlshdk/libs/rest/error.h"
#include "mysqlshdk/libs/rest/retry_strategy.h"

namespace mysqlshdk {
namespace rest {
namespace {

TEST(Retry_strategy_test, generic_errors) {
  // One second retry strategy
  Retry_strategy_builder builder{1};

  // Some stop criteria must be defined otherwise we have infinite retries
  EXPECT_THROW_MSG(
      const auto r = builder.build(), std::logic_error,
      "A stop criteria must be defined to avoid infinite retries.");

  // set stop criterion
  const auto retry_strategy = builder.set_max_attempts(2).build();

  // retry strategy is not used in case of non-recoverable error
  retry_strategy->reset();
  EXPECT_FALSE(retry_strategy->should_retry(
      Connection_error{"Couldn't connect", Error_code::COULDNT_CONNECT}));

  // generic errors are retried twice
  retry_strategy->reset();
  EXPECT_TRUE(retry_strategy->should_retry(Unknown_error{}));
  EXPECT_TRUE(retry_strategy->should_retry(Unknown_error{}));
  EXPECT_FALSE(retry_strategy->should_retry(Unknown_error{}));
  EXPECT_FALSE(retry_strategy->should_retry(Unknown_error{}));
}

TEST(Retry_strategy_test, server_errors) {
  // One second retry strategy
  const auto retry_strategy =
      Retry_strategy_builder{1}.set_max_attempts(2).build();

  // Server error codes are not configured to be retriable, so no retry is done
  retry_strategy->reset();
  EXPECT_FALSE(retry_strategy->should_retry(
      Response_error{Response::Status_code::INTERNAL_SERVER_ERROR}));

  // Configure specific error to be retriable
  retry_strategy->retry_on(Response::Status_code::INTERNAL_SERVER_ERROR);

  // retry on configured error
  retry_strategy->reset();
  EXPECT_TRUE(retry_strategy->should_retry(
      Response_error{Response::Status_code::INTERNAL_SERVER_ERROR}));

  // Other server error would not be retriable
  retry_strategy->reset();
  EXPECT_FALSE(retry_strategy->should_retry(
      Response_error{Response::Status_code::SERVICE_UNAVAILABLE}));

  // add handling of a status code with specific error message
  retry_strategy->retry_on(Response::Status_code::INTERNAL_SERVER_ERROR,
                           "Unexpected-error");

  // this error is retriable regardless of error message, because it is also
  // registered using just the status code
  retry_strategy->reset();
  EXPECT_TRUE(retry_strategy->should_retry(
      Response_error{Response::Status_code::INTERNAL_SERVER_ERROR}));

  // this error is retriable regardless of error message, even if it does not
  // match
  retry_strategy->reset();
  EXPECT_TRUE(retry_strategy->should_retry(Response_error{
      Response::Status_code::INTERNAL_SERVER_ERROR, "Some-error"}));

  // error message matches
  retry_strategy->reset();
  EXPECT_TRUE(retry_strategy->should_retry(Response_error{
      Response::Status_code::INTERNAL_SERVER_ERROR, "Unexpected-error"}));

  // add handling of a status code with specific another error message
  retry_strategy->retry_on(Response::Status_code::GATEWAY_TIMEOUT,
                           "Serious-error");

  // no error message -> no retries
  retry_strategy->reset();
  EXPECT_FALSE(retry_strategy->should_retry(
      Response_error{Response::Status_code::GATEWAY_TIMEOUT}));

  // error message does not match -> no retries
  retry_strategy->reset();
  EXPECT_FALSE(retry_strategy->should_retry(Response_error{
      Response::Status_code::GATEWAY_TIMEOUT, "Something-went-wrong"}));

  // error message matches -> retries
  retry_strategy->reset();
  EXPECT_TRUE(retry_strategy->should_retry(
      Response_error{Response::Status_code::GATEWAY_TIMEOUT, "Serious-error"}));

  // All the server errors are now retriable
  retry_strategy->retry_on_server_errors();

  // error is not registered explicitly, but still retried
  retry_strategy->reset();
  EXPECT_TRUE(retry_strategy->should_retry(
      Response_error{Response::Status_code::SERVICE_UNAVAILABLE}));
}

TEST(Retry_strategy_test, connection_errors) {
  // One second retry strategy
  const auto retry_strategy =
      Retry_strategy_builder{1}.set_max_attempts(2).build();

  // connection errors are not configured to be retriable, so no retry is done
  retry_strategy->reset();
  EXPECT_FALSE(retry_strategy->should_retry(
      Connection_error{"Couldn't connect", Error_code::COULDNT_CONNECT}));

  // Configure specific error to be retriable
  retry_strategy->retry_on(Error_code::COULDNT_CONNECT);

  // retry on configured error
  retry_strategy->reset();
  EXPECT_TRUE(retry_strategy->should_retry(
      Connection_error{"Couldn't connect", Error_code::COULDNT_CONNECT}));

  // Other connection errors would not be retriable
  retry_strategy->reset();
  EXPECT_FALSE(retry_strategy->should_retry(
      Connection_error{"Partial file", Error_code::PARTIAL_FILE}));
}

TEST(Retry_strategy_test, max_elapsed_time) {
  // One second retry strategy
  const auto retry_strategy =
      Retry_strategy_builder{1}.set_max_elapsed_time(5).build();

  // All the server errors are now retriable
  retry_strategy->retry_on_server_errors();
  retry_strategy->reset();

  int retries = 0;

  mysqlshdk::utils::Duration d;
  d.start();

  while (retry_strategy->should_retry(
      Response_error{Response::Status_code::GATEWAY_TIMEOUT})) {
    ++retries;
    retry_strategy->wait_for_retry();
  }

  d.finish();

  // Considering each attempt consumes at least 1 (sleep time) the 5th attempt
  // would not take place
  EXPECT_EQ(4, retries);
  // execution time should be greater than 4 seconds (4 retries, 1 second each)
  // and less than maximum allowed time
  EXPECT_GT(d.seconds_elapsed(), 4.0);
  EXPECT_LT(d.seconds_elapsed(), 5.0);
}

TEST(Retry_strategy_test, throttling) {
  // 1 second as base wait time
  // 2 as exponential grow factor
  // 4 seconds as max wait time between calls
  const auto retry_strategy =
      Retry_strategy_builder{1, 2, 4, true}.set_max_elapsed_time(12).build();

  // Time Table
  // MET: Max exponential time topped with the maximum allowed between retries
  // GW: Guaranteed Wait
  // GA: Guaranteed Accumulated
  // JW: Wait caused by Jitter
  // MAA: Max time in attempt
  // MTA: Max Total Accumulated (GA + Max JW)
  // -------------------------------------------|
  // Attempt | MET  | GW | GA | JW  | MAA | MTA |
  // --------|------|----|----|-----|-----|-----|
  // 1       | 2/2  | 1  | 1  | 0-1 | 2   | 2   |
  // 2       | 4/4  | 2  | 3  | 0-2 | 4   | 6   |
  // 3       | 8/4  | 2  | 5  | 0-2 | 4   | 10  |<-- Worse Case Scenario in 12s
  // 4       | 16/4 | 2  | 7  | 0-2 | 4   | 14  |    Picked max jitter/attempt
  // 5       | 32/4 | 2  | 9  | 0-2 | 4   | 18  |
  // 6       | 64/4 | 2  | 11 | 0-2 | 4   | 20  |<-- Best Case Scenario in 12s
  // 6       | 64/4 | 2  | 13 | 0-2 | 4   | 24  |    Picked 0 jitter/attemp
  // --------------------------------------------

  retry_strategy->reset();

  int retries = 0;

  mysqlshdk::utils::Duration d;
  d.start();

  while (retry_strategy->should_retry(
      Response_error{Response::Status_code::TOO_MANY_REQUESTS})) {
    ++retries;
    retry_strategy->wait_for_retry();
  }

  d.finish();

  // execution time should be less than maximum allowed time
  EXPECT_LT(d.seconds_elapsed(), 12.0);
  // Worse case scenario determines minimum attempts in 12 secs
  EXPECT_GE(retries, 3);
  // Best case scenario determines maximum attempts in 12 secs
  EXPECT_LE(retries, 6);
}

}  // namespace
}  // namespace rest
}  // namespace mysqlshdk

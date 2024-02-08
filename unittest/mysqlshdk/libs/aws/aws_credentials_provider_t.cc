/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#include "unittest/gprod_clean.h"

#include "mysqlshdk/libs/aws/aws_credentials_provider.h"

#include "unittest/gtest_clean.h"

#include <chrono>
#include <thread>
#include <vector>

#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_time.h"

namespace mysqlshdk {
namespace aws {
namespace {

TEST(Aws_credentials_provider_test, temporary_credentials) {
  class Temporary_credentials_provider : public Aws_credentials_provider {
   public:
    Temporary_credentials_provider()
        : Aws_credentials_provider(
              {"temporary", "access_key_", "secret_access_key_"}) {}

    int called() const { return m_called; }

   private:
    Credentials fetch_credentials() override {
      Credentials result;

      result.access_key_id = access_key_id() + std::to_string(m_called);
      result.secret_access_key = secret_access_key() + std::to_string(m_called);
      result.expiration = expiration();

      ++m_called;

      return result;
    }

    static std::string expiration() {
      using std::chrono::milliseconds;
      using std::chrono::time_point_cast;

      const auto tp = time_point_cast<milliseconds>(
          Aws_credentials::Clock::now() + milliseconds(100));
      auto result = shcore::time_point_to_rfc3339(tp);
      if (std::string::npos == result.find('.')) {
        // add milliseconds
        auto ms = std::to_string(tp.time_since_epoch().count() %
                                 milliseconds::period::den);
        ms = "." + std::string(3 - ms.length(), '0') + ms;
        result = result.substr(0, 19) + ms + result.substr(19);
      }
      return result;
    }

    int m_called = 0;
  };

  Temporary_credentials_provider provider;

  ASSERT_EQ(true, provider.initialize());

  int thread_count = 4;
  std::vector<int> refreshes;
  refreshes.resize(thread_count);

  const auto fun = [provider = &provider, &refreshes](int id) {
    auto credentials = provider->credentials();

    for (int i = 0; i < 95; ++i) {
      if (credentials->expired()) {
        credentials = provider->credentials();
        const auto r = ++refreshes[id];

        EXPECT_EQ(provider->access_key_id() + std::to_string(r),
                  credentials->access_key_id());
        EXPECT_EQ(provider->secret_access_key() + std::to_string(r),
                  credentials->secret_access_key());
      }

      shcore::sleep_ms(10);
    }
  };

  std::vector<std::thread> threads;
  threads.resize(thread_count);

  for (int id = 0; id < thread_count; ++id) {
    threads[id] = std::thread(fun, id);
  }

  for (auto &t : threads) {
    t.join();
  }

  // initialization + refreshed nine times
  EXPECT_LE(10, provider.called());

  for (const auto r : refreshes) {
    EXPECT_LE(9, r);
  }
}

}  // namespace
}  // namespace aws
}  // namespace mysqlshdk

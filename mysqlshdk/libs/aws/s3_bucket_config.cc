/*
 * Copyright (c) 2022, 2023, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/aws/s3_bucket_config.h"

#include <cstdlib>
#include <memory>
#include <optional>
#include <stdexcept>

#include "mysqlshdk/libs/rest/error.h"
#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"

#include "mysqlshdk/libs/aws/aws_signer.h"
#include "mysqlshdk/libs/aws/config_credentials_provider.h"
#include "mysqlshdk/libs/aws/env_credentials_provider.h"
#include "mysqlshdk/libs/aws/process_credentials_provider.h"
#include "mysqlshdk/libs/aws/s3_bucket.h"

namespace mysqlshdk {
namespace aws {

namespace {

bool is_lower_or_digit(char c) {
  return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

bool valid_as_virtual_path(const std::string &bucket_name) {
  // Bucket name can be used as a virtual path if it is a valid DNS label:
  //  - 0 - 63 characters
  //  - consists of A-Z, a-z, 0-9 and hyphen (-)
  //  - cannot start or end with a hyphen
  // Bucket names have some additional requirements, i.e. only lower-case
  // letters are allowed.
  if (bucket_name.empty() || bucket_name.length() > 63) {
    return false;
  }

  if (!is_lower_or_digit(bucket_name.front()) ||
      !is_lower_or_digit(bucket_name.back())) {
    return false;
  }

  for (const auto &c : bucket_name) {
    if (!is_lower_or_digit(c) && '-' != c) {
      return false;
    }
  }

  return true;
}

std::string config_file_path(const std::string &filename) {
  static const auto s_home_dir = shcore::get_home_dir();
  return shcore::path::join_path(s_home_dir, ".aws", filename);
}

std::optional<const char *> get_env(const char *name) {
  if (const auto value = ::getenv(name); value && value[0]) {
    return value;
  } else {
    return {};
  }
}

/**
 * Retries all unhandled errors up to three times, with one second delay between
 * each retry.
 */
class Aws_retry_strategy : public rest::IRetry_strategy {
 public:
  Aws_retry_strategy() = delete;

  Aws_retry_strategy(const Aws_retry_strategy &) = delete;
  Aws_retry_strategy(Aws_retry_strategy &&) = default;

  Aws_retry_strategy &operator=(const Aws_retry_strategy &) = delete;
  Aws_retry_strategy &operator=(Aws_retry_strategy &&) = default;

  ~Aws_retry_strategy() override = default;

  [[nodiscard]] static std::unique_ptr<Aws_retry_strategy> create(
      std::unique_ptr<IRetry_strategy> strategy) {
    auto delay = std::make_unique<Variable_delay>();
    auto base =
        std::make_unique<Base_strategy>(std::move(strategy), delay.get());

    // this is a private constructor, std::make_unique cannot be used
    std::unique_ptr<Aws_retry_strategy> aws_strategy{
        new Aws_retry_strategy(std::move(delay))};

    aws_strategy->add_condition(std::move(base));
    aws_strategy->add_condition<Filter_status_codes>();
    aws_strategy->add_condition<Accept_all>();

    return aws_strategy;
  }

 private:
  /**
   * Exponential back-off delay with equal jitter.
   */
  class Equal_jitter_delay : public Exponential_backoff_delay {
   public:
    Equal_jitter_delay() = delete;

    /**
     * Initializes an exponential back-off delay.
     *
     * @param base_delay Base delay.
     * @param exponent_grow_factor Exponent grow factor.
     * @param max_wait_between_calls Max time to wait between calls.
     */
    Equal_jitter_delay(uint32_t base_delay, uint32_t exponent_grow_factor,
                       uint32_t max_wait_between_calls)
        : Exponential_backoff_delay(base_delay, exponent_grow_factor,
                                    max_wait_between_calls) {}

    Equal_jitter_delay(const Equal_jitter_delay &) = default;
    Equal_jitter_delay(Equal_jitter_delay &&) = default;

    Equal_jitter_delay &operator=(const Equal_jitter_delay &) = default;
    Equal_jitter_delay &operator=(Equal_jitter_delay &&) = default;

    ~Equal_jitter_delay() override = default;

   private:
    [[nodiscard]] std::chrono::seconds handle(
        const rest::Unknown_error &) override {
      return equal_jitter();
    }

    [[nodiscard]] std::chrono::seconds handle(
        const rest::Response_error &) override {
      return equal_jitter();
    }

    [[nodiscard]] std::chrono::seconds handle(
        const rest::Connection_error &) override {
      return equal_jitter();
    }
  };

  /**
   * Delay which can be changed at will. If retry is attempted within the first
   * 10 minutes since creation, it will use 1 second delay between retries.
   * Afterwards, it's using delay with equal jitter. Delay can be also set to a
   * custom value.
   */
  class Variable_delay : public IRetry_delay {
   public:
    Variable_delay() = default;

    Variable_delay(const Variable_delay &) = delete;
    Variable_delay(Variable_delay &&) = default;

    Variable_delay &operator=(const Variable_delay &) = delete;
    Variable_delay &operator=(Variable_delay &&) = default;

    ~Variable_delay() override = default;

    /**
     * Sets the delay between retries.
     */
    inline void override_delay(std::chrono::seconds delay) noexcept {
      m_custom_delay = delay;
    }

    inline void clear_delay() noexcept { m_custom_delay.reset(); }

   private:
    void reset() override {
      clear_delay();
      m_delay->reset();
    }

    template <typename T>
    [[nodiscard]] std::chrono::seconds delay(const T &error) {
      if (m_custom_delay.has_value()) {
        return *m_custom_delay;
      }

      // use longer delays after 10 minutes have passed
      if (!m_delay_replaced &&
          std::chrono::system_clock::now() - m_creation_time >
              std::chrono::minutes(10)) {
        // 1st delay: 3-6s
        // 2nd delay: 18-36s
        // 3rd and subsequent delays: 40-80s
        m_delay = std::make_unique<Equal_jitter_delay>(1, 6, 80);
        m_delay_replaced = true;
      }

      m_delay->set_next_sleep_time(error);
      return m_delay->next_sleep_time();
    }

    [[nodiscard]] std::chrono::seconds handle(
        const rest::Unknown_error &error) override {
      return delay(error);
    }

    [[nodiscard]] std::chrono::seconds handle(
        const rest::Response_error &error) override {
      return delay(error);
    }

    [[nodiscard]] std::chrono::seconds handle(
        const rest::Connection_error &error) override {
      return delay(error);
    }

    std::chrono::system_clock::time_point m_creation_time =
        std::chrono::system_clock::now();

    // custom delay
    std::optional<std::chrono::seconds> m_custom_delay;
    // start with one second delay between retries
    std::unique_ptr<IRetry_delay> m_delay = std::make_unique<Constant_delay>(1);
    bool m_delay_replaced = false;
  };

  /**
   * Allows up to three retries, with one second delay between retries.
   *
   * First allows the base strategy to retry, if it decides not to, takes over.
   *
   * Controls the delay.
   */
  class Base_strategy : public IRetry_condition {
   public:
    Base_strategy() = delete;

    /**
     * Uses the base strategy, but overrides it decision to always retry.
     *
     * @param strategy Base strategy.
     * @param delay Delay to be used.
     */
    explicit Base_strategy(std::unique_ptr<IRetry_strategy> strategy,
                           Variable_delay *delay)
        : m_base_strategy(std::move(strategy)), m_delay(delay) {}

    Base_strategy(const Base_strategy &) = delete;
    Base_strategy(Base_strategy &&) = default;

    Base_strategy &operator=(const Base_strategy &) = delete;
    Base_strategy &operator=(Base_strategy &&) = default;

    ~Base_strategy() override = default;

   private:
    void reset() override {
      m_base_strategy->reset();
      m_retry_count = 0;
    }

    [[nodiscard]] std::optional<bool> handle(
        const rest::Unknown_error &error) override {
      return handle_retry(error);
    }

    [[nodiscard]] std::optional<bool> handle(
        const rest::Response_error &error) override {
      return handle_retry(error);
    }

    [[nodiscard]] std::optional<bool> handle(
        const rest::Connection_error &error) override {
      return handle_retry(error);
    }

    template <typename T>
    [[nodiscard]] std::optional<bool> handle_retry(const T &error) {
      // always increase the counter, so we don't continue to retry after base
      // strategy finishes its retries
      ++m_retry_count;

      std::optional<bool> retry;

      if (m_base_strategy->should_retry(error)) {
        // use base strategy
        retry = true;
        m_delay->override_delay(m_base_strategy->next_sleep_time());
      } else {
        // use our strategy
        if (m_retry_count > k_max_retries) {
          // we've reached the maximum number of retries
          retry = false;
        }
        // else, decision will be made by another retry condition

        m_delay->clear_delay();
      }

      // reset the waiting time
      m_delay->set_next_sleep_time(error);

      return retry;
    }

    // three retries maximum
    static constexpr uint32_t k_max_retries{3};

    std::unique_ptr<IRetry_strategy> m_base_strategy;
    Variable_delay *m_delay;
    uint32_t m_retry_count = 0;
  };

  /**
   * Not all status codes need to be retried, i.e. 2xx, or 404 (used to detect
   * non-existent files).
   */
  class Filter_status_codes : public Ignore_all {
   public:
    Filter_status_codes() = default;

    Filter_status_codes(const Filter_status_codes &) = default;
    Filter_status_codes(Filter_status_codes &&) = default;

    Filter_status_codes &operator=(const Filter_status_codes &) = default;
    Filter_status_codes &operator=(Filter_status_codes &&) = default;

    ~Filter_status_codes() override = default;

   private:
    [[nodiscard]] std::optional<bool> handle(
        const rest::Response_error &error) override {
      const auto code = error.status_code();

      // don't retry 1xx, 2xx, 3xx
      if (code < rest::Response::Status_code::BAD_REQUEST) {
        return false;
      }

      // don't retry 404
      if (rest::Response::Status_code::NOT_FOUND == code) {
        return false;
      }

      return {};
    }
  };

  explicit Aws_retry_strategy(std::unique_ptr<Variable_delay> delay)
      : IRetry_strategy(std::move(delay)) {}
};

}  // namespace

S3_bucket_config::S3_bucket_config(const S3_bucket_options &options)
    : Bucket_config(options),
      m_credentials_file(options.m_credentials_file),
      m_region(options.m_region),
      m_endpoint(options.m_endpoint_override) {
  setup_profile_name();

  setup_credentials_file();
  load_profile(&m_profile_from_credentials_file);

  setup_config_file();
  load_profile(&m_profile_from_config_file);

  setup_region_name();

  setup_endpoint_uri();

  setup_credentials_provider();
}

std::unique_ptr<rest::Signer> S3_bucket_config::signer() const {
  return std::make_unique<Aws_signer>(*this);
}

std::unique_ptr<rest::IRetry_strategy> S3_bucket_config::retry_strategy()
    const {
  return Aws_retry_strategy::create(
      Signed_rest_service_config::retry_strategy());
}

std::unique_ptr<storage::backend::object_storage::Container>
S3_bucket_config::container() const {
  return s3_bucket();
}

std::unique_ptr<S3_bucket> S3_bucket_config::s3_bucket() const {
  return std::make_unique<S3_bucket>(shared_ptr<S3_bucket_config>());
}

const std::string &S3_bucket_config::hash() const {
  if (m_hash.empty()) {
    m_hash.reserve(512);

    m_hash += m_label;
    m_hash += '-';
    m_hash += m_endpoint;
    m_hash += '-';
    m_hash += m_container_name;
    m_hash += '-';
    m_hash += m_config_file;
    m_hash += '-';
    m_hash += m_config_profile;
    m_hash += '-';
    m_hash += m_credentials_file;
    m_hash += '-';
    m_hash += m_credentials_provider->name();
    m_hash += '-';
    m_hash += m_credentials_provider->credentials()->access_key_id();
    m_hash += '-';
    m_hash += std::to_string(m_credentials_provider->credentials()
                                 ->expiration()
                                 .time_since_epoch()
                                 .count());
    m_hash += '-';
    m_hash += m_region;
  }

  return m_hash;
}

std::string S3_bucket_config::describe_self() const {
  return "AWS S3 bucket=" + m_container_name;
}

void S3_bucket_config::load_profile(
    std::optional<Aws_config_file::Profile> *target) {
  const auto is_config_file = target == &m_profile_from_config_file;
  const auto &path = is_config_file ? m_config_file : m_credentials_file;
  const auto context = is_config_file ? "config" : "credentials";
  Aws_config_file config{path};

  if (!config.load()) {
    log_warning("Could not open the %s file at '%s': %s.", context,
                path.c_str(), shcore::errno_to_string(errno).c_str());
    return;
  }

  const auto profile = config.get_profile(m_config_profile);

  if (!profile) {
    log_warning("The %s file at '%s' does not contain a profile named: '%s'.",
                context, path.c_str(), m_config_profile.c_str());
    return;
  }

  *target = *profile;
}

void S3_bucket_config::use_path_style_access() { m_path_style_access = true; }

void S3_bucket_config::setup_profile_name() {
  if (!m_config_profile.empty()) {
    m_explicit_profile = true;
    return;
  }

  for (const auto name : {"AWS_PROFILE", "AWS_DEFAULT_PROFILE"}) {
    if (const auto value = get_env(name)) {
      m_config_profile = *value;
      return;
    }
  }

  m_config_profile = "default";
}

void S3_bucket_config::setup_credentials_file() {
  setup_config_file(&m_credentials_file);
}

void S3_bucket_config::setup_config_file() {
  setup_config_file(&m_config_file);
}

void S3_bucket_config::setup_config_file(std::string *target) {
  if (!target->empty()) {
    return;
  }

  const auto is_config_file = target == &m_config_file;

  if (const auto value = get_env(
          is_config_file ? "AWS_CONFIG_FILE" : "AWS_SHARED_CREDENTIALS_FILE")) {
    *target = *value;
    return;
  }

  *target = config_file_path(is_config_file ? "config" : "credentials");
}

void S3_bucket_config::setup_region_name() {
  if (!m_region.empty()) {
    return;
  }

  for (const auto name : {"AWS_REGION", "AWS_DEFAULT_REGION"}) {
    if (const auto value = get_env(name)) {
      m_region = *value;
      return;
    }
  }

  if (m_profile_from_config_file.has_value()) {
    const auto region = m_profile_from_config_file->settings.find("region");

    if (m_profile_from_config_file->settings.end() != region &&
        !region->second.empty()) {
      m_region = region->second;
      return;
    }
  }

  m_region = "us-east-1";
}

void S3_bucket_config::setup_endpoint_uri() {
  while (!m_endpoint.empty() && '/' == m_endpoint.back()) {
    m_endpoint.pop_back();
  }

  if (!m_endpoint.empty()) {
    m_host = storage::utils::strip_scheme(m_endpoint);
    // if endpoint is overridden, path-style access is used
    use_path_style_access();
  } else {
    // if bucket can be used as a virtual path, the following URI is used:
    // <bucket>.s3.<region>.amazonaws.com
    if (valid_as_virtual_path(m_container_name)) {
      m_host = m_container_name + ".";
    } else {
      use_path_style_access();
    }

    m_host += "s3." + m_region + ".amazonaws.com";
    m_endpoint = "https://" + m_host;
  }
}

void S3_bucket_config::setup_credentials_provider() {
  std::vector<std::unique_ptr<Aws_credentials_provider>> providers;

  if (m_explicit_profile) {
    // profile was set by an option, don't use credentials from environment
    // variables, see https://github.com/aws/aws-cli/issues/113
    log_info(
        "The environment variables are not going to be used to fetch AWS "
        "credentials, because the '%s' option is set.",
        S3_bucket_options::profile_option());
  } else {
    providers.emplace_back(std::make_unique<Env_credentials_provider>());
  }

  if (m_profile_from_credentials_file.has_value()) {
    providers.emplace_back(std::make_unique<Config_credentials_provider>(
        m_credentials_file, "credentials file",
        &*m_profile_from_credentials_file));
  }

  if (m_profile_from_config_file.has_value()) {
    const auto &profile = *m_profile_from_config_file;

    if (Process_credentials_provider::available(profile)) {
      providers.emplace_back(std::make_unique<Process_credentials_provider>(
          m_config_file, &profile));
    }

    providers.emplace_back(std::make_unique<Config_credentials_provider>(
        m_config_file, "config file", &profile));
  }

  if (providers.empty()) {
    throw std::runtime_error(
        "Could not select the AWS credentials provider, please see log for "
        "more details");
  }

  for (auto &provider : providers) {
    if (provider->initialize()) {
      m_credentials_provider = std::move(provider);
      break;
    }
  }

  if (!m_credentials_provider) {
    throw std::runtime_error(
        "The AWS access and secret keys were not found in: " +
        shcore::str_join(providers, ", ",
                         [](const auto &p) { return p->name(); }));
  }
}

}  // namespace aws
}  // namespace mysqlshdk

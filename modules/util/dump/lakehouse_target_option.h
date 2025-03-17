/*
 * Copyright (c) 2025, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_LAKEHOUSE_TARGET_OPTION_H_
#define MODULES_UTIL_DUMP_LAKEHOUSE_TARGET_OPTION_H_

#include <memory>
#include <string>

#include "mysqlshdk/include/scripting/types/option_pack.h"

#include "modules/util/common/common_options.h"

namespace mysqlsh {
namespace dump {

class Lakehouse_target_option final {
 public:
  Lakehouse_target_option() = default;

  Lakehouse_target_option(const Lakehouse_target_option &) = default;
  Lakehouse_target_option(Lakehouse_target_option &&) = default;

  Lakehouse_target_option &operator=(const Lakehouse_target_option &) = default;
  Lakehouse_target_option &operator=(Lakehouse_target_option &&) = default;

  ~Lakehouse_target_option() = default;

  static const shcore::Option_pack_def<Lakehouse_target_option> &options();

  /**
   * Provides the URL of the output directory.
   */
  const std::string &output_url() const noexcept {
    return m_storage_options.url();
  }

  /**
   * Provides handle to the output directory.
   *
   * @return Directory handle.
   */
  std::unique_ptr<mysqlshdk::storage::IDirectory> output() const;

  /**
   * Provides a description of the output URL.
   */
  std::string describe_output_url() const;

  /**
   * Provides the type of the output directory's storage.
   */
  mysqlsh::common::Storage_options::Storage_type storage_type() const noexcept {
    return m_storage_options.storage_type();
  }

  /**
   * Provides the config of the output directory's storage.
   */
  mysqlshdk::storage::Config_ptr storage_config() const noexcept {
    return m_storage_options.storage_config();
  }

 private:
  class Lakehouse_target : public mysqlsh::common::Common_options {
   public:
    Lakehouse_target();

    Lakehouse_target(const Lakehouse_target &) = default;
    Lakehouse_target(Lakehouse_target &&) = default;

    Lakehouse_target &operator=(const Lakehouse_target &) = default;
    Lakehouse_target &operator=(Lakehouse_target &&) = default;

    ~Lakehouse_target() override = default;

    using Common_options::storage_config;
    using Common_options::storage_type;
  };

  void on_unpacked_options();

  Lakehouse_target m_storage_options;
  std::string m_url;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_LAKEHOUSE_TARGET_OPTION_H_

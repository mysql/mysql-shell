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

#include "modules/util/dump/dump_options.h"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/strformat.h"

namespace mysqlsh {
namespace dump {

Dump_options::Dump_options(const std::string &output_url)
    : m_output_url(output_url),
      m_show_progress(isatty(fileno(stdout)) ? true : false) {}

void Dump_options::set_options(const shcore::Dictionary_t &options) {
  m_options = options;

  shcore::Option_unpacker unpacker{options};
  std::string rate;
  mysqlshdk::db::nullable<std::string> compression;

  unpacker.optional("maxRate", &rate)
      .optional("showProgress", &m_show_progress)
      .optional("compression", &compression)
      .optional("defaultCharacterSet", &m_character_set);

  m_oci_options.unpack(&unpacker);

  unpack_options(&unpacker);

  unpacker.end();

  if (!rate.empty()) {
    m_max_rate = mysqlshdk::utils::expand_to_bytes(rate);
  }

  if (compression) {
    if (compression->empty()) {
      throw std::invalid_argument(
          "The option 'compression' cannot be set to an empty string.");
    }

    m_compression = mysqlshdk::storage::to_compression(*compression);
  }

  m_oci_options.check_option_values();
}

void Dump_options::set_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  m_session = session;

  on_set_session(session);
}

void Dump_options::validate() const {
  m_dialect.validate();

  validate_options();
}

}  // namespace dump
}  // namespace mysqlsh

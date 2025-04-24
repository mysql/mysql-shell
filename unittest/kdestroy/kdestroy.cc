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

extern "C" {
#include <com_err.h>
}

#include <krb5/krb5.h>

namespace {

class Context final {
 public:
  Context() = delete;

  explicit Context(krb5_context context) : m_context(context) {}

  Context(const Context &) = delete;
  Context(Context &&) = delete;

  Context &operator=(const Context &) = delete;
  Context &operator=(Context &&) = delete;

  ~Context() { krb5_free_context(m_context); }

 private:
  krb5_context m_context;
};

}  // namespace

int main() {
  krb5_error_code code;

  const auto report_error = [&code](const char *msg) {
    com_err("kdestroy", code, "%s", msg);
    return 1;
  };

  krb5_context context;

  if (code = krb5_init_context(&context); code) {
    return report_error("Failed to initialize krb5 context");
  }

  Context deleter{context};
  krb5_ccache cache = nullptr;

  if (code = krb5_cc_default(context, &cache); code) {
    return report_error("Failed to resolve default cache name");
  }

  if (code = krb5_cc_destroy(context, cache); code && KRB5_FCC_NOFILE != code) {
    return report_error("Failed to destroy default cache");
  }

  return 0;
}

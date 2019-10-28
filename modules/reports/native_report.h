/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_REPORTS_NATIVE_REPORT_H_
#define MODULES_REPORTS_NATIVE_REPORT_H_

#include <memory>

#include "modules/mod_shell_reports.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/shellcore/base_session.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace reports {

class Native_report {
 public:
  virtual ~Native_report() = default;

  template <typename T,
            typename std::enable_if<std::is_base_of<Native_report, T>::value,
                                    int>::type = 0>
  static void register_report(Shell_reports *reports) {
    reports->register_report(Registrar<T>::create());
  }

  shcore::Dictionary_t report(const std::shared_ptr<ShellBaseSession> &session,
                              const shcore::Array_t &argv,
                              const shcore::Dictionary_t &options);

 protected:
  virtual void parse(const shcore::Array_t &argv,
                     const shcore::Dictionary_t &options) = 0;

  virtual shcore::Array_t execute() const = 0;

  std::shared_ptr<mysqlshdk::db::ISession> m_session;

 private:
  template <typename T>
  struct Registrar {
   public:
    static std::unique_ptr<Report> create() {
      auto report = std::make_unique<Report>(
          T::Config::name(), T::Config::type(),
          [](const std::shared_ptr<ShellBaseSession> &session,
             const shcore::Array_t &argv, const shcore::Dictionary_t &options) {
            return T{}.report(session, argv, options);
          });

      report->set_brief(T::Config::brief());
      const auto r = report.get();
      set_details(r);
      set_options(r);
      set_argc(r);
      set_formatter(r);
      set_examples(r);

      return report;
    }

   private:
    template <typename...>
    using void_t = void;

    template <class U, template <typename> class, typename = void_t<>>
    struct detect : std::false_type {};

    template <class U, template <typename> class Op>
    struct detect<U, Op, void_t<Op<U>>> : std::true_type {};

#define ADD_OPTIONAL_METHOD(name)                      \
  template <typename U>                                \
  using name##_t = decltype(U::Config::name());        \
  template <typename U>                                \
  using has_##name = detect<U, name##_t>;              \
  template <typename U>                                \
  static void set_##name(Report *r, std::true_type) {  \
    return r->set_##name(U::Config::name());           \
  }                                                    \
  template <typename U>                                \
  static void set_##name(Report *, std::false_type) {} \
  static void set_##name(Report *r) { set_##name<T>(r, has_##name<T>{}); }

    ADD_OPTIONAL_METHOD(details)
    ADD_OPTIONAL_METHOD(options)
    ADD_OPTIONAL_METHOD(argc)
    ADD_OPTIONAL_METHOD(formatter)
    ADD_OPTIONAL_METHOD(examples)

#undef ADD_OPTIONAL_METHOD
  };
};

}  // namespace reports
}  // namespace mysqlsh

#endif  // MODULES_REPORTS_NATIVE_REPORT_H_

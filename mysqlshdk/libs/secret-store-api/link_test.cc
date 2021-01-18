/*
 * Copyright (c) 2018, 2021 Oracle and/or its affiliates.
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

#include <iostream>

#include "mysql-secret-store/include/mysql-secret-store/api.h"

namespace mysql {
namespace secret_store {
namespace api {
namespace link {

void test() {
  using std::cout;
  using std::endl;

  auto helper_name = get_available_helpers()[0];
  cout << "name: " << helper_name.get() << endl;
  cout << "path: " << helper_name.path() << endl;
  auto helper_interface = get_helper(helper_name);
  cout << "name: " << helper_interface->name().get() << endl;
  cout << "path: " << helper_interface->name().path() << endl;
  Secret_spec spec{Secret_type::PASSWORD, "user@host:port"};
  cout << "equal: " << (spec == spec) << endl;
  cout << "not equal: " << (spec != spec) << endl;
  cout << "store: " << helper_interface->store(spec, "s3cr3t") << endl;
  std::string secret;
  cout << "get: " << helper_interface->get(spec, &secret) << endl;
  cout << "erase: " << helper_interface->erase(spec) << endl;
  std::vector<Secret_spec> specs;
  cout << "list: " << helper_interface->list(&specs) << endl;
  cout << "last error: " << helper_interface->get_last_error() << endl;
}

}  // namespace link
}  // namespace api
}  // namespace secret_store
}  // namespace mysql

int main(int argc, char *[]) {
  if (123456 == argc) {
    mysql::secret_store::api::link::test();
  }

  return 0;
}

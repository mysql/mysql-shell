/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/oci/oci_regions.h"

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace mysqlshdk {
namespace oci {
namespace {

class Oci_regions_test : public Shell_core_test_wrapper {};

TEST_F(Oci_regions_test, short_region_names) {
  execute("\\py");
  execute("import oci");

  output_handler.wipe_all();
  execute("oci.regions.REGIONS_SHORT_NAMES");

  const auto regions = shcore::Value::parse(output_handler.std_out);
  ASSERT_EQ(shcore::Value_type::Map, regions.get_type());

  for (const auto &s : *regions.as_map()) {
    ASSERT_EQ(shcore::Value_type::String, s.second.get_type());
    const auto full_name = s.second.as_string();

    SCOPED_TRACE(s.first + " -> " + full_name);

    EXPECT_EQ(full_name, regions::from_short_name(s.first));
  }
}

}  // namespace
}  // namespace oci
}  // namespace mysqlshdk

/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "unittest/mysqlshdk/libs/azure/test_utils.h"

#include "unittest/gtest_clean.h"

namespace testing {

std::shared_ptr<Blob_storage_config> get_config(const std::string &container) {
  const auto options = shcore::make_dict(
      Blob_storage_options::container_name_option(), container);

  Blob_storage_options parsed_options;
  Blob_storage_options::options().unpack(options, &parsed_options);

  try {
    return std::make_shared<Blob_storage_config>(parsed_options);
  } catch (...) {
  }
  return {};
}

void create_container(const std::string &name) {
  auto config = get_config(name);

  if (config && config->valid()) {
    Blob_container container(config);
    if (container.exists())
      clean_container(container);
    else
      container.create();
  }
}

void clean_container(Blob_container &container) {
  auto objects = container.list_objects();

  if (!objects.empty()) {
    for (const auto &object : objects) {
      container.delete_object(object.name);
    }

    objects = container.list_objects();
    EXPECT_TRUE(objects.empty());

    auto multiparts = container.list_multipart_uploads();
    for (auto &multipart : multiparts) {
      container.commit_multipart_upload(multipart, {});
      container.delete_object(multipart.name);
    }
  }
}

void delete_container(const std::string &name) {
  auto config = get_config(name);

  if (config && config->valid()) {
    Blob_container container(config);

    container.delete_();
  }
}

}  // namespace testing

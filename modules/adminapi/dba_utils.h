/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_DBA_UTILS_H_
#define MODULES_ADMINAPI_DBA_UTILS_H_

// Public AdminAPI utilities.

#include <memory>
#include <string>

#include "modules/adminapi/common/instance_pool.h"

namespace mysqlsh {
namespace dba {

std::string find_primary_member_uri(const std::shared_ptr<Instance> &instance,
                                    bool xproto,
                                    bool *out_single_primary = nullptr,
                                    Cluster_type *out_type = nullptr);

std::string find_secondary_member_uri(const std::shared_ptr<Instance> &instance,
                                      bool xproto,
                                      bool *out_single_primary = nullptr,
                                      Cluster_type *out_type = nullptr);

/**
 * Get the primary member of a GR group.
 *
 * @param instance session object of a member of the GR group.
 *
 * @throw shcore::Exception if the Group has no quorum.
 * @throw std::logic_error if a primary cannot be found.
 *
 * @return A instance session object to the primary member of the GR group.
 */
std::shared_ptr<mysqlsh::dba::Instance> get_primary_member_from_group(
    const std::shared_ptr<Instance> &instance);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_DBA_UTILS_H_

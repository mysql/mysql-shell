/*
* Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of the
* License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#include <ctime>
#include <random>
#include "utils_uuid.h"
#include "common/uuid/include/uuid_gen.h"

namespace shcore {

struct Init_uuid_gen {
  Init_uuid_gen() {
    // Use a random seed for UUIDs
    std::uniform_int_distribution<> dist(std::numeric_limits<int>::min(),
                                         std::numeric_limits<int>::max());
    std::random_device r;
    std::mt19937 gen(r());

    init_uuid(dist(gen));
  }
};
static Init_uuid_gen __init_uuid;

std::string Id_generator::new_uuid() {
  uuid_type uuid;

  generate_uuid(uuid);

  return hexify(uuid);
}

std::string Id_generator::new_document_id() {
  uuid_type uuid;
  generate_uuid(uuid);
  uuid_type document_id;

  auto src = std::begin(uuid);
  auto tgt = std::begin(document_id);

  // Document UUID has the components reverted
  // Time Low (size 4) is sent to the end
  std::copy(src, src + 4, tgt + 12);

  // Time Mid (size 2) is sent before Time Low
  std::copy(src + 4, src + 6, tgt + 10);

  // Time High and Version (size 2) is sent before Time Mid
  std::copy(src + 6, src + 8, tgt + 8);

  // Clock Seq is sent before Time High
  std::copy(src + 8, src + 10, tgt + 6);

  // Node ID is sent to the beginning
  std::copy(src + 10, std::end(uuid), tgt);

  return hexify(document_id);
}


}
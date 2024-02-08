/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_COPY_COPY_OPERATION_H_
#define MODULES_UTIL_COPY_COPY_OPERATION_H_

#include <memory>
#include <string>
#include <utility>

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/storage/backend/in_memory/virtual_config.h"
#include "mysqlshdk/libs/storage/config.h"
#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/version.h"

#include "modules/mod_utils.h"
#include "modules/util/dump/ddl_dumper.h"
#include "modules/util/load/dump_loader.h"

namespace mysqlsh {
namespace copy {

std::pair<std::shared_ptr<mysqlshdk::storage::in_memory::Virtual_config>,
          std::unique_ptr<mysqlshdk::storage::IDirectory>>
setup_virtual_storage();

void copy(dump::Ddl_dumper *dumper, Dump_loader *loader,
          const std::shared_ptr<mysqlshdk::storage::in_memory::Virtual_config>
              &storage);

template <class Dumper, class Options>
void copy(const mysqlshdk::db::Connection_options &connection_options,
          Options *copy_options) {
  std::shared_ptr<mysqlshdk::db::ISession> load_session;

  try {
    load_session = establish_session(connection_options,
                                     current_shell_options()->get().wizards);
  } catch (const mysqlshdk::db::Error &e) {
    throw std::invalid_argument("Could not connect to the target instance: " +
                                e.format());
  }

  auto [storage, output] = setup_virtual_storage();

  copy_options->dump_options()->set_storage_config(storage);
  copy_options->dump_options()->set_output_url(output->full_path().real());

  const auto version = mysqlshdk::utils::Version(
      load_session->query("SELECT @@version")->fetch_one()->get_string(0));
  auto is_mds = version.is_mds();
  DBUG_EXECUTE_IF("copy_utils_force_mds", { is_mds = true; });

  // if target is MDS, then we want to validate the version, so we won't copy
  // to an unsupported version
  copy_options->dump_options()->set_target_version(version, is_mds);

  // enable MDS checks if target is an MDS instance
  if (is_mds) {
    copy_options->dump_options()->enable_mds_compatibility_checks();
  }

  copy_options->dump_options()->validate();

  copy_options->load_options()->set_storage_config(storage);
  copy_options->load_options()->set_url(output->full_path().real());
  copy_options->load_options()->set_session(load_session);
  copy_options->load_options()->validate();

  const auto host_info = [](const auto &session) {
    const auto instance = mysqlshdk::mysql::Instance(session);
    return instance.get_canonical_address();
  };

  const auto src = host_info(copy_options->dump_options()->session());
  const auto tgt = host_info(copy_options->load_options()->base_session());

  if (src == tgt) {
    throw std::invalid_argument(
        "The target instance is the same as the source instance");
  }

  current_console()->print_info(
      copy_options->load_options()->target_import_info(
          "Copying", ", source: " + src + ", target: " + tgt));

  // Both dumper and loader initialize their scoped consoles in constructors,
  // this order of initialization means that dumper has a Console_with_progress,
  // which uses the global console, loader has a Console_with_progress which
  // uses dumper's scoped console. Loader's console becomes the global console
  // for the main thread and it's also used by all subsequent threads. This
  // chain of consoles allows to synchronize console output with progress
  // threads of both dumper and loader.
  Dumper dumper{*copy_options->dump_options()};
  Dump_loader loader{*copy_options->load_options()};

  copy(&dumper, &loader, storage);
}

}  // namespace copy
}  // namespace mysqlsh

#endif  // MODULES_UTIL_COPY_COPY_OPERATION_H_

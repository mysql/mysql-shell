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

#include "modules/adminapi/common/metadata_backup_handler.h"

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/metadata-model_definitions.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/debug.h"

namespace mysqlsh {
namespace dba {
namespace metadata {

namespace {
/**
 * This class is in charge of performing a simple backup of the metadata schema
 * assuming the only thing needed is creating of the backup schema and copying
 * the data there.
 *
 * It should serve as the backup process on a metadata that does not have
 * additional requirements, custom backup processes should be handled as
 * subclasses of this one and the proper usage should be added to the
 * generate_backup_handler factory function.
 */
class Base_backup_handler {
 public:
  Base_backup_handler(const Version &version, const std::string &src_schema,
                      const std::string &tgt_schema,
                      const std::shared_ptr<Instance> &instance,
                      const std::string &context, bool keep_upgrading_version)
      : m_version(version),
        m_context(context),
        m_source_schema(src_schema),
        m_target_schema(tgt_schema),
        m_instance(instance),
        m_keep_schema_version(keep_upgrading_version) {}
  virtual ~Base_backup_handler() = default;

  /**
   * Executes the metadata backup as self contained operation.
   */
  void execute() {
    try {
      prepare();
      copy_data();
      finish();
    } catch (const std::exception &err) {
      rollback();
      throw;
    }
  }

 protected:
  virtual std::string prepare_schema() {
    // When keeping the schema version we need to ensure to NOT:
    // - Recreate the schema
    // - Recreate schema_version

    std::string script = k_metadata_schema_scripts.at(m_version.get_base());

    if (m_keep_schema_version) {
      script = shcore::str_replace(script,
                                   "DROP VIEW IF EXISTS schema_version;", "");

      std::string create_schema_version_stmt = shcore::str_format(
          "CREATE SQL SECURITY INVOKER VIEW schema_version (major, minor, "
          "patch) AS SELECT %d, %d, %d;",
          m_version.get_major(), m_version.get_minor(), m_version.get_patch());

      script = shcore::str_replace(script, create_schema_version_stmt, "");
    }

    if (m_target_schema != "mysql_innodb_cluster_metadata") {
      script = shcore::str_replace(script, "mysql_innodb_cluster_metadata",
                                   m_target_schema);
    }

    return script;
  }

  /**
   * Deploys the target schema on the backup process using the schema that
   * corresponds to the required version.
   *
   * If the it is required to replace the schema_version with the upgrading
   * version it will be done as well.
   */
  virtual void prepare() {
    if (m_keep_schema_version) {
      log_debug("Dropping tables from metadata schema %s.",
                m_target_schema.c_str());
      auto tables =
          mysqlshdk::mysql::schema::get_tables(*m_instance, m_target_schema);

      m_instance->execute("SET FOREIGN_KEY_CHECKS=0");

      for (const auto &table : tables) {
        m_instance->executef("DROP TABLE IF EXISTS !.!", m_target_schema,
                             table);
      }

      log_debug("Dropping views from metadata schema %s.",
                m_target_schema.c_str());
      auto views =
          mysqlshdk::mysql::schema::get_views(*m_instance, m_target_schema);

      for (const auto &view : views) {
        if (view != "schema_version") {
          m_instance->executef("DROP VIEW IF EXISTS !.!", m_target_schema,
                               view);
        }
      }

      m_instance->execute("SET FOREIGN_KEY_CHECKS=1");
    } else {
      log_debug("Dropping existing metadata schema: %s.",
                m_target_schema.c_str());
      m_instance->executef("DROP SCHEMA IF EXISTS !", m_target_schema);
    }

    execute_script(m_instance, prepare_schema(), "While " + m_context);
  };

  /**
   * Copies the table data from the source to the target schema.
   */
  virtual void copy_data() {
    mysqlshdk::mysql::copy_data(*m_instance, m_source_schema, m_target_schema);
  };

  /**
   * Placeholder function to do any additional operations once the backup is
   * done.
   */
  virtual void finish(){};

  /**
   * Executes the required actions in case the backup process fails.
   */
  virtual void rollback() {
    m_instance->executef("DROP SCHEMA IF EXISTS !", m_target_schema);
  };

  const Version &m_version;
  const std::string &m_context;
  const std::string &m_source_schema;
  const std::string &m_target_schema;
  const std::shared_ptr<Instance> &m_instance;
  bool m_keep_schema_version;
};

/**
 * Custom backup handler for schema version 1.0.*.
 *
 * The metadata 1.0.1 contains a foreign key on the instances table that uses
 * SET NULL which is not allowed by Group Replication when
 * group_replication_enforce_update_everywhere_checks is ON, this option is
 * diabled on single-primary deployments but enabled in multi-primary
 * deployments.
 *
 * This class will handle removing such constraint to execute the backup and put
 * it back whenever the backup is complete or if the data copy failed, the
 * objective is to let the source schema in the original state.
 */
class MD_1_0_backup_handler : public Base_backup_handler {
 public:
  MD_1_0_backup_handler(const Version &version, const std::string &src_schema,
                        const std::string &tgt_schema,
                        const std::shared_ptr<Instance> &instance,
                        const std::string &context, bool keep_upgrading_version)
      : Base_backup_handler(version, src_schema, tgt_schema, instance, context,
                            keep_upgrading_version) {}
  virtual ~MD_1_0_backup_handler() = default;

 protected:
  /**
   * Retrieves the name of the foreign key from instances to replicaset in case
   * it exists, otherwise returns an empty string.
   */
  std::string get_instance_to_replicasets_fk_name(const std::string &schema) {
    std::string fk_name;
    auto result = m_instance->queryf(
        "SELECT constraint_name FROM information_schema.key_column_usage WHERE "
        "constraint_schema = ? AND table_name = 'instances' AND column_name = "
        "'replicaset_id'",
        schema);
    auto row = result->fetch_one();
    if (row) {
      fk_name = row->get_string(0);
    }
    return fk_name;
  }

  void remove_instance_to_replicasets_fk(const std::string &schema) {
    auto fk_name = get_instance_to_replicasets_fk_name(schema);
    if (!fk_name.empty()) {
      m_instance->executef("ALTER TABLE !.instances DROP FOREIGN KEY !", schema,
                           fk_name);
    }
  };

  void install_instance_to_replicasets_fk(const std::string &schema) {
    auto fk_name = get_instance_to_replicasets_fk_name(schema);
    if (fk_name.empty()) {
      m_instance->executef(
          "ALTER TABLE !.instances ADD FOREIGN KEY (`replicaset_id`) "
          "REFERENCES "
          "`replicasets` (`replicaset_id`) ON DELETE SET NULL",
          schema);
    }
  };

  std::string prepare_schema() override {
    // When keeping the schema version we need to ensure to NOT:
    // - Recreate the schema
    // - Recreate schema_version

    std::string script = k_metadata_schema_scripts.at(m_version.get_base());

    if (m_keep_schema_version) {
      script = shcore::str_replace(
          script, "CREATE DATABASE mysql_innodb_cluster_metadata;",
          "CREATE DATABASE IF NOT EXISTS mysql_innodb_cluster_metadata;");

      script = shcore::str_replace(
          script,
          "CREATE VIEW schema_version (major, minor, patch) AS SELECT 1, 0, 1;",
          "");
    }

    if (m_target_schema != "mysql_innodb_cluster_metadata") {
      script = shcore::str_replace(script, "mysql_innodb_cluster_metadata",
                                   m_target_schema);
    }

    return script;
  }

  void prepare() override {
    Base_backup_handler::prepare();
    // Removes the problematic constraint in both the source and target
    // schemas.
    remove_instance_to_replicasets_fk(m_target_schema);
    remove_instance_to_replicasets_fk(m_source_schema);
  }

  void finish() override {
    // Once the backup is complete restores the constraint in both the source
    // and target schemas
    install_instance_to_replicasets_fk(m_target_schema);
    install_instance_to_replicasets_fk(m_source_schema);
    Base_backup_handler::finish();
  };

  void rollback() override {
    // If the backup failed but the constraint was removed from the source then
    // it is restored.
    install_instance_to_replicasets_fk(m_source_schema);
    Base_backup_handler::rollback();
  };
};

/**
 * Instantiates the backup handler class that corresponds to the required
 * metadata schema version.
 */
std::unique_ptr<Base_backup_handler> generate_backup_handler(
    const Version &version, const std::string &src_schema,
    const std::string &tgt_schema, const std::shared_ptr<Instance> &instance,
    const std::string &context, bool keep_upgrading_version) {
  bool found = k_metadata_schema_scripts.find(version.get_base()) !=
               k_metadata_schema_scripts.end();

  bool emulating = false;
  DBUG_EXECUTE_IF("dba_EMULATE_UNEXISTING_MD", {
    found = true;
    emulating = true;
  });

  if (found) {
    if (emulating || version == Version(1, 0, 1)) {
      return std::make_unique<MD_1_0_backup_handler>(
          version, src_schema, tgt_schema, instance, context,
          keep_upgrading_version);
    } else {
      return std::make_unique<Base_backup_handler>(
          version, src_schema, tgt_schema, instance, context,
          keep_upgrading_version);
    }
  } else {
    throw shcore::Exception::runtime_error(
        "Failed " + context + ", metadata version " + version.get_base() +
        " is not supported.");
  }
}

}  // namespace

void backup(const Version &version, const std::string &src_schema,
            const std::string &tgt_schema,
            const std::shared_ptr<Instance> &instance,
            const std::string &context, bool keep_upgrading_version) {
  std::unique_ptr<Base_backup_handler> backup;

  DBUG_EXECUTE_IF("dba_EMULATE_UNEXISTING_MD", {
    backup = generate_backup_handler(Version(1, 0, 1), src_schema, tgt_schema,
                                     instance, context, keep_upgrading_version);
  });

  if (!backup) {
    backup = generate_backup_handler(version, src_schema, tgt_schema, instance,
                                     context, keep_upgrading_version);
  }

  backup->execute();
}
}  // namespace metadata
}  // namespace dba
}  // namespace mysqlsh

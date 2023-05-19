function get_1_0_1_snapshot_file() {
  if (__version_num > 800000)
    return __test_data_path + '/sql/' + 'metadata-1.0.1-8.0.17-snapshot.sql';
  else
    return __test_data_path + '/sql/' + 'metadata-1.0.1-5.7.27-snapshot.sql';
}

function get_1_0_1_template_file() {
  if (__version_num > 800000)
    return __test_data_path + '/sql/' + 'metadata-1.0.1-8.0.17-template.sql';
  else
    return __test_data_path + '/sql/' + 'metadata-1.0.1-5.7.27-template.sql';
}


function set_metadata_version(major, minor, patch, clear_data,
                              replication_group_name) {
  session.runSql("DROP VIEW mysql_innodb_cluster_metadata.schema_version");
  session.runSql("CREATE VIEW mysql_innodb_cluster_metadata.schema_version (major, minor, patch) AS SELECT ?, ?, ?",
      [ major, minor, patch ]);
}

function update_var(file, variable, val) {
  var re = RegExp(variable, "g")
  return file.replace(re, val);
}

function create_instance_record(id, hostport, uuid, server_id) {
  if (__version_num < 80027) {
  return `(${id},1,1,'${uuid}','${hostport}','HA',NULL,'{\"mysqlX\":\"${hostport}0\",\"grLocal\":\"${hostport}1\",\"mysqlClassic\":\"${hostport}\"}','{\"server_id\":${server_id},\"recoveryAccountHost\":\"%\",\"recoveryAccountUser\":\"mysql_innodb_cluster_15662\"}',NULL,NULL)`;
  } else {
    // With 8.0.27, the default communications stack is MySQL, so grLocal will
    // use the classic port and not the generated GR port
    return `(${id},1,1,'${uuid}','${hostport}','HA',NULL,'{\"mysqlX\":\"${hostport}0\",\"grLocal\":\"${hostport}\",\"mysqlClassic\":\"${hostport}\"}','{\"server_id\":${server_id},\"recoveryAccountHost\":\"%\",\"recoveryAccountUser\":\"mysql_innodb_cluster_15662\"}',NULL,NULL)`;
  }
}

function prepare_1_0_1_metadata_from_template(file, replication_group_id, instances, topology = "pm", schema = "mysql_innodb_cluster_metadata") {
 var ports = [__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3];

  var script = os.load_text_file(get_1_0_1_template_file());
  var s1 = update_var(script, "__schema__", schema)
  var s1 = update_var(script, "__replication_group_uuid__", replication_group_id)
  var s2 = update_var(s1, "__hostname__", hostname)
  var s3 = update_var(s2, "__real_hostname__", real_hostname)
  var s4 = update_var(s3, "__report_host__", real_hostname)
  var s5 = update_var(s4, "__topology__", topology)

  instance_records = []
  for(index in instances) {
    hostport = `${hostname}:${ports[index]}`;
    instance_records.push(create_instance_record(parseInt(index) + 1, hostport, instances[index][0], instances[index][1]))
  }

  var s6 = update_var(s5, "__instances__", instance_records.join(","));

  // In multi primary mode the file will not be importable because of the constraints
  // So we remove them and add them at the end
  if (topology == "mm") {
    s6 = s6.replace("CONSTRAINT `instances_ibfk_1` FOREIGN KEY (`host_id`) REFERENCES `hosts` (`host_id`),",
                    "CONSTRAINT `instances_ibfk_1` FOREIGN KEY (`host_id`) REFERENCES `hosts` (`host_id`)");
    s6 = s6.replace("CONSTRAINT `instances_ibfk_2` FOREIGN KEY (`replicaset_id`) REFERENCES `replicasets` (`replicaset_id`) ON DELETE SET NULL","");
    s6 = s6.replace("CONSTRAINT `instances_ibfk_1` FOREIGN KEY (`host_id`) REFERENCES `hosts` (`host_id`) ON DELETE RESTRICT,",
                    "CONSTRAINT `instances_ibfk_1` FOREIGN KEY (`host_id`) REFERENCES `hosts` (`host_id`) ON DELETE RESTRICT");

    s6 = s6 + "\nALTER TABLE instances ADD FOREIGN KEY (`replicaset_id`) REFERENCES `replicasets` (`replicaset_id`) ON DELETE SET NULL";
  }

  testutil.createFile(file, s6);
}

function backup_metadata(uri, file) {
  testutil.dumpData(uri, file, ["mysql_innodb_cluster_metadata"]);
}

function clear_metadata_1_0_1() {
  session.runSql("SET FOREIGN_KEY_CHECKS=0");
  session.runSql("DELETE FROM mysql_innodb_cluster_metadata.clusters");
  session.runSql("DELETE FROM mysql_innodb_cluster_metadata.replicasets");
  session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances");
  session.runSql("DELETE FROM mysql_innodb_cluster_metadata.routers");
  session.runSql("DELETE FROM mysql_innodb_cluster_metadata.hosts");
  session.runSql("SET FOREIGN_KEY_CHECKS=1");
}

function clear_metadata_2_0_0() {
  session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances");
  session.runSql("DELETE FROM mysql_innodb_cluster_metadata.clusters");
  session.runSql("DELETE FROM mysql_innodb_cluster_metadata.async_cluster_views");
  session.runSql("DELETE FROM mysql_innodb_cluster_metadata.router_rest_accounts");
  session.runSql("DELETE FROM mysql_innodb_cluster_metadata.routers");
}

function load_metadata(uri, file, clear_data, replication_group_name) {
  println("Loading metadata from: " + file);
  session.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
  testutil.importData(uri, file);

  if (clear_data || replication_group_name) {
    var installed = testutil.getInstalledMetadataVersion();
    var version = installed.split('.');

    if (!clear_data) {
      session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET mysql_server_uuid = @@server_uuid");
    }

    if (version[0] == "1") {
      if (clear_data) {
        clear_metadata_1_0_1();
      } else {
        if (replication_group_name) {
          session.runSql(
              "UPDATE mysql_innodb_cluster_metadata.replicasets SET attributes = JSON_REPLACE(attributes, '$.group_replication_group_name','" + replication_group_name + "')");
        }
      }
    } else {
      if (clear_data) {
        clear_metadata_2_0_0();
      } else {
        if (replication_group_name) {
          session.runSql(
              "UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = JSON_REPLACE(attributes, '$.group_replication_group_name','" + replication_group_name + "')");
        }
      }
    }
  }
}

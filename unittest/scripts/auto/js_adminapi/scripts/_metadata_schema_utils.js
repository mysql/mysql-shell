// Internal module functions
function get_2_0_0_template_file() {
  if (__version_num > 80000)
    return __test_data_path + '/sql/' + 'metadata-2.0.0-8.0.26-template.sql';
  else
    return __test_data_path + '/sql/' + 'metadata-2.0.0-5.7.35-template.sql';
}

function update_var(file, variable, val) {
  var re = RegExp(variable, "g")
  return file.replace(re, val);
}

function create_instance_record_2_0_0(id, cluster_id, hostport, uuid, server_id) {
  return `(${id},'${cluster_id}','${hostport}','${uuid}','${hostport}','{\"mysqlX\": \"${hostport}0\", \"grLocal\": \"${hostport}1\", \"mysqlClassic\": \"${hostport}\"}','{\"server_id\": ${server_id}, \"recoveryAccountHost\": \"%\", \"recoveryAccountUser\": \"mysql_innodb_cluster_4094449207\"}',NULL)`;
}

function update_script(script, schema, replication_group_id, topology) {
  var s1 = update_var(script, "__schema__", schema)
  var s2 = update_var(s1, "__replication_group_uuid__", replication_group_id)
  var s3 = update_var(s2, "__hostname__", hostname)
  var s4 = update_var(s3, "__real_hostname__", real_hostname)
  var s5 = update_var(s4, "__report_host__", real_hostname)
  var s6 = update_var(s5, "__topology__", topology)

  return s6;
}

// External module functions
exports.prepare_2_0_0_metadata_from_template = function(file, cluster_id, replication_group_id, instances, topology = "pm", schema = "mysql_innodb_cluster_metadata") {
 var ports = [__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3];


  var script = os.load_text_file(get_2_0_0_template_file());

  var s1 = update_script(script, schema, replication_group_id, topology)
  var s2 = update_var(s1, "__cluster_id__", cluster_id)

  instance_records = []
  for(index in instances) {
    hostport = `${hostname}:${ports[index]}`;
    instance_records.push(create_instance_record_2_0_0(parseInt(index) + 1, cluster_id, hostport, instances[index][0], instances[index][1]))
  }

  var s3 = update_var(s2, "__instances__", instance_records.join(","));

  testutil.createFile(file, s3);
}

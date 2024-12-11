function get_2_1_0_template_file() {
  return __test_data_path + '/sql/' + 'metadata-2.1.0-8.0.40-template.sql';
}

function update_var(file, variable, val) {
  var re = RegExp(variable, "g")
  return file.replace(re, val);
}

function create_instance_record(id, cluster_id, hostport, uuid, server_id) {
  return `(${id},'${cluster_id}','${hostport}','${uuid}','${hostport}','{\"mysqlX\": \"${hostport}0\", \"mysqlClassic\": \"${hostport}\"}','{\"server_id\": ${server_id}}', NULL)`;
}

function create_async_cluster_member_record(id, cluster_id, hostport, uuid) {
  return `('${cluster_id}',2 ,${id}, NULL, 1, '{\"instance.address\": \"${hostport}\", \"instance.mysql_server_uuid\": \"${uuid}\"}')`;
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

function prepare_2_1_0_metadata_from_template(file, cluster_id, instances, schema = "mysql_innodb_cluster_metadata") {
 var ports = [__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3];

  var script = os.loadTextFile(get_2_1_0_template_file());

  var s1 = update_script(script, schema, "", "")
  var s2 = update_var(s1, "__cluster_id__", cluster_id)
  var s3 = update_var(s2, "__primary_id__", instances[0][0])

  instance_records = []
  for(index in instances) {
    hostport = `${hostname}:${ports[index]}`;
    instance_records.push(create_instance_record(parseInt(index) + 1, cluster_id, hostport, instances[index][0], instances[index][1]))
  }

  async_cluster_members_records = []
  for(index in instances) {
    hostport = `${hostname}:${ports[index]}`;
    async_cluster_members_records.push(create_async_cluster_member_record(parseInt(index) + 1, cluster_id, hostport, instances[index][0]))
  }

  var s4 = update_var(s3, "__instances__", instance_records.join(","));

  var s5 = update_var(s4, "__async_cluster_members__", async_cluster_members_records.join(","));

  testutil.createFile(file, s5);
}

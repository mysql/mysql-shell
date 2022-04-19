function check_gr_settings(cluster, endpoints, commstack, localaddress, allowlist) {
  var session_primary = mysql.getSession("root:root@"+cluster.status()["defaultReplicaSet"]["primary"]);

  var group_seeds = [];

  // Compute group_seeds
  if (endpoints.length > 1) {
    for (var endpoint of endpoints) {
      try {
        var uri = "mysql://root:root@" + endpoint;
        var session = mysql.getSession(uri);
      } catch (e) {
        println(uri, e);
        throw e;
      }

      var group_seeds_endpoint = session.runSql("SELECT @@group_replication_local_address").fetchOne()[0];

        group_seeds.push(group_seeds_endpoint);
    }
  }

  for (var endpoint of endpoints) {
    try {
      var uri = "mysql://root:root@" + endpoint;
      var session = mysql.getSession(uri);
    } catch (e) {
      println(uri, e);
      throw e;
    }

    println("CHECKING GR SETTINGS AT:", endpoint);

    // Verify _local_address
    var local_address_ = session.runSql("SELECT @@group_replication_local_address").fetchOne()[0];

    var instance_address;
    var primary_member = cluster.status()["defaultReplicaSet"]["primary"];
    if (localaddress === undefined || endpoint != primary_member) {
      if (commstack == "MYSQL") {
        instance_address = session.runSql("SELECT CONCAT(COALESCE(@@report_host, @@hostname), ':', COALESCE(@@report_port, @@port))").fetchOne()[0];
      } else {
        instance_address = session.runSql("SELECT CONCAT(COALESCE(@@report_host, @@hostname), ':', COALESCE(@@report_port, @@port), '1')").fetchOne()[0];
      }

      EXPECT_EQ(local_address_, instance_address, endpoint+".local_address_check");
    } else {
      EXPECT_EQ(local_address_, localaddress, endpoint+".local_address_check");
    }

    // Verify _allow_list
    var allow_list_;
    if (__version_num < 80027) {
      allow_list_ = session.runSql("SELECT @@group_replication_ip_whitelist").fetchOne()[0];
    } else {
      allow_list_ = session.runSql("SELECT @@group_replication_ip_allowlist").fetchOne()[0];
    }

    if (allowlist === undefined) {
      EXPECT_EQ(allow_list_, "AUTOMATIC", endpoint+".allowlist_check");
    } else {
      EXPECT_EQ(allow_list_, allowlist, endpoint+".allowlist_check");
    }

    // Verify _group_seeds
    if (endpoints.length > 1) {
      // Remove itself from the seeds
      var new_group_seeds = group_seeds.filter(function(f) { return f !== local_address_ })

      var group_seeds_instance = session.runSql("SELECT @@group_replication_group_seeds").fetchOne()[0];

      EXPECT_EQ(new_group_seeds.toString(), group_seeds_instance, endpoint+".group_seeds_check");
    }

    // Verify SSL settings
    // NOTE: only check in single member clusters since we want to test
    // VERIFY_CA/VERIFY_IDENTIFY
    if (endpoints.length == 1) {
      var use_ssl = session.runSql("SELECT @@group_replication_recovery_use_ssl").fetchOne()[0];

      var ssl_mode = cluster.status()["defaultReplicaSet"]["ssl"];

      if (ssl_mode != "DISABLED") {
        EXPECT_EQ(use_ssl, 1);
      }

      if (ssl_mode == "VERIFY_CA" || ssl_mode == "VERIFY_IDENTITY") {
        var gr_ssl_ca = session.runSql("SELECT @@group_replication_recovery_ssl_ca").fetchOne()[0];
        var ssl_ca = session.runSql("SELECT @@ssl_ca").fetchOne()[0];
        EXPECT_EQ(gr_ssl_ca, ssl_ca);
      }
    }
  }
}

function shutdown_cluster(cluster) {
  // Get the cluster members
  var instances = [];
  for (var instance in cluster.status()["defaultReplicaSet"]["topology"]) {
      instances.push(instance);
  }

  for (var instance of instances) {
    var port = instance.substring(instance.indexOf(':') + 1);
    port = parseInt(port);

    disable_auto_rejoin(port);
    testutil.killSandbox(port);
  }

  for (var instance of instances) {
    var port = instance.substring(instance.indexOf(':') + 1);
    port = parseInt(port);

    testutil.startSandbox(port);
  }
}

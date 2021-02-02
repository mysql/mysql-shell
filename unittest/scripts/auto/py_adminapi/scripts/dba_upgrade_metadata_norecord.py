#@ {not real_host_is_loopback}

#@<> Snapshot File Name {VER(<8.0.0)}
snapshot_file = 'metadata-1.0.1-5.7.27-snapshot.sql'

#@<> Snapshot File Name {VER(>=8.0.0)}
snapshot_file = 'metadata-1.0.1-8.0.17-snapshot.sql'

#@ Creates the sample cluster
testutil.deploy_sandbox(__mysql_sandbox_port1, "root", {'report_host': hostname})
dba.configure_instance(__sandbox_uri1, {'clusterAdmin': 'tst_admin', 'clusterAdminPassword': 'tst_pwd'})
testutil.snapshot_sandbox_conf(__mysql_sandbox_port1)
cluster_admin_uri= "mysql://tst_admin:tst_pwd@" + __host + ":" + str(__mysql_sandbox_port1)

# Session to be used through all the AAPI calls
shell.connect(cluster_admin_uri)
dba.create_cluster('sample', {'ipAllowlist': '127.0.0.1,' + hostname_ip})

def set_metadata_1_0_1():
    dba.drop_metadata_schema({"force": True})
    testutil.import_data(__sandbox_uri1, __test_data_path + '/sql/' + snapshot_file)
    session.run_sql("UPDATE mysql_innodb_cluster_metadata.instances SET mysql_server_uuid = @@server_uuid")

#@ Upgrades the metadata, no registered routers
set_metadata_1_0_1()
session.run_sql("delete from mysql_innodb_cluster_metadata.routers")
dba.upgrade_metadata({'interactive':True})

#@ Upgrades the metadata, up to date
dba.upgrade_metadata({'interactive':True})

#@ Upgrades the metadata, interactive off, error
set_metadata_1_0_1()
EXPECT_THROWS(lambda: dba.upgrade_metadata({'interactive':False}),
    "Dba.upgrade_metadata: Outdated Routers found. Please upgrade the Routers before upgrading the Metadata schema")

#@ Upgrades the metadata, upgrade done by unregistering 10 routers and no router accounts
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (2, 'second', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (3, 'third', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (4, 'fourth', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (5, 'fifth', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (6, 'sixth', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (7, 'seventh', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (8, 'eighth', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (9, 'nineth', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (10, 'tenth', 2, NULL)")

# Chooses to unregister the existing routers
testutil.expect_prompt("Please select an option: ", "2")
testutil.expect_prompt("Unregistering a Router implies it will not be used in the Cluster, do you want to continue? [y/N]:", "y")
dba.upgrade_metadata({'interactive':True})


#@ Upgrades the metadata, upgrade done by unregistering more than 10 routers with router accounts
# Fake router account to get the account upgrade tested
set_metadata_1_0_1()
session.run_sql("CREATE USER mysql_router_test@`%` IDENTIFIED BY 'whatever'")
session.run_sql("CREATE USER mysql_router1_bc0e9n9dnfzk@`%` IDENTIFIED BY 'whatever'")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (2, 'second', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (3, 'third', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (4, 'fourth', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (5, 'fifth', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (6, 'sixth', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (7, 'seventh', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (8, 'eighth', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (9, 'nineth', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (10, 'tenth', 2, NULL)")
session.run_sql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (11, 'eleventh', 2, NULL)")

# Chooses to unregister the existing routers
testutil.expect_prompt("Please select an option: ", "2")
testutil.expect_prompt("Unregistering a Router implies it will not be used in the Cluster, do you want to continue? [y/N]:", "y")
dba.upgrade_metadata({'interactive':True})

#@<> Verifying grants for mysql_router_test
session.run_sql("SHOW GRANTS FOR mysql_router_test@`%`")
EXPECT_STDOUT_CONTAINS("GRANT USAGE ON *.*")
EXPECT_STDOUT_CONTAINS("GRANT SELECT, EXECUTE ON `mysql_innodb_cluster_metadata`.*")
EXPECT_STDOUT_CONTAINS("GRANT INSERT, UPDATE, DELETE ON `mysql_innodb_cluster_metadata`.`routers`")
EXPECT_STDOUT_CONTAINS("GRANT INSERT, UPDATE, DELETE ON `mysql_innodb_cluster_metadata`.`v2_routers`")
EXPECT_STDOUT_CONTAINS("GRANT SELECT ON `performance_schema`.`global_variables`")
EXPECT_STDOUT_CONTAINS("GRANT SELECT ON `performance_schema`.`replication_group_member_stats`")
EXPECT_STDOUT_CONTAINS("GRANT SELECT ON `performance_schema`.`replication_group_members`")

#@<> Verifying grants for mysql_router1_bc0e9n9dnfzk
session.run_sql("SHOW GRANTS FOR mysql_router1_bc0e9n9dnfzk@`%`")
EXPECT_STDOUT_CONTAINS("GRANT USAGE ON *.*")
EXPECT_STDOUT_CONTAINS("GRANT SELECT, EXECUTE ON `mysql_innodb_cluster_metadata`.*")
EXPECT_STDOUT_CONTAINS("GRANT INSERT, UPDATE, DELETE ON `mysql_innodb_cluster_metadata`.`routers`")
EXPECT_STDOUT_CONTAINS("GRANT INSERT, UPDATE, DELETE ON `mysql_innodb_cluster_metadata`.`v2_routers`")
EXPECT_STDOUT_CONTAINS("GRANT SELECT ON `performance_schema`.`global_variables`")
EXPECT_STDOUT_CONTAINS("GRANT SELECT ON `performance_schema`.`replication_group_member_stats`")
EXPECT_STDOUT_CONTAINS("GRANT SELECT ON `performance_schema`.`replication_group_members`")

#@ Test Migration from 1.0.1 to 2.0.0
set_metadata_1_0_1()
test_session = mysql.get_session(__sandbox_uri1)

# Chooses to unregister the existing router
testutil.expect_prompt("Please select an option: ", "2")
testutil.expect_prompt("Unregistering a Router implies it will not be used in the Cluster, do you want to continue? [y/N]:", "y")
dba.upgrade_metadata({'interactive':True})

#@<> Cleanup
session.close()
test_session.close()
testutil.destroy_sandbox(__mysql_sandbox_port1)

#@ {VER(>=8.0.11)}

# Plain replicaset setup test, use as a template for other tests that check
# a specific feature/aspect across the whole API

#@<> Setup
testutil.deploy_raw_sandbox(port=__mysql_sandbox_port1, rootpass="root")
testutil.snapshot_sandbox_conf(port=__mysql_sandbox_port1)
testutil.deploy_sandbox(port=__mysql_sandbox_port2, rootpass="root")
testutil.snapshot_sandbox_conf(port=__mysql_sandbox_port2)
testutil.deploy_sandbox(port=__mysql_sandbox_port3, rootpass="root")
testutil.snapshot_sandbox_conf(port=__mysql_sandbox_port3)

shell.options.useWizards = False

#@ configure_replica_set_instance + create admin user
dba.configure_replica_set_instance(instance=__sandbox_uri1, options={"clusterAdmin":"admin", "clusterAdminPassword":"bla"})
testutil.restart_sandbox(port=__mysql_sandbox_port1)

#@ configure_replica_set_instance
dba.configure_replica_set_instance(instance=__sandbox_uri2, options={"clusterAdmin":"admin", "clusterAdminPassword":"bla"})
dba.configure_replica_set_instance(instance=__sandbox_uri3, options={"clusterAdmin":"admin", "clusterAdminPassword":"bla"})

#@ create_replica_set
shell.connect(connectionData=__sandbox_uri1)

rs = dba.create_replica_set(name="myrs")

#@ status
rs.status()

#@ disconnect
rs.disconnect()

#@ get_replica_set
rs = dba.get_replica_set()

#@<> add_instance using clone recovery {VER(>=8.0.17)}
rs.add_instance(instance=__sandbox_uri2, options={"recoveryMethod":'clone'})

#@<> add_instance using incremental recovery {VER(<8.0.17)}
rs.add_instance(instance=__sandbox_uri2, options={"recoveryMethod":'incremental'})

#@<> add_instance 3
rs.add_instance(instance=__sandbox_uri3, options={"recoveryMethod":'incremental'})

#@ remove_instance
rs.remove_instance(instance=__sandbox_uri2)

rs.add_instance(instance=__sandbox_uri2, options={"recoveryMethod":'incremental'})

#@ set_primary_instance
rs.set_primary_instance(instance=__sandbox_uri3)

#@ force_primary_instance (prepare)
testutil.stop_sandbox(port=__mysql_sandbox_port3, options={"wait":1})
rs = dba.get_replica_set()

rs.status()

#@ force_primary_instance
rs.force_primary_instance(instance=__sandbox_uri1)

# rejoinInstance
testutil.start_sandbox(port=__mysql_sandbox_port3)

# XXX rs.rejoinInstance(__sandbox_uri3)

#@ list_routers
cluster_id = session.run_sql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetch_one()[0]
session.run_sql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id])
session.run_sql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id])

rs.list_routers()

#@ remove_router_metadata
rs.remove_router_metadata(routerDef="routerhost1::system")

rs.list_routers()

#@ create_replica_set(adopt)
session.run_sql("DROP SCHEMA mysql_innodb_cluster_metadata")
testutil.wait_member_transactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

rs = dba.create_replica_set(name="adopted", options={"adoptFromAR":True})

#@<> create_replica_set(adopt) - status
rs.status()

rs.disconnect()

#@<> Cleanup
testutil.destroy_sandbox(port=__mysql_sandbox_port1)
testutil.destroy_sandbox(port=__mysql_sandbox_port2)
testutil.destroy_sandbox(port=__mysql_sandbox_port3)

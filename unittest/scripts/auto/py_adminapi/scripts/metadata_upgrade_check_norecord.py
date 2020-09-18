#@ {not real_host_is_loopback}

import os
import filecmp

#@<> Snapshot File Name {VER(<8.0.0)}
snapshot_file = 'metadata-1.0.1-5.7.27-snapshot.sql'

#@<> Snapshot File Name {VER(>=8.0.0)}
snapshot_file = 'metadata-1.0.1-8.0.17-snapshot.sql'

#@<> Comparing Upgraded Model vs Actual Model
# Creates the sample cluster
testutil.deploy_sandbox(__mysql_sandbox_port1, "root", {'report_host': hostname})
testutil.snapshot_sandbox_conf(__mysql_sandbox_port1)
shell.connect(__sandbox_uri1)
dba.create_cluster('sample', {'ipAllowlist': '127.0.0.1,' + hostname_ip});

# Gets a snapshot of the latest version of the metadata
testutil.dump_data(__sandbox_uri1, "latest_md.sql", ["mysql_innodb_cluster_metadata"], {'noData':True, 'skipComments':True});

# Replaces the metadata with the snapshot at version 1.0.1, removes schema_version to make it even 1.0.0
dba.drop_metadata_schema({"force": True});
md_1_0_1 = os.path.join(__test_data_path, "sql", snapshot_file)
testutil.import_data(__sandbox_uri1, __test_data_path + '/sql/' + snapshot_file)
session.run_sql("UPDATE mysql_innodb_cluster_metadata.instances SET mysql_server_uuid = @@server_uuid")
session.run_sql("DELETE FROM mysql_innodb_cluster_metadata.routers")

# Upgrades the metadata to the latest version
dba.upgrade_metadata()
session.close()

# Takes the final snapshot
testutil.dump_data(__sandbox_uri1, "upgraded_md.sql", ["mysql_innodb_cluster_metadata"], {'noData':True, 'skipComments':True})
testutil.destroy_sandbox(__mysql_sandbox_port1)

equal = filecmp.cmp("latest_md.sql", "upgraded_md.sql")

if not equal:
    print_differences("latest_md.sql", "upgraded_md.sql")

testutil.rmfile("latest_md.sql");
testutil.rmfile("upgraded_md.sql");

if not equal:
    testutil.fail("Upgraded metadata file differs from latest version.")

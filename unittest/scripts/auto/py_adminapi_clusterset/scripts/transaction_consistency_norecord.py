#@ {VER(>=8.0.27)}

# Various tests regarding transaction consistency between the primary cluster
# and replica cluster. Ensures reconciliation happens correctly.


#@<> Setup

testutil.deploy_sandbox(__mysql_sandbox_port1, "root", {"report_host":hostname})
testutil.deploy_sandbox(__mysql_sandbox_port2, "root", {"report_host":hostname})
testutil.deploy_sandbox(__mysql_sandbox_port3, "root", {"report_host":hostname})
testutil.deploy_sandbox(__mysql_sandbox_port4, "root", {"report_host":hostname})

session1 = mysql.get_session(__sandbox_uri1)
session2 = mysql.get_session(__sandbox_uri2)
session3 = mysql.get_session(__sandbox_uri3)

shell.connect(__sandbox_uri1)
pc = dba.create_cluster("primary", {"gtidSetIsComplete":1})
cs = pc.create_cluster_set("cs")
rc = cs.create_replica_cluster(__sandbox_uri2, "replica")
rc.add_instance(__sandbox_uri3)
pc.add_instance(__sandbox_uri4)

#@<> Regression test to ensure only VCLEs are injected by ensure_transaction_set_consistent()
# Bug #34462141	clusterset.rejoinCluster() hangs

# pause replication
session2.run_sql("stop replica for channel 'clusterset_replication'")

# inject some trxs
session1.run_sql("create schema test")
session1.run_sql("create table test.tbl (a int primary key)")
for i in range(10):
    session1.run_sql(f"insert into test.tbl values ({i})")

vcuuid1 = session1.run_sql("select @@group_replication_view_change_uuid").fetch_one()[0]
vcuuid2 = session2.run_sql("select @@group_replication_view_change_uuid").fetch_one()[0]

for i in range(5):
    session1.run_sql(f"set gtid_next='{vcuuid1}:{i+100}'")
    session1.run_sql("start transaction")
    session1.run_sql("commit")
session1.run_sql(f"set gtid_next='automatic'")

session2.run_sql("set global super_read_only=0")
for i in range(5):
    session2.run_sql(f"set gtid_next='{vcuuid2}:{i+100}'")
    session2.run_sql("start transaction")
    session2.run_sql("commit")
session2.run_sql(f"set gtid_next='automatic'")
session2.run_sql("set global super_read_only=1")

# inject fake vcles

session2.run_sql("change replication source to source_delay=5 for channel 'clusterset_replication'")
session2.run_sql("start replica for channel 'clusterset_replication'")

#shell.options["verbose"]=1
#shell.options["dba.logSql"]=2

shell.dump_rows(session2.run_sql("select @@gtid_executed, @@group_replication_group_name, @@group_replication_view_change_uuid"), "vertical")

shell.dump_rows(session1.run_sql("select @@gtid_executed, @@group_replication_group_name, @@group_replication_view_change_uuid"), "vertical")

cs.set_primary_cluster("replica")

shell.dump_rows(session1.run_sql("select @@gtid_executed, @@group_replication_group_name, @@group_replication_view_change_uuid"), "vertical")

#shell.options["verbose"]=0

# ensure all trxs were replicated for real
EXPECT_EQ(10, session2.run_sql("select count(*) from test.tbl").fetch_one()[0])

group_name1 = session1.run_sql("select @@group_replication_group_name").fetch_one()[0]
group_name2 = session2.run_sql("select @@group_replication_group_name").fetch_one()[0]

# ensure there are no bogus transactions in binlog
def count_injected_transactions(sess):
    general_log = sess.run_sql("select @@general_log_file").fetch_one()[0]
    bad_trxs1 = 0
    bad_trxs2 = 0
    for line in open(general_log, "r"):
        if "SET gtid_next = " in line:
            if group_name1 in line:
                bad_trxs1 += 1
            if group_name2 in line:
                bad_trxs2 += 1
    return bad_trxs1, bad_trxs2

bad_trxs1, bad_trxs2 = count_injected_transactions(session1)
EXPECT_EQ(0, bad_trxs1, "legitimate trxs from cluster 'primary' injected at 'primary'")
EXPECT_EQ(0, bad_trxs2, "legitimate trxs from cluster 'replica' injected at 'primary'")

bad_trxs1, bad_trxs2 = count_injected_transactions(session2)
EXPECT_EQ(0, bad_trxs1, "legitimate trxs from cluster 'primary' injected at 'replica'")
EXPECT_EQ(0, bad_trxs2, "legitimate trxs from cluster 'replica' injected at 'replica'")

#@<> same thing on the way back
session1.run_sql("stop replica for channel 'clusterset_replication'")

# inject some trxs
for i in range(10):
    session2.run_sql(f"insert into test.tbl values ({i+10})")

session1.run_sql("start replica for channel 'clusterset_replication'")

cs.set_primary_cluster("primary")

bad_trxs1, bad_trxs2 = count_injected_transactions(session1)
EXPECT_EQ(0, bad_trxs1, "legitimate trxs from cluster 'primary' injected at 'primary'")
EXPECT_EQ(0, bad_trxs2, "legitimate trxs from cluster 'replica' injected at 'primary'")

bad_trxs1, bad_trxs2 = count_injected_transactions(session2)
EXPECT_EQ(0, bad_trxs1, "legitimate trxs from cluster 'primary' injected at 'replica'")
EXPECT_EQ(0, bad_trxs2, "legitimate trxs from cluster 'replica' injected at 'replica'")

cs.set_primary_cluster("replica")


#@<> Destroy
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.destroy_sandbox(__mysql_sandbox_port3)
testutil.destroy_sandbox(__mysql_sandbox_port4)
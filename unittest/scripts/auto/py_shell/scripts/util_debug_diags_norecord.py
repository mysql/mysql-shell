import zipfile
import yaml
import os

#@<> Init
testutil.deploy_sandbox(__mysql_sandbox_port1, "root")
testutil.deploy_sandbox(__mysql_sandbox_port2, "root")
testutil.deploy_sandbox(__mysql_sandbox_port3, "root")
testutil.deploy_sandbox(__mysql_sandbox_port4, "root")

def zfpath(zf, fn):
    return os.path.basename(zf.filename).rsplit(".", 1)[0]+"/"+fn

def ZEXPECT_NOT_EMPTY(zf, fn):
    with zf.open(zfpath(zf,fn), "r") as f:
        EXPECT_NE("", f.read(), f"{fn} is empty")

def ZEXPECT_EXISTS(zf, fn):
    EXPECT_NO_THROWS(lambda: zf.open(zfpath(zf,fn), "r"))

def ZEXPECT_NOT_EXISTS(zf, fn):
    EXPECT_THROWS(lambda: zf.open(zfpath(zf,fn), "r"), f"There is no item named '{zfpath(zf,fn)}'")

def EXPECT_NO_FILE(fn):
    EXPECT_FALSE(os.path.exists(fn), f"{fn} exists but shouldn't")

def EXPECT_FILE(fn):
    EXPECT_TRUE(os.path.exists(fn), f"{fn} doesn't exist as expected")

g_expected_files_per_instance = [
    ("", "replication_applier_configuration.tsv", None),
    ("select * from performance_schema.replication_applier_filters", "replication_applier_filters.tsv", 80000),
    ("select * from performance_schema.replication_applier_global_filters", "replication_applier_global_filters.tsv", 80000),
    ("", "replication_applier_status.tsv", None),
    ("", "replication_applier_status_by_coordinator.tsv", None),
    ("", "replication_applier_status_by_worker.tsv", None),
    ("select * from performance_schema.replication_asynchronous_connection_failover_managed", "replication_asynchronous_connection_failover_managed.tsv", 80000),
    ("", "replication_connection_configuration.yaml", None),
    ("", "replication_connection_status.tsv", None),
    ("", "replication_group_member_stats.tsv", None),
    ("select * from performance_schema.replication_group_members", "replication_group_members.tsv", None),
    ("", "SHOW_BINARY_LOGS.tsv", None),
    ("", "SHOW_SLAVE_HOSTS.tsv", None),
    ("", "SHOW_MASTER_STATUS.tsv", None),
    ("", "processlist.tsv", 80000),
    ("", "SHOW_PROCESSLIST.tsv", -80000),
    ("", "SHOW_GLOBAL_STATUS.tsv", None),
    ("", "error_log.tsv", 80022),
    ("""SELECT g.variable_name name, g.variable_value value /*!80000, i.variable_source source*/
        FROM performance_schema.global_variables g
        /*!80000 JOIN performance_schema.variables_info i ON g.variable_name = i.variable_name*/ order by name""", "global_variables.tsv", None),
    ("", "innodb_metrics.tsv", None),
    ("", "innodb_status.yaml", None)
]

def CHECK_QUERY(session, data, query, note=None):
    all = []
    res = session.run_sql(query)
    all.append("# "+"\t".join([c.column_label for c in res.columns])+"\n")
    for row in res.fetch_all():
        line = []
        for i in range(len(row)):
            line.append(str(row[i]) if row[i] is not None else "NULL")
        all.append("\t".join(line)+"\n")
    EXPECT_EQ_TEXT("".join(all), "".join(data), note)


def CHECK_METADATA(session, zf):
    tables = [r[0] for r in session.run_sql("show full tables in mysql_innodb_cluster_metadata").fetch_all() if r[1] == "BASE TABLE" or r[0] == "schema_version"]
    for table in tables:
        with zf.open(zfpath(zf, f"mysql_innodb_cluster_metadata.{table}.tsv"), "r") as f:
            data = [line.decode("utf-8") for line in f.readlines()]
            CHECK_QUERY(session, data, f"select * from mysql_innodb_cluster_metadata.{table}")


def CHECK_INSTANCE(session, zf, instance_id, expected_files_per_instance):
    x,y,z = session.run_sql("select @@version").fetch_one()[0].split("-")[0].split(".")
    ver = int(x)*10000+int(y)*100+int(z)
    for q, fn, minver in expected_files_per_instance:
        if minver and ((minver>0 and ver < minver) or (minver<0 and ver > -minver)):
            continue
        with zf.open(zfpath(zf, f"{instance_id}.{fn}"), "r") as f:
            data = [line.decode("utf-8") for line in f.readlines()]
            if q:
                CHECK_QUERY(session, data, q, f"{instance_id}: {fn}")
            else:
                EXPECT_NE("", data, f"{instance_id}: {fn}")


def CHECK_DIAGPACK(file, sessions_or_errors, is_cluster=False, innodbMutex=False, allMembers=1, schemaStats=False, slowQueries=False):
    expected_files_per_instance = g_expected_files_per_instance[:]
    if innodbMutex:
        expected_files_per_instance.append(("", "innodb_mutex.tsv", None))
    with zipfile.ZipFile(file, mode="r") as zf:
        def read_yaml(fn):
            with zf.open(fn, "r") as f:
                return yaml.safe_load(f.read().decode("utf-8"))
        def read_tsv(fn):
            with zf.open(fn, "r") as f:
                return [l.decode("utf-8").split("\t") for l in f.readlines()]
        def read_text(fn):
            with zf.open(fn, "r") as f:
                return f.read().decode("utf-8")
        ZEXPECT_NOT_EMPTY(zf, "shell_info.yaml")
        ZEXPECT_NOT_EMPTY(zf, "mysqlsh.log")
        filelist = zf.namelist()
        if is_cluster:
            ZEXPECT_NOT_EMPTY(zf, "cluster_accounts.tsv")
            CHECK_METADATA(sessions_or_errors[0][1], zf)
            if allMembers:
                ZEXPECT_NOT_EMPTY(zf, "ping.txt")
        else:
            EXPECT_FALSE(zfpath(zf, "cluster_accounts.tsv") in filelist)
        ZEXPECT_EXISTS(zf, "tables_without_a_PK.tsv")
        if schemaStats:
            ZEXPECT_EXISTS(zf, "schema_object_overview.tsv")
            ZEXPECT_EXISTS(zf, "top_biggest_tables.tsv")
        else:
            ZEXPECT_NOT_EXISTS(zf, "schema_object_overview.tsv")
            ZEXPECT_NOT_EXISTS(zf, "top_biggest_tables.tsv")
        first = True
        for instance_id, session_or_error in sessions_or_errors:
            if __version_num > 80000:
                if slowQueries:
                    ZEXPECT_EXISTS(zf, f"{instance_id}.slow_log.tsv")
                    ZEXPECT_EXISTS(zf, f"{instance_id}.slow_log.yaml")
                else:
                    ZEXPECT_NOT_EXISTS(zf, f"{instance_id}.slow_log.tsv")
                    ZEXPECT_NOT_EXISTS(zf, f"{instance_id}.slow_log.yaml")
            if not allMembers:
                if first:
                    ZEXPECT_EXISTS(zf, f"{instance_id}.global_variables.tsv")
                    first = False
                else:
                    ZEXPECT_NOT_EXISTS(zf, f"{instance_id}.global_variables.tsv")
                    continue
            if zfpath(zf, f"{instance_id}.connect_error.txt") in filelist:
                EXPECT_IN(session_or_error, read_text(zfpath(zf, f"{instance_id}.connect_error.txt")))
            else:
                assert type(session_or_error) != str, f"{instance_id} = {session_or_error}"
                CHECK_INSTANCE(session_or_error, zf, instance_id, expected_files_per_instance)

def run_collect(uri, path, options={}, *, password="root", **kwargs):
    if not options:
        options = kwargs
    args = [uri, "--passwords-from-stdin", "-e", f"util.debug.collectDiagnostics('{path}', {repr(options)})"]
    print(args)
    testutil.call_mysqlsh(args, password+"\n", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])


outdir = __tmp_dir+"/diagout"
try:
    testutil.rmdir(outdir, True)
except:
    pass
testutil.mkdir(outdir)


session1 = mysql.get_session(__sandbox_uri1)
session2 = mysql.get_session(__sandbox_uri2)
session3 = mysql.get_session(__sandbox_uri3)
session4 = mysql.get_session(__sandbox_uri4)

session1.run_sql("create user nothing@'%'")

session1.run_sql("create user selectonly@'%'")
session1.run_sql("grant select on *.* to selectonly@'%'")

session1.run_sql("create user minimal@'%'")
session1.run_sql("grant select, process, replication client, replication slave on *.* to minimal@'%'")
session1.run_sql("grant execute on sys.* to minimal@'%'")

session1.run_sql("create user nomd@'%'")
session1.run_sql("grant select on mysql.* to nomd@'%'")
session1.run_sql("grant select on sys.* to nomd@'%'")
session1.run_sql("grant select on performance_schema.* to nomd@'%'")

#@<> invalid option
run_collect(__sandbox_uri1, os.path.join(outdir,"diag0i.zip"), {"innodb_mutex":1})
EXPECT_STDOUT_CONTAINS("debug.collectDiagnostics: Invalid options at Argument #2: innodb_mutex")
EXPECT_NO_FILE(os.path.join(outdir,"diag0i.zip"))

#@<> BUG#34048754 - 'path' set to an empty string should result in an error
run_collect(__sandbox_uri1, "")
EXPECT_STDOUT_CONTAINS("debug.collectDiagnostics: 'path' cannot be an empty string")
EXPECT_NO_FILE(".zip")

#@<> Regular instance
shell.connect(__sandbox_uri1)

run_collect(__sandbox_uri1, os.path.join(outdir,"diag0.zip"))
# allMembers is true but it's a standalone instance, so it's a no-op
CHECK_DIAGPACK(os.path.join(outdir,"diag0.zip"), [(0, session1)], is_cluster=False, allMembers=1, innodbMutex=False)

#@<> output is a directory, default filename with timestamp
run_collect(__sandbox_uri1, outdir+"/")
filenames = [f for f in os.listdir(outdir) if f.startswith("mysql-diagnostics-")]
EXPECT_NE([], filenames)
CHECK_DIAGPACK(os.path.join(outdir,filenames[0]), [(0, session1)], is_cluster=False, innodbMutex=False)

#@<> slowQueries fail
run_collect(__sandbox_uri1, os.path.join(outdir, "diag0.0.zip"), {"slowQueries":1})
EXPECT_STDOUT_CONTAINS("slowQueries option requires slow_query_log to be enabled and log_output to be set to TABLE")
EXPECT_NO_FILE(os.path.join(outdir,"diag0.0.zip"))

#@<> slowQueries OK
session.run_sql("set global slow_query_log=1")
session.run_sql("set session long_query_time=0.1")
session.run_sql("set global log_output='TABLE'")
session.run_sql("select sleep(1)")
run_collect(__sandbox_uri1, os.path.join(outdir,"diag0.0.zip"), {"slowQueries":1})
CHECK_DIAGPACK(os.path.join(outdir,"diag0.0.zip"), [(0, session1)], is_cluster=False, innodbMutex=False, slowQueries=True)

#@<> with no access
run_collect("nothing:@localhost:"+str(__mysql_sandbox_port1), os.path.join(outdir,"diag0.1.zip"))
EXPECT_STDOUT_CONTAINS("SELECT command denied to user 'nothing'@'localhost'")
EXPECT_NO_FILE(os.path.join(outdir,"diag0.1.zip"))

#@<> no access + ignoreErrors
run_collect("nothing:@localhost:"+str(__mysql_sandbox_port1), os.path.join(outdir,"diag0.1.1.zip"), {"allMembers":0, "ignoreErrors":1})
EXPECT_FILE(os.path.join(outdir,"diag0.1.1.zip"))

#@<> with select only access
run_collect("selectonly:@localhost:"+str(__mysql_sandbox_port1), os.path.join(outdir,"diag0.1.zip"))
EXPECT_STDOUT_CONTAINS("Access denied; you need (at least one of)")
EXPECT_NO_FILE(os.path.join(outdir,"diag0.1.zip"))

#@<> minimal privs
run_collect("minimal:@localhost:"+str(__mysql_sandbox_port1), os.path.join(outdir,"diag0.2.zip"))
CHECK_DIAGPACK(os.path.join(outdir,"diag0.2.zip"), [(0, session1)], is_cluster=False, innodbMutex=False)

#@<> Duplicate filename
run_collect(__sandbox_uri1, os.path.join(outdir,"diag0.zip"))
EXPECT_STDOUT_CONTAINS("already exists")

#@<> Regular instance + innodbMutex + schemaStatus
run_collect(__sandbox_uri1, os.path.join(outdir,"diag0i.zip"), innodbMutex=1, schemaStats=1, allMembers=1)
CHECK_DIAGPACK(os.path.join(outdir,"diag0i.zip"), [(0, session1)], is_cluster=False, innodbMutex=True, schemaStats=True)

#@<> ReplicaSet {VER(>8.0.0)}
shell.connect(__sandbox_uri1)

r = dba.create_replica_set("replicaset", {"gtidSetIsComplete":1})
r.add_instance(__sandbox_uri2)

run_collect(__sandbox_uri1, os.path.join(outdir,"diag1.zip"), allMembers=1)
CHECK_DIAGPACK(os.path.join(outdir,"diag1.zip"), [(1, session1), (2, session2)], is_cluster=True, innodbMutex=False)

session.run_sql("drop schema mysql_innodb_cluster_metadata")

session2.close()
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.deploy_sandbox(__mysql_sandbox_port2, "root")
session2 = mysql.get_session(__sandbox_uri2)

r.disconnect()

#@<> Setup InnoDB Cluster
shell.connect(__sandbox_uri1)
c = dba.create_cluster("cluster", {"gtidSetIsComplete":1})
c.add_instance(__sandbox_uri2)
c.add_instance(__sandbox_uri3)

testutil.wait_member_transactions(__mysql_sandbox_port3, __mysql_sandbox_port1)

#@<> cluster with innodbMutex + schemaStats
testutil.expect_password("Password for root: ", "root")
run_collect(__sandbox_uri1, os.path.join(outdir,"diag1.1.zip"), innodbMutex=1, schemaStats=1, allMembers=1)
CHECK_DIAGPACK(os.path.join(outdir,"diag1.1.zip"), [(1, session1), (2, session2), (3, session3)], is_cluster=True, innodbMutex=True, schemaStats=True)

#@<> cluster with allMembers=False
run_collect(__sandbox_uri1, os.path.join(outdir,"diag1.2.zip"), schemaStats=1, allMembers=0)
EXPECT_STDOUT_NOT_CONTAINS("Password for root:")
CHECK_DIAGPACK(os.path.join(outdir,"diag1.2.zip"), [(1, session1), (2, session2), (3, session3)], is_cluster=True, schemaStats=True, allMembers=False)

#@<> with bad password
run_collect(__sandbox_uri1, os.path.join(outdir,"diag1.3.zip"), password="bla", schemaStats=1, allMembers=1)
EXPECT_STDOUT_CONTAINS("Access denied for user 'root'@'localhost'")
EXPECT_NO_FILE(os.path.join(outdir,"diag1.3.zip"))

#@<> with no access to MD
run_collect("nomd:@localhost:"+str(__mysql_sandbox_port1), os.path.join(outdir,"diag1.4.zip"), allMembers=1)
EXPECT_STDOUT_CONTAINS("SELECT command denied to user 'nomd'@'localhost' for table 'instances'")
EXPECT_NO_FILE(os.path.join(outdir,"diag1.4.zip"))

#@<> no access to MD but ignoreErrors (allMembers=1, so still error)
run_collect("nomd:@localhost:"+str(__mysql_sandbox_port1), os.path.join(outdir,"diag1.4.zip"), allMembers=1, ignoreErrors=1)
EXPECT_STDOUT_CONTAINS("SELECT command denied to user 'nomd'@'localhost' for table 'instances'")
EXPECT_NO_FILE(os.path.join(outdir,"diag1.4.zip"))

#@<> no access to MD but ignoreErrors (allMembers=0, so success)
run_collect(__sandbox_uri1, os.path.join(outdir,"diag1.4.zip"), allMembers=0, ignoreErrors=1)
EXPECT_FILE(os.path.join(outdir,"diag1.4.zip"))

#@<> minimal privs with innodb cluster
run_collect("minimal:@localhost:"+str(__mysql_sandbox_port1), os.path.join(outdir,"diag1.5.zip"), allMembers=1, password="")
CHECK_DIAGPACK(os.path.join(outdir,"diag1.5.zip"), [(1, session1), (2, session2), (3, session3)], is_cluster=True, innodbMutex=False, allMembers=1)

#@<> Shutdown instance
session2.close()
testutil.stop_sandbox(__mysql_sandbox_port2, {"wait":1})

run_collect(__sandbox_uri1, os.path.join(outdir,"diag2.zip"), allMembers=1)
CHECK_DIAGPACK(os.path.join(outdir,"diag2.zip"), [(1, session1), (2, "MySQL Error (2003): mysql.get_session: Can't connect to MySQL server on"), (3, session3)], is_cluster=True, innodbMutex=False)

#@<> Take offline
session3.run_sql("stop group_replication")

run_collect(__sandbox_uri1, os.path.join(outdir,"diag3.zip"), allMembers=1)
CHECK_DIAGPACK(os.path.join(outdir,"diag3.zip"), [(1, session1), (2, "MySQL Error (2003): mysql.get_session: Can't connect to MySQL server on"), (3, session3)], is_cluster=True, innodbMutex=False)

session1.run_sql("stop group_replication")

run_collect(__sandbox_uri1, os.path.join(outdir,"diag4.zip"), allMembers=1)
CHECK_DIAGPACK(os.path.join(outdir,"diag4.zip"), [(1, session1), (2, "MySQL Error (2003): mysql.get_session: Can't connect to MySQL server on"), (3, session3)], is_cluster=True, innodbMutex=False)

#@<> Expand to ClusterSet {VER(>8.0.0)}
testutil.start_sandbox(__mysql_sandbox_port2)
session2 = mysql.get_session(__sandbox_uri2)
c = dba.reboot_cluster_from_complete_outage()
c.rejoin_instance(__sandbox_uri2)
c.rejoin_instance(__sandbox_uri3)

cs = c.create_cluster_set("cset")
c2 = cs.create_replica_cluster(__sandbox_uri4, "cluster2")

testutil.wait_member_transactions(__mysql_sandbox_port4, __mysql_sandbox_port1)
testutil.wait_member_transactions(__mysql_sandbox_port3, __mysql_sandbox_port1)
testutil.wait_member_transactions(__mysql_sandbox_port2, __mysql_sandbox_port1)

run_collect(__sandbox_uri1, os.path.join(outdir,"diag5.zip"), allMembers=1)
CHECK_DIAGPACK(os.path.join(outdir,"diag5.zip"), [(1, session1), (2, session2), (3, session3), (4, session4)], is_cluster=True, innodbMutex=False)

c.disconnect()
cs.disconnect()
c2.disconnect()

#@<> Cleanup
session1.close()
session2.close()
session3.close()
session4.close()

testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.destroy_sandbox(__mysql_sandbox_port3)
testutil.destroy_sandbox(__mysql_sandbox_port4)

try:
    testutil.rmdir(outdir, True)
except:
    pass

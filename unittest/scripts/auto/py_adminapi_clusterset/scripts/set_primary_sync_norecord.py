#@ {VER(>=8.0.27)}
# Check that switchover executes in the correct order at a low-level

#@<> utils
import string
import re
import mysqlsh
import threading


class SQLLogAnalyzer:
    def __init__(self, sbports):
        self.sbports = sbports
        self.logs = []
        self.last_timestamp = len(sbports)*[""]
    def read(self):
        for i, (port, last_ts) in enumerate(zip(self.sbports, self.last_timestamp)):
            logs = testutil.read_general_log(port, last_ts)
            if logs:
                self.last_timestamp[i] = logs[-1]["timestamp"]
                for l in logs:
                    self.logs.append((l["timestamp"], port, l["sql"]))
        self.logs.sort()
    def discard(self):
        self.logs = []
    def filter(self, pat):
        self.logs = [l for l in self.logs if not re.match(
            pat, l[2], re.IGNORECASE)]
    def __str__(self):
        return "\n".join([f"{l[0]}\t{l[1]}\t{l[2]}" for l in self.logs])
    def find(self, port, pat):
        "Return timestamp of the 1st occurrence of the pattern"
        for ts, p, sql in self.logs:
            if p == port and re.match(pat, sql, re.IGNORECASE):
                return ts
        return None
    def find_after(self, port, ts, pat):
        assert ts
        found_ts = False
        for t, p, sql in self.logs:
            if p != port:
                continue
            if t >= ts and not found_ts:
                found_ts = True
                continue
            if found_ts and re.match(pat, sql, re.IGNORECASE):
                return t, sql
        return None
    def find_before(self, port, ts, pat):
        assert ts
        for t, p, sql in self.logs:
            if p != port:
                continue
            if t >= ts:
                return None
            if re.match(pat, sql, re.IGNORECASE):
                return t, sql
        return None


def EXPECT_SYNC_AT_PROMOTED_BEFORE_FTWRL(logs, promoted_sb, demoted_sb):
    ftwrl = logs.find(demoted_sb, "FLUSH TABLES WITH READ LOCK")
    gtid_wait = logs.find(promoted_sb, "SELECT WAIT_FOR_EXECUTED_GTID_SET")
    EXPECT_LT(gtid_wait, ftwrl)


def EXPECT_NO_GR_OPS_DURING_FTWRL_(logs, sb):
    ftwrl = logs.find(sb, "FLUSH TABLES WITH READ LOCK")
    unlock = logs.find(sb, "UNLOCK TABLES")
    op = logs.find(sb, r"SELECT (asynchronous_|group_replication_).*\(.*\)")
    if op and ftwrl and unlock:
        EXPECT_NOT_BETWEEN(ftwrl[0], op[0], unlock[0])
    EXPECT_TRUE(not ftwrl or unlock)


def EXPECT_NO_GR_OPS_DURING_FTWRL(logs):
    EXPECT_NO_GR_OPS_DURING_FTWRL_(logs, __mysql_sandbox_port1)
    EXPECT_NO_GR_OPS_DURING_FTWRL_(logs, __mysql_sandbox_port2)
    EXPECT_NO_GR_OPS_DURING_FTWRL_(logs, __mysql_sandbox_port3)
    EXPECT_NO_GR_OPS_DURING_FTWRL_(logs, __mysql_sandbox_port4)
    EXPECT_NO_GR_OPS_DURING_FTWRL_(logs, __mysql_sandbox_port5)
    EXPECT_NO_GR_OPS_DURING_FTWRL_(logs, __mysql_sandbox_port6)


def CHECK_SRO_CLEAR_AT_PROMOTED(logs, promoted_sb, demoted_sb):
    # check that SRO is only disabled at the promoted after FTWRL and before UNLOCK
    clear_sro = logs.find(promoted_sb, "SET.*SUPER_READ_ONLY.*(OFF|0)")
    EXPECT_TRUE(clear_sro, "clear_sro")
    ftwrl = logs.find(demoted_sb, "FLUSH TABLES WITH READ LOCK")
    EXPECT_TRUE(ftwrl, "ftwrl")
    unlock = logs.find_after(demoted_sb, ftwrl[0], "UNLOCK TABLES")
    EXPECT_TRUE(unlock, "unlock tables")
    EXPECT_BETWEEN(ftwrl[0], unlock[0], clear_sro[0])
    clear_sro_too_soon = logs.find_before(
        promoted_sb, ftwrl[0], "SET.*SUPER_READ_ONLY.*(OFF|0)")
    EXPECT_FALSE(clear_sro_too_soon, "clear_sro_too_soon    ")


class Inserter(threading.Thread):
    def __init__(self, sbindex, uri, table_name):
        super().__init__()
        self.sbindex = sbindex
        self.session = mysql.get_session(uri)
        self.done = 0
        self.errors = []
        self.table_name = f"testdb{self.sbindex}.{table_name}"
    def prepare(self, session):
        session.run_sql(f"CREATE SCHEMA IF NOT EXISTS testdb{self.sbindex}")
        session.run_sql(
            f"CREATE TABLE {self.table_name} (pk int primary key auto_increment, ts timestamp(6))")
    def start(self):
        self.done = 0
        super().start()
    def stop(self):
        self.done = 1
        self.join()
    def run(self):
        while not self.done:
            try:
                sql = f"INSERT INTO {self.table_name} VALUES (DEFAULT, NOW(6))"
                self.session.run_sql(sql)
            except mysqlsh.DBError as e:
                self.errors.append(e.code)
            except:
                import traceback
                traceback.print_exc()
                print("QUERY:", sql)
                raise


def EXPECT_NO_INSERTS(inserter, session):
    nrows = session.run_sql(
        f"SELECT count(*) FROM {inserter.table_name}").fetch_one()[0]
    EXPECT_EQ(0, nrows, f"no rows inserted to sandbox{inserter.sbindex}")


def EXPECT_HAS_INSERTS(inserter, session):
    nrows = session.run_sql(
        f"SELECT count(*) FROM {inserter.table_name}").fetch_one()[0]
    EXPECT_LT(0, nrows, f"rows inserted to sandbox{inserter.sbindex}")


#@<> Setup
testutil.deploy_sandbox(__mysql_sandbox_port1, "root",
                        {"report_host": hostname})
testutil.deploy_sandbox(__mysql_sandbox_port2, "root",
                        {"report_host": hostname})
testutil.deploy_sandbox(__mysql_sandbox_port3, "root",
                        {"report_host": hostname})
testutil.deploy_sandbox(__mysql_sandbox_port4, "root",
                        {"report_host": hostname})
testutil.deploy_sandbox(__mysql_sandbox_port5, "root",
                        {"report_host": hostname})
testutil.deploy_sandbox(__mysql_sandbox_port6, "root",
                        {"report_host": hostname})

session1 = mysql.get_session(__sandbox_uri1)
session2 = mysql.get_session(__sandbox_uri2)
session3 = mysql.get_session(__sandbox_uri3)
session4 = mysql.get_session(__sandbox_uri4)
session5 = mysql.get_session(__sandbox_uri5)
session6 = mysql.get_session(__sandbox_uri6)

# make all SBs (except sb1) read-only
for s in [session2, session3, session4, session5, session6]:
    s.run_sql("set persist super_read_only=1")

# start a thread per sandbox and make all of them try to insert stuff at the same time
thds = []
thds2 = []
for sbi, uri in enumerate([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3, __sandbox_uri4, __sandbox_uri5, __sandbox_uri6]):
    thds.append(Inserter(sbi+1, uri, "sink1"))
    #thds2.append(Inserter(sbi+1, uri, "sink2"))
    # prepare using session from primary
    thds[sbi].prepare(thds[0].session)
    #thds2[sbi].prepare(thds2[0].session)


shell.connect(__sandbox_uri1)
c1 = dba.create_cluster(
    "cluster1", {"gtidSetIsComplete": 1, "ipAllowlist": "127.0.0.1," + hostname_ip})
c1.add_instance(__sandbox_uri2, {"ipAllowlist": "127.0.0.1," + hostname_ip})
c1.add_instance(__sandbox_uri3, {"ipAllowlist": "127.0.0.1," + hostname_ip})

cs = c1.create_cluster_set("cs")
c2 = cs.create_replica_cluster(__sandbox_uri4, "cluster2", {
                               "ipAllowlist": "127.0.0.1," + hostname_ip})
c2.add_instance(__sandbox_uri5, {"ipAllowlist": "127.0.0.1," + hostname_ip})

c3 = cs.create_replica_cluster(__sandbox_uri6, "cluster3", {
                               "ipAllowlist": "127.0.0.1," + hostname_ip})

logs = SQLLogAnalyzer(
    [__mysql_sandbox_port1, __mysql_sandbox_port4, __mysql_sandbox_port6])


for thd in thds + thds2:
    thd.start()

s = cs.status({"extended": 1})
EXPECT_EQ("OK", s["clusters"]["cluster2"]["transactionSetConsistencyStatus"])
EXPECT_EQ("OK", s["clusters"]["cluster3"]["transactionSetConsistencyStatus"])

logs.read()

#@<> check that no bogus inserts happened during clusterset setup
EXPECT_HAS_INSERTS(thds[0], session1)
EXPECT_NO_INSERTS(thds[1], session2)
EXPECT_NO_INSERTS(thds[2], session3)
EXPECT_NO_INSERTS(thds[3], session4)
EXPECT_NO_INSERTS(thds[4], session5)
EXPECT_NO_INSERTS(thds[5], session6)

#@<> switch to cluster2
logs.discard()

s = cs.status({"extended": 1})
EXPECT_EQ("OK", s["clusters"]["cluster2"]["transactionSetConsistencyStatus"])
EXPECT_EQ("OK", s["clusters"]["cluster3"]["transactionSetConsistencyStatus"])
cs.set_primary_cluster("cluster2")

logs.read()
logs.filter(r"SELECT.*FROM|SELECT (gtid_sub|@@)|SET (SESSION|@)|SHOW|.*testdb.*|BEGIN|START TRANSACT|COMMIT|.*@@hostname|.*@@server_uuid")
print(logs)

# check order of operations
#@<> cluster2: ensure it's impossible for 2 instances to be R/W at the same time
CHECK_SRO_CLEAR_AT_PROMOTED(logs, __mysql_sandbox_port4, __mysql_sandbox_port1)

#@<> cluster2: ensure that there's a sync on promoted before FTWRL is attempted, to minimize sync wait while FTWRL is held
EXPECT_SYNC_AT_PROMOTED_BEFORE_FTWRL(
    logs, __mysql_sandbox_port4, __mysql_sandbox_port1)

#@<> cluster2: ensure no GR changes during FTWRL
EXPECT_NO_GR_OPS_DURING_FTWRL(logs)

#@<> switch to cluster1
logs.discard()

s = cs.status({"extended": 1})
EXPECT_EQ("OK", s["clusters"]["cluster1"]["transactionSetConsistencyStatus"])
EXPECT_EQ("OK", s["clusters"]["cluster3"]["transactionSetConsistencyStatus"])

cs.set_primary_cluster("cluster1")

logs.read()
logs.filter(r"SELECT.*FROM|SELECT (gtid_sub|@@)|SET (SESSION|@)|SHOW|.*testdb.*|BEGIN|START TRANSACT|COMMIT|.*@@hostname|.*@@server_uuid")
print(logs)

#@<> cluster1: ensure it's impossible for 2 instances to be R/W at the same time
CHECK_SRO_CLEAR_AT_PROMOTED(logs, __mysql_sandbox_port1, __mysql_sandbox_port4)

#@<> cluster1: ensure that there's a sync on promoted before FTWRL is attempted, to minimize sync wait while FTWRL is held
EXPECT_SYNC_AT_PROMOTED_BEFORE_FTWRL(
    logs, __mysql_sandbox_port1, __mysql_sandbox_port4)


#@<> switch back and forth a few times
s = cs.status({"extended": 1})
EXPECT_EQ("OK", s["clusters"]["cluster2"]["transactionSetConsistencyStatus"])
EXPECT_EQ("OK", s["clusters"]["cluster3"]["transactionSetConsistencyStatus"])

cs.set_primary_cluster("cluster2")

s = cs.status({"extended": 1})
EXPECT_EQ("OK", s["clusters"]["cluster1"]["transactionSetConsistencyStatus"])
EXPECT_EQ("OK", s["clusters"]["cluster3"]["transactionSetConsistencyStatus"])

cs.set_primary_cluster("cluster1")

#@<> check for errant trxs

# stop threads
for thd in thds + thds2:
    thd.stop()

EXPECT_HAS_INSERTS(thds[0], session1)
EXPECT_NO_INSERTS(thds[1], session2)
EXPECT_NO_INSERTS(thds[2], session3)
EXPECT_HAS_INSERTS(thds[3], session4)
EXPECT_NO_INSERTS(thds[4], session5)
EXPECT_NO_INSERTS(thds[5], session6)

if thds2:
    EXPECT_HAS_INSERTS(thds2[0], session1)
    EXPECT_NO_INSERTS(thds2[1], session2)
    EXPECT_NO_INSERTS(thds2[2], session3)
    EXPECT_HAS_INSERTS(thds2[3], session4)
    EXPECT_NO_INSERTS(thds2[4], session5)
    EXPECT_NO_INSERTS(thds2[5], session6)

#@<> check what happens during dryRun
logs.read()
logs.discard()

cs.set_primary_cluster("cluster2", {"dryRun": 1})

logs.read()
logs.filter(r"SELECT.*FROM|SELECT (gtid_sub|@@)|SET (SESSION|@)|SHOW|.*testdb.*|BEGIN|START TRANSACT|COMMIT|.*@@hostname|.*@@server_uuid")
print(logs)

#@<> Destroy
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.destroy_sandbox(__mysql_sandbox_port3)
testutil.destroy_sandbox(__mysql_sandbox_port4)
testutil.destroy_sandbox(__mysql_sandbox_port5)
testutil.destroy_sandbox(__mysql_sandbox_port6)

import zipfile
import yaml
import os
import re
import shutil

#@<> INCLUDE diags_common.inc

#@<> Init
testutil.deploy_sandbox(__mysql_sandbox_port1, "root")

session1 = mysql.get_session(__sandbox_uri1)

session1.run_sql("create user nothing@'%'")

session1.run_sql("create user selectonly@'%'")
session1.run_sql("grant select on *.* to selectonly@'%'")

session1.run_sql("create user minimal@'%'")
session1.run_sql("grant select, process, replication client, replication slave on *.* to minimal@'%'")
session1.run_sql("grant execute, select, create temporary tables on sys.* to minimal@'%'")

session1.run_sql("create schema test")

# TODO replace localhost connections with 127.0.0.1 except where system stuff is wanted

#@<> no session - TSFR_1_2_1
def check(outpath):
    if "_hl" in outpath:
        EXPECT_STDOUT_CONTAINS("Shell must be connected to the MySQL server to be diagnosed.")
    elif "_sq" in outpath:
        EXPECT_STDOUT_CONTAINS("Shell must be connected to a MySQL server.")
    else:
        EXPECT_STDOUT_CONTAINS("Shell must be connected to a member of the desired MySQL topology.")

CHECK_ALL_ERROR(check, uri=None)

#@<> invalid option
def check(outpath):
    if "_sq" in outpath:
        EXPECT_STDOUT_CONTAINS("Invalid options at Argument #3: innodb_mutex")
    else:
        EXPECT_STDOUT_CONTAINS("Invalid options at Argument #2: innodb_mutex")
    EXPECT_NO_FILE(outpath)

CHECK_ALL_ERROR(check, {"innodb_mutex":1})

#@<> invalid option - slowQuery

outpath = run_collect_sq(__sandbox_uri1, None, "", {"innodb_mutex":1})
EXPECT_STDOUT_CONTAINS("debug.collectSlowQueryDiagnostics: Invalid options at Argument #3: innodb_mutex")
EXPECT_NO_FILE(outpath)

RESET(outpath)

# TSFR_9_0_14
outpath = run_collect_sq(__sandbox_uri1, None, {})
EXPECT_STDOUT_CONTAINS("debug.collectSlowQueryDiagnostics: Argument #2 is expected to be a string")
EXPECT_NO_FILE(outpath)

#@<> invalid option - pfsInstrumentation garbage TSFR_6_1, TSFR_6_2
def check(outpath):
    EXPECT_STDOUT_CONTAINS("'pfsInstrumentation' must be one of current, medium, full")

CHECK_ALL_ERROR(check, {"pfsInstrumentation":""}, nobasic=True)

CHECK_ALL_ERROR(check, {"pfsInstrumentation":"nwelqeq"}, nobasic=True)

def check(outpath):
    EXPECT_STDOUT_CONTAINS("option 'pfsInstrumentation' is expected to be a string")

CHECK_ALL_ERROR(check, {"pfsInstrumentation":{}}, nobasic=True)

#@<> ensure copy of log file with non-utf8 data works
# Bug#34208308	util.debug.collectDiagnostics() fails if a log file includes non-utf8 character

# inject some binary junk into the log
shell.log("info", b"test\xed\xf0")

outpath = run_collect(__sandbox_uri1, None)
EXPECT_FILE_CONTENTS(outpath, "mysqlsh.log", b"test\xed\xf0")

#@<> BUG#34048754 - 'path' set to an empty string should result in an error - all
# TSFR_1_3_3
def check(outpath):
    EXPECT_STDOUT_CONTAINS("'path' cannot be an empty string")
    EXPECT_NO_FILE(".zip")

CHECK_ALL_ERROR(check, path="")

#@<> bogus value for path TSFR_1_1_2, TSFR_9_0_2
def check(outpath):
    EXPECT_STDOUT_CONTAINS("Argument #1 is expected to be a string")

CHECK_ALL_ERROR(check, path={})

#@<> TSFR_1_3_2 - bad dir
CHECK_ALL_ERROR(lambda out: EXPECT_STDOUT_CONTAINS("No such file or directory"), path=outdir+"/invalid/out.zip", keep_file=True)

#@<> TSFR_1_3_2 - not writable
os.mkdir(outdir+"/unwritable", mode=0o555)
CHECK_ALL_ERROR(lambda out: EXPECT_STDOUT_CONTAINS("Permission denied"), path=outdir+"/unwritable/out.zip", keep_file=True)
os.rmdir(outdir+"/unwritable")

#@<> TSFR_1_3_2 Duplicate filename - all
outpath = outdir+"/out.zip"
open(outpath, "w").close()
os.chmod(outpath, 0o600)
CHECK_ALL_ERROR(lambda out: EXPECT_STDOUT_CONTAINS("already exists"), path=outpath, keep_file=True)

# TSFR_1_3_5 - out should expand to out.zip
outpath = outdir+"/out"
open(outpath, "w").close()
os.chmod(outpath, 0o600)
CHECK_ALL_ERROR(lambda out: EXPECT_STDOUT_CONTAINS("already exists"), path=outpath, keep_file=True)

#@<> TSFR_1_4_1 - bad value for iterations
outpath = run_collect_hl(hostname_uri, None, {"iterations":"bla"})
EXPECT_STDOUT_CONTAINS("Argument #2, option 'iterations' is expected to be an integer")
RESET(outpath)

# TSFR_1_4_2
outpath = run_collect_hl(hostname_uri, None, {"iterations":0})
EXPECT_STDOUT_CONTAINS("'iterations' must be > 0")
RESET(outpath)

# TSFR_1_4_2
outpath = run_collect_hl(hostname_uri, None, {"iterations":-1})
EXPECT_STDOUT_CONTAINS("'iterations' must be > 0")
RESET(outpath)

# TSFR_1_4_8
outpath = run_collect_hl(hostname_uri, None, {"delay":0})
EXPECT_STDOUT_CONTAINS("'delay' must be > 0")
RESET(outpath)

# TSFR_1_4_8
outpath = run_collect_hl(hostname_uri, None, {"delay":-1})
EXPECT_STDOUT_CONTAINS("'delay' must be > 0")
RESET(outpath)

# TSFR_1_4_9
outpath = run_collect_hl(hostname_uri, None, {"delay":{}})
EXPECT_STDOUT_CONTAINS("'delay' is expected to be an integer")

#@<> bogus value for options TSFR_1_1_3, TSFR_9_0_3, TSFR_9_0_4
args = [hostname_uri, "--passwords-from-stdin", "-e", "util.debug.collectDiagnostics('x', false)"]
testutil.call_mysqlsh(args, "\n", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])

EXPECT_STDOUT_CONTAINS("Argument #2 is expected to be a map")
RESET()

args = [hostname_uri, "--passwords-from-stdin", "-e", "util.debug.collectHighLoadDiagnostics('x', false)"]
testutil.call_mysqlsh(args, "\n", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])

EXPECT_STDOUT_CONTAINS("Argument #2 is expected to be a map")
RESET()

args = [hostname_uri, "--passwords-from-stdin", "-e", "util.debug.collectSlowQueryDiagnostics('x', 'x', false)"]
testutil.call_mysqlsh(args, "\n", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])

EXPECT_STDOUT_CONTAINS("Argument #3 is expected to be a map")

RESET()

args = [hostname_uri, "--passwords-from-stdin", "-e", "util.debug.collectSlowQueryDiagnostics('x', {}, false)"]
testutil.call_mysqlsh(args, "\n", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])

EXPECT_STDOUT_CONTAINS("Argument #2 is expected to be a string")


#@<> Regular instance - basic
shell.connect(__sandbox_uri1)

outpath = run_collect(__sandbox_uri1, None)
# allMembers is true but it's a standalone instance, so it's a no-op
CHECK_DIAGPACK(outpath, [(0, session1)], is_cluster=False, allMembers=1, innodbMutex=False, localTarget=True)

#@<> output is a directory, default filename with timestamp - all
# TSFR_1_3_4
run_collect(hostname_uri, outdir+"/")
filenames = [f for f in os.listdir(outdir) if f.startswith("mysql-diagnostics-")]
EXPECT_EQ(1, len(filenames))

RESET(filenames[0])

run_collect_hl(hostname_uri, outdir+"/")
filenames = [f for f in os.listdir(outdir) if f.startswith("mysql-diagnostics-")]
EXPECT_EQ(1, len(filenames))

EXPECT_STDOUT_CONTAINS("NOTE: Target server is not at localhost, host information was not collected")

RESET(filenames[0])

run_collect_sq(hostname_uri+"/mysql", outdir+"/", "select * from mysql.user")
filenames = [f for f in os.listdir(outdir) if f.startswith("mysql-diagnostics-")]
EXPECT_EQ(1, len(filenames))

EXPECT_STDOUT_CONTAINS("NOTE: Target server is not at localhost, host information was not collected")

#@<> slowQueries fail - basic
outpath = run_collect(hostname_uri, None, {"slowQueries":1})
EXPECT_STDOUT_CONTAINS("slowQueries option requires slow_query_log to be enabled and log_output to be set to TABLE")
EXPECT_NO_FILE(outpath)

#@<> slowQueries OK
session.run_sql("set global slow_query_log=1")
session.run_sql("set session long_query_time=0.1")
session.run_sql("set global log_output='TABLE'")
session.run_sql("select sleep(1)")
outpath = run_collect(hostname_uri, None, {"slowQueries":1})
CHECK_DIAGPACK(outpath, [(0, session1)], is_cluster=False, innodbMutex=False, slowQueries=True)

#@<> with no access - all - TSFR_1_2_4
outpath = run_collect(f"nothing:@{hostname}:{__mysql_sandbox_port1}", None)
EXPECT_STDOUT_CONTAINS("Access denied for user 'nothing'@'%'")
EXPECT_NO_FILE(outpath)

RESET(outpath)

outpath = run_collect_hl(f"nothing:@{hostname}:{__mysql_sandbox_port1}", None)
EXPECT_STDOUT_CONTAINS("Access denied for user 'nothing'@'%'")
EXPECT_NO_FILE(outpath)

RESET(outpath)

outpath = run_collect_sq(f"nothing:@{hostname}:{__mysql_sandbox_port1}", None, "select 1")
EXPECT_STDOUT_CONTAINS("Access denied for user 'nothing'@'%'")
EXPECT_NO_FILE(outpath)

#@<> no access + ignoreErrors
# not enough for anything, so error out
outpath = run_collect(f"nothing:@{hostname}:{__mysql_sandbox_port1}", None, {"allMembers":0, "ignoreErrors":1})
EXPECT_STDOUT_CONTAINS("Access denied for user 'nothing'@'%'")
EXPECT_NO_FILE(outpath)

#@<> with select only access
outpath = run_collect(f"selectonly:@{hostname}:{__mysql_sandbox_port1}", None)
EXPECT_STDOUT_CONTAINS("execute command denied to user 'selectonly'@'%' for routine")
EXPECT_NO_FILE(outpath)

#@<> minimal privs
outpath = run_collect(f"minimal:@{hostname}:{__mysql_sandbox_port1}", None)
CHECK_DIAGPACK(outpath, [(0, session1)], is_cluster=False, innodbMutex=False)

#@<> Regular instance + innodbMutex + schemaStatus
outpath = run_collect(hostname_uri, None, innodbMutex=1, schemaStats=1, allMembers=1)
CHECK_DIAGPACK(outpath, [(0, session1)], is_cluster=False, innodbMutex=True, schemaStats=True)
EXPECT_STDOUT_CONTAINS("NOTE: allMembers enabled, but InnoDB Cluster metadata not found")

#@<> customSql - bad value - TSFR_1_5_1
def check(outpath):
    EXPECT_STDOUT_CONTAINS("option 'customSql' is expected to be an array")
    EXPECT_NO_FILE(outpath)

CHECK_ALL_ERROR(check, {"customSql":"select 1"})

outpath = run_collect(__sandbox_uri1, None, {"customSql": ["during:select 1"]})
EXPECT_STDOUT_CONTAINS("Option 'customSql' may not contain before:, during: or after: prefixes")
EXPECT_NO_FILE(outpath)

#@<> customShell - bad value - TSFR_1_6_1
def check(outpath):
    EXPECT_STDOUT_CONTAINS("option 'customShell' is expected to be an array")
    EXPECT_NO_FILE(outpath)

CHECK_ALL_ERROR(check, {"customShell":"true"})

outpath = run_collect(__sandbox_uri1, None, {"customShell": ["during:echo 1"]})
EXPECT_STDOUT_CONTAINS("Option 'customShell' may not contain before:, during: or after: prefixes")
EXPECT_NO_FILE(outpath)

#@<> customShell while connected to not localhost - TSFR_1_6_6
def check(outpath):
    EXPECT_STDOUT_CONTAINS("Option 'customShell' is only allowed when connected to localhost")
    EXPECT_NO_FILE(outpath)

CHECK_ALL_ERROR(check, {"customShell": ["echo 1"]}, uri=f"root:root@{hostname}:{__mysql_sandbox_port1}")

#@<> customSql and customShell - success - TSFR_1_6_2, TSFR_1_6_4
def check(outpath):
    if "diag_hl" in outpath or "diag_sq" in outpath:
        csfn = "custom_shell-before.txt"
        cqfn = "custom_sql-before-script"
    else:
        csfn = "custom_shell.txt"
        cqfn = "0.custom_sql-script"
    EXPECT_FILE_CONTENTS(outpath, csfn, b"^custom script$")
    EXPECT_FILE_CONTENTS(outpath, csfn, b"^custom script2$")
    EXPECT_FILE_CONTENTS(outpath, f"{cqfn}_0.tsv", b"^CUSTOM SQL$")
    EXPECT_FILE_CONTENTS(outpath, f"{cqfn}_1.tsv", b"^CUSTOM SQL2$")

CHECK_ALL(check, {"customShell": ['echo "custom script"',
                                'echo "custom script2"',
                                'false || true'],
                                      "customSql":["select upper('custom sql')",
                                                    "select upper('custom sql2')"]},
        uri=__sandbox_uri1)

#@<> customSql and customShell - errors in the scripts

# should abort
def check(outpath):
    EXPECT_STDOUT_CONTAINS("While executing \"select invalid\"")
    EXPECT_NO_FILE(outpath)

CHECK_ALL_ERROR(check, {"customSql":["select invalid"]})

def check(outpath):
    EXPECT_STDOUT_CONTAINS("Shell command \"false\" exited with code 1")
    EXPECT_NO_FILE(outpath)

# TSFR_1_5_2 TSFR_1_6_3
CHECK_ALL_ERROR(check, {"customShell": ['echo "custom script"',
                                        'echo "custom script2"',
                                        'false']}, uri=__sandbox_uri1)


def check(outpath):
    if "_hq" in outpath or "_sl" in outpath:
        EXPECT_STDOUT_CONTAINS("You have an error in your SQL syntax")
    EXPECT_NO_FILE(outpath)

CHECK_ALL_ERROR(check, {"customSql":["THIS WILL CAUSE AN ERROR",
                                     "BefoRe:THIS WILL CAUSE AN ERROR",
                                     "DURING:THIS WILL CAUSE AN ERROR"]}, 
                uri=__sandbox_uri1)

#@<> customSql and customShell - before/during/after - TSFR_1_6_4, TSFR_1_6_7, TSFR_13_1, TSFR_13_2
options = {
    "customSql": ["before:select 'before'", "during:select 'during'", "during:select 'during2'", "after:select 'after'"],
    "customShell": ["before:echo 'before'", "during:echo 'during'", "during:echo 'during2'", "after:echo 'after'", "echo $bla"]
}
outpath = run_collect_hl(__sandbox_uri1, None, options, env=["bla=inherited"])
EXPECT_FILE_CONTENTS(outpath, "custom_shell-before.txt", b"^before$")
EXPECT_FILE_CONTENTS(outpath, "custom_shell-before.txt", b"^inherited$")
EXPECT_FILE_CONTENTS(outpath, "custom_shell-iteration0.txt", b"^during$")
EXPECT_FILE_CONTENTS(outpath, "custom_shell-iteration0.txt", b"^during2$")
EXPECT_FILE_CONTENTS(outpath, "custom_shell-after.txt", b"^after$")
EXPECT_FILE_CONTENTS(outpath, "custom_sql-before-script_0.tsv", b"^before$")
EXPECT_FILE_CONTENTS(outpath, "custom_sql-iteration0-script_0.tsv", b"^during$")
EXPECT_FILE_CONTENTS(outpath, "custom_sql-iteration0-script_1.tsv", b"^during2$")
EXPECT_FILE_CONTENTS(outpath, "custom_sql-after-script_0.tsv", b"^after$")
RESET(outpath)

outpath = run_collect_sq(__sandbox_uri1, None, "select 123", options, env=["bla=inherited"])
EXPECT_FILE_CONTENTS(outpath, "custom_shell-before.txt", b"^before$")
EXPECT_FILE_CONTENTS(outpath, "custom_shell-before.txt", b"^inherited$")
EXPECT_FILE_CONTENTS(outpath, "custom_shell-iteration0.txt", b"^during$")
EXPECT_FILE_CONTENTS(outpath, "custom_shell-iteration0.txt", b"^during2$")
EXPECT_FILE_CONTENTS(outpath, "custom_shell-after.txt", b"^after$")
EXPECT_FILE_CONTENTS(outpath, "custom_sql-before-script_0.tsv", b"^before$")
EXPECT_FILE_CONTENTS(outpath, "custom_sql-iteration0-script_0.tsv", b"^during$")
EXPECT_FILE_CONTENTS(outpath, "custom_sql-iteration0-script_1.tsv", b"^during2$")
EXPECT_FILE_CONTENTS(outpath, "custom_sql-after-script_0.tsv", b"^after$")
RESET(outpath)

#@<> customSql - TSFR_1_5_3
options = {
    "customSql": [
        "CREATE TABLE IF NOT EXISTS test.preamble(execution TIMESTAMP(6))",
        "DURING:INSERT INTO test.preamble(execution) VALUES (NOW(6))"
    ],
    "delay": 2
}
def check(outpath):
    if __version_num >= 80000:
        count, dif = session1.run_sql("select count(*), cast(max(execution)-min(execution) as float) from test.preamble").fetch_one()
    else:
        count, dif = session1.run_sql("select count(*), cast(max(execution)-min(execution) as signed) from test.preamble").fetch_one()
    EXPECT_EQ(2, count, outpath+" execution count")
    EXPECT_GT(dif, 1, outpath)
    # this test is not deterministic
    # EXPECT_LT(dif, 4, outpath) # ensure iterations are spaced by more than the specified delay, but not too much more
    if dif > 4:
        print(f"WARNING: time between iterations > 4??? ({dif})")
    session1.run_sql("drop table test.preamble")

CHECK_ALL(check, options, nobasic=True, query="select sleep(4)")

#@<> customSql - TSFR_1_5_4
options = {
    "customSql": [
        "before:CREATE TABLE IF NOT EXISTS test.preamble(execution char(64))",
        "during:INSERT INTO test.preamble(execution) VALUES ('during')",
        "after:INSERT INTO test.preamble(execution) VALUES ('after') "
    ],
    "delay": 2
}
def check(outpath):
    rows = session1.run_sql("select * from test.preamble").fetch_all()
    EXPECT_EQ(3, len(rows))
    EXPECT_EQ("during", rows[0][0])
    EXPECT_EQ("during", rows[1][0])
    EXPECT_EQ("after", rows[2][0])
    session1.run_sql("drop table test.preamble")

CHECK_ALL(check, options, nobasic=True, query="select sleep(4)")

#@<> customSql - TSFR_1_5_5
options = {
    "customSql": [
        "SHOW VARIABLES",
        "before:SHOW VARIABLES",
        "during:SHOW VARIABLES",
        "after:SHOW VARIABLES"
    ],
    "delay": 1
}
def check(outpath):
    EXPECT_FILE_CONTENTS(outpath, "custom_sql-before-script_0.tsv", b"SHOW VARIABLES")
    EXPECT_FILE_CONTENTS(outpath, "custom_sql-before-script_1.tsv", b"SHOW VARIABLES")
    EXPECT_FILE_CONTENTS(outpath, "custom_sql-iteration0-script_0.tsv", b"SHOW VARIABLES")
    EXPECT_FILE_CONTENTS(outpath, "custom_sql-iteration1-script_0.tsv", b"SHOW VARIABLES")
    EXPECT_FILE_CONTENTS(outpath, "custom_sql-after-script_0.tsv", b"SHOW VARIABLES")

# TSFR_1_4_7
CHECK_ALL(check, options, nobasic=True, query="select sleep(4)")

#@<> plain highLoad

# TSFR_1_4_4
def check(outpath):
    CHECK_DIAGPACK(outpath, [(None, session1)], allMembers=0, schemaStats=True, slowQueries=True)
    EXPECT_FILE_IN_ZIP(outpath, "diagnostics-raw/iteration-1.metrics.tsv")
    EXPECT_FILE_IN_ZIP(outpath, "diagnostics-raw/iteration-2.metrics.tsv")
    EXPECT_FILE_IN_ZIP(outpath, "diagnostics-raw/iteration-3.metrics.tsv")
    EXPECT_FILE_NOT_IN_ZIP(outpath, "diagnostics-raw/iteration-4.metrics.tsv")

outpath = run_collect_hl(hostname_uri, None, {"iterations":3, "delay":1})
check(outpath)

#@<> plain all with innodbMutex

# TSFR_1_4_5
def check(outpath):
    CHECK_DIAGPACK(outpath, [(None, session1)], innodbMutex=True, allMembers=0, schemaStats=True, slowQueries=True)
    EXPECT_FILE_IN_ZIP(outpath, "diagnostics-raw/iteration-1.metrics.tsv")
    EXPECT_FILE_IN_ZIP(outpath, "diagnostics-raw/iteration-2.metrics.tsv")
    EXPECT_FILE_NOT_IN_ZIP(outpath, "diagnostics-raw/iteration-3.metrics.tsv")

outpath = run_collect_hl(hostname_uri, None, {"delay":1, "innodbMutex":1})
check(outpath)

#@<> highLoad + slowQuery with pfsInstrumentation - current

if __version_num > 80000:
    default_consumers = ["events_statements_current", "events_statements_history", "events_transactions_current", "events_transactions_history", "global_instrumentation", "thread_instrumentation", "statements_digest"]
else:
    default_consumers = ["events_statements_current", "events_statements_history", "global_instrumentation", "thread_instrumentation", "statements_digest"]

def check(outpath):
    CHECK_DIAGPACK(outpath, [(None, session1)], allMembers=0, schemaStats=True, slowQueries=True)
    CHECK_PFS_INSTRUMENTS(outpath, "current", {"stage/mysys": lambda x: x == 0,
                                        "wait/synch":lambda x: x == 0,
                                        "stage/sql": lambda x: 0 < x < 5,
                                        "wait/io": lambda x: 80 < x < 90 })
    CHECK_PFS_CONSUMERS(outpath, "current", default_consumers)

CHECK_ALL(check, {"delay":1, "pfsInstrumentation":"current"}, nobasic=True)

#@<> highLoad + slowQuery with pfsInstrumentation - medium
if __version_num > 80000:
    consumers = ["events_stages_current", "events_statements_cpu", "events_statements_current", "events_statements_history", "events_transactions_current", "events_transactions_history", "events_waits_current", "global_instrumentation", "thread_instrumentation", "statements_digest"]
else:
    consumers = ["events_stages_current", "events_statements_current", "events_statements_history", "events_transactions_current", "events_waits_current", "global_instrumentation", "thread_instrumentation", "statements_digest"]
def check(outpath):
    CHECK_PFS_INSTRUMENTS(outpath, "medium", {"wait/synch":lambda x: x == 0})
    CHECK_PFS_CONSUMERS(outpath, "medium", consumers)

CHECK_ALL(check, {"delay":1, "pfsInstrumentation":"medium"}, nobasic=True)

#@<> highLoad + slowQuery with pfsInstrumentation - full
# TSFR_6_1_1

def check(outpath):
    CHECK_PFS_INSTRUMENTS(outpath, "full", {})
    CHECK_PFS_CONSUMERS(outpath, "full", ["events_stages_current", "events_stages_history", "events_stages_history_long", "events_statements_cpu", "events_statements_current","events_statements_history","events_statements_history_long","events_transactions_current","events_transactions_history","events_transactions_history_long","events_waits_current","events_waits_history","events_waits_history_long","global_instrumentation","thread_instrumentation","statements_digest"])

CHECK_ALL(check, {"delay":1, "pfsInstrumentation":"full"}, nobasic=True)


#@<> highLoad with pfsInstrumentation current and all disabled instruments/consumers
# TSFR_6_8 (is invalid) TSFR_6_9 TSFR_6_10
CHECK_ALL(check, {"delay":1, "pfsInstrumentation":"current"}, nobasic=True)

session1.run_sql("call sys.ps_setup_reset_to_default(true)")

session1.run_sql("update performance_schema.setup_consumers set enabled='NO'")

def check(outpath):
    EXPECT_STDOUT_CONTAINS("WARNING: performance_schema.setup_consumers is completely disabled")

CHECK_ALL(check, {"delay":1, "pfsInstrumentation":"current"}, nobasic=True)

session1.run_sql("call sys.ps_setup_reset_to_default(true)")

#@<> highLoad with pfsInstrumentation current and all disabled threads {VER(>=8.0.0)}
session1.run_sql("update performance_schema.setup_threads set enabled='NO'")

def check(outpath):
    EXPECT_STDOUT_CONTAINS("WARNING: performance_schema.setup_threads is completely disabled")

CHECK_ALL(check, {"delay":1, "pfsInstrumentation":"current"}, nobasic=True)

session1.run_sql("call sys.ps_setup_reset_to_default(true)")

#@<> no pfs TSFR_6_3
testutil.deploy_sandbox(__mysql_sandbox_port5, "root", {"performance-schema":"off"})

def check(outpath):
    EXPECT_STDOUT_CONTAINS("WARNING: performance_schema is disabled, collected a limited amount of information")

CHECK_ALL(check, uri=__sandbox_uri5)

#@<> plain slowQuery

outpath = run_collect_sq(__sandbox_uri1, None, "select * from mysql.user join mysql.db", {"delay":1})
CHECK_DIAGPACK(outpath, [(None, session1)], innodbMutex=False, allMembers=0, schemaStats=True, slowQueries=True, localTarget=True)

#@<> invalid slowQuery - TSFR_9_0_15
outpath = run_collect_sq(__sandbox_uri1, None, "drop schema information_schema")
EXPECT_STDOUT_CONTAINS("ERROR running query:  EXPLAIN drop schema information_schema MySQL Error (1064):")
EXPECT_STDOUT_CONTAINS("Access denied for user 'root'@'localhost' to database 'information_schema' (MySQL Error 1044)")
EXPECT_NO_FILE(outpath)
RESET(outpath)

outpath = run_collect_sq(__sandbox_uri1, None, "garbage")
EXPECT_STDOUT_CONTAINS("ERROR running query:  EXPLAIN garbage MySQL Error (1046)")
EXPECT_STDOUT_CONTAINS("You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near 'garbage' at line 1 (MySQL Error 1064)")
EXPECT_NO_FILE(outpath)
RESET(outpath)

outpath = run_collect_sq(__sandbox_uri1, None, "")
EXPECT_STDOUT_CONTAINS("debug.collectSlowQueryDiagnostics: 'query' must contain the query to be analyzed")
EXPECT_NO_FILE(outpath)

#@<> Cleanup
session1.close()

testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port5)

shutil.rmtree(outdir)

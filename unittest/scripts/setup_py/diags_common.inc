import os

outdir = __tmp_dir+"/diagout"
if os.path.exists(outdir):
    shutil.rmtree(outdir)
os.makedirs(outdir)

# connect via not-localhost so that slow system collection isn't always executed
hostname_uri = f"mysql://root:root@{hostname}:{__mysql_sandbox_port1}"


def zfpath(zf, fn):
    return os.path.basename(zf.filename).rsplit(".", 1)[0]+"/"+fn

def read_from_zip(zfn, fn, strip_comments=True):
    with zipfile.ZipFile(zfn, mode="r") as zf:
        with zf.open(zfpath(zf, fn), "r") as f:
            text = f.read().decode("utf-8")
            if strip_comments:
                start = text.rfind("\n#")
                return text[start+1:].partition("\n")[-1]
            else:
                return text

def tsv_from_zip(zfn, fn):
    text = read_from_zip(zfn, fn, strip_comments=True)
    out = []
    for line in text.split("\n"):
        if line:
            out.append(line.split("\t"))
    return out

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

def EXPECT_FILE_IN_ZIP(fn, path):
    with zipfile.ZipFile(fn, mode="r") as zf:
        ZEXPECT_EXISTS(zf, path)

def EXPECT_FILE_NOT_IN_ZIP(fn, path):
    with zipfile.ZipFile(fn, mode="r") as zf:
        ZEXPECT_NOT_EXISTS(zf, path)

def EXPECT_FILE_CONTENTS(fn, path, pattern):
    with zipfile.ZipFile(fn, mode="r") as zf:
        ZEXPECT_EXISTS(zf, path)
        with zf.open(zfpath(zf, path), "r") as f:
            contents = f.read()
            EXPECT_TRUE(re.findall(pattern, contents, re.M), f"{pattern} expected in {path} but missing")



def CHECK_PFS_INSTRUMENTS(path, level, limited_instruments):
    for name, enabledpct, timedpct in tsv_from_zip(path, f"diagnostics-applied_{level}-pfs_instruments.tsv"):
        limit = limited_instruments.get("name")
        if limit:
            EXPECT_TRUE(limit(enabledpct), f"{name} out of range - {enabledpct}% enabled")


def CHECK_PFS_CONSUMERS(path, level, enabled_consumers):
    for name, enabled, timed in tsv_from_zip(path, f"diagnostics-applied_{level}-pfs_consumers.tsv"):
        if enabled == "YES":
            EXPECT_TRUE(name in enabled_consumers, f"consumer {name} is {enabled} but isn't expected to be")
        else:
            EXPECT_TRUE(name not in enabled_consumers, f"consumer {name} is {enabled} but is expected to be")


g_expected_files_per_instance = [
    # query, filename, requirements
    ("", "performance_schema.replication_applier_configuration.tsv", {}),
    ("select * from performance_schema.replication_applier_filters", "performance_schema.replication_applier_filters.tsv", {"version":80000}),
    ("select * from performance_schema.replication_applier_global_filters", "performance_schema.replication_applier_global_filters.tsv", {"version":80000}),
    ("", "performance_schema.replication_applier_status.tsv", {}),
    ("", "performance_schema.replication_applier_status_by_coordinator.tsv", {}),
    ("", "performance_schema.replication_applier_status_by_worker.tsv", {}),
    ("select * from performance_schema.replication_asynchronous_connection_failover_managed", "performance_schema.replication_asynchronous_connection_failover_managed.tsv", {"version":80000}),
    ("", "performance_schema.replication_connection_configuration.yaml", {}),
    ("", "performance_schema.replication_connection_status.tsv", {}),
    ("", "performance_schema.replication_group_member_stats.tsv", {}),
    ("select * from performance_schema.replication_group_members", "performance_schema.replication_group_members.tsv", {}),
    ("", "SHOW_BINARY_LOGS.tsv", {}),
    ("", "SHOW_REPLICAS.tsv", {"version":80023}),
    ("", "SHOW_SLAVE_HOSTS.tsv", {"version":-80023}),
    ("", "SHOW_MASTER_STATUS.tsv", {}),
    ("", "sys.processlist.tsv", {"version":80000}),
    ("", "SHOW_FULL_PROCESSLIST.tsv", {"version":-80000}),
    ("", "SHOW_GLOBAL_STATUS.tsv", {}),
    ("", "error_log", {"version":80022}),
    ("", "global_variables.tsv", {}),
    ("", "information_schema.innodb_metrics.tsv", {}),
    ("", "SHOW_ENGINE_INNODB_STATUS.yaml", {}),
    ("", "SHOW_ENGINE_INNODB_MUTEX.yaml", {"innodbMutex":1})
]

g_expected_files_per_instance_high_load = [
    # query, filename, requirements
    ("", "performance_schema.replication_applier_configuration.tsv", {}),
    ("select * from performance_schema.replication_applier_filters", "performance_schema.replication_applier_filters.tsv", {"version":80000}),
    ("select * from performance_schema.replication_applier_global_filters", "performance_schema.replication_applier_global_filters.tsv", {"version":80000}),
    ("", "performance_schema.replication_applier_status.tsv", {}),
    ("", "performance_schema.replication_applier_status_by_coordinator.tsv", {}),
    ("", "performance_schema.replication_applier_status_by_worker.tsv", {}),
    ("select * from performance_schema.replication_asynchronous_connection_failover_managed", "performance_schema.replication_asynchronous_connection_failover_managed.tsv", {"version":80000}),
    ("", "performance_schema.replication_connection_configuration.yaml", {}),
    ("", "performance_schema.replication_connection_status.tsv", {}),
    ("", "performance_schema.replication_group_member_stats.tsv", {}),
    ("select * from performance_schema.replication_group_members", "performance_schema.replication_group_members.tsv", {}),
    ("", "SHOW_BINARY_LOGS.tsv", {}),
    ("", "SHOW_REPLICAS.tsv", {"version":80023}),
    ("", "SHOW_SLAVE_HOSTS.tsv", {"version":-80022}),
    ("", "SHOW_MASTER_STATUS.tsv", {}),
    ("", "diagnostics-metrics.summary.tsv", {}),
    ("", "diagnostics-raw/start.host_summary.tsv", {}),
    ("", "diagnostics-raw/end.host_summary.tsv", {}),
    ("", "diagnostics-raw/iteration-1.metrics.tsv", {}),
    ("", "diagnostics-raw/iteration-1.sys.processlist.tsv", {"version":80000}),
    ("", "diagnostics-raw/iteration-1.SHOW_FULL_PROCESSLIST.tsv", {"version":-80000}),
    ("", "diagnostics-raw/iteration-1.SHOW_GLOBAL_STATUS.tsv", {}),
    ("", "error_log", {"version":80022}),
    ("", "global_variables.tsv", {}),
    ("", "diagnostics-raw/iteration-1.information_schema.innodb_metrics.tsv", {}),
    ("", "diagnostics-raw/iteration-1.SHOW_ENGINE_INNODB_STATUS.yaml", {}),
    ("", "diagnostics-raw/iteration-1.SHOW_ENGINE_INNODB_MUTEX.tsv", {"innodbMutex":1})
]


g_expected_files_per_instance_slow_query = [
    # query, filename, requirements
    ("", "performance_schema.replication_applier_configuration.tsv", {}),
    ("select * from performance_schema.replication_applier_filters", "performance_schema.replication_applier_filters.tsv", {"version":80000}),
    ("select * from performance_schema.replication_applier_global_filters", "performance_schema.replication_applier_global_filters.tsv", {"version":80000}),
    ("", "performance_schema.replication_applier_status.tsv", {}),
    ("", "performance_schema.replication_applier_status_by_coordinator.tsv", {}),
    ("", "performance_schema.replication_applier_status_by_worker.tsv", {}),
    ("select * from performance_schema.replication_asynchronous_connection_failover_managed", "performance_schema.replication_asynchronous_connection_failover_managed.tsv", {"version":80000}),
    ("", "performance_schema.replication_connection_configuration.yaml", {}),
    ("", "performance_schema.replication_connection_status.tsv", {}),
    ("", "performance_schema.replication_group_member_stats.tsv", {}),
    ("select * from performance_schema.replication_group_members", "performance_schema.replication_group_members.tsv", {}),
    ("", "SHOW_BINARY_LOGS.tsv", {}),
    ("", "SHOW_REPLICAS.tsv", {"version":80023}),
    ("", "SHOW_SLAVE_HOSTS.tsv", {"version":-80022}),
    ("", "SHOW_MASTER_STATUS.tsv", {}),
    ("", "diagnostics-metrics.summary.tsv", {}),
    ("", "diagnostics-raw/start.host_summary.tsv", {}),
    ("", "diagnostics-raw/end.host_summary.tsv", {}),
    ("", "diagnostics-raw/iteration-1.metrics.tsv", {}),
    ("", "diagnostics-raw/iteration-1.sys.processlist.tsv", {"version":80000}),
    ("", "diagnostics-raw/iteration-1.SHOW_FULL_PROCESSLIST.tsv", {"version":-80000}),
    ("", "diagnostics-raw/iteration-1.SHOW_GLOBAL_STATUS.tsv", {}),
    ("", "error_log", {"version":80022}),
    ("", "global_variables.tsv", {}),
    ("", "diagnostics-raw/iteration-1.information_schema.innodb_metrics.tsv", {}),
    ("", "diagnostics-raw/iteration-1.SHOW_ENGINE_INNODB_STATUS.yaml", {}),
    ("", "diagnostics-raw/iteration-1.SHOW_ENGINE_INNODB_MUTEX.tsv", {"innodbMutex":1}),
    ("", "explain.tsv", {}),
    ("", "explain_analyze.tsv", {"version":80022}),
    ("", "explain_json.tsv", {}),
    ("", "referenced_table-mysql.user.yaml", {}),
    ("", "referenced_table-mysql.db.yaml", {})
]


def CHECK_QUERY(session, data, query, note=None):
    assert isinstance(data, list)
    all = []
    res = session.run_sql(query)
    #all.append("# "+"\t".join([c.column_label for c in res.columns]))
    for row in iter(res.fetch_one, None):
        line = []
        for i in range(len(row)):
            line.append(str(row[i]) if row[i] is not None else "NULL")
        all.append("\t".join(line))
    all.append("")
    EXPECT_EQ_TEXT(all, data, note)


def CHECK_METADATA(session, path):
    tables = [r[0] for r in session.run_sql("show full tables in mysql_innodb_cluster_metadata").fetch_all() if r[1] == "BASE TABLE" or r[0] == "schema_version"]
    for table in tables:
        data = read_from_zip(path, f"mysql_innodb_cluster_metadata.{table}.tsv", strip_comments=True)
        CHECK_QUERY(session, data.split("\n"), f"select * from mysql_innodb_cluster_metadata.{table}", f"metadata table {table}")


def CHECK_INSTANCE(session, zf, instance_id, expected_files_per_instance, innodbMutex, localTarget):
    for q, fn, options in expected_files_per_instance:
        minver = options.get("version", None)
        if minver and ((minver>0 and __version_num < minver) or (minver<0 and __version_num > -minver)):
            continue
        if options.get("innodbMutex") and not innodbMutex:
            continue
        if fn == "error_log":
            if not localTarget:
                fn += ".tsv"
            elif (instance_id and instance_id > 1):
                fn += ".tsv"
        if instance_id is None:
            prefix = ""
        else:
            prefix = f"{instance_id}."
        with zf.open(zfpath(zf, f"{prefix}{fn}"), "r") as f:
            data = f.read().decode("utf-8").split("\n")
            while data and data[0].startswith("#"):
                last = data[0]
                del data[0]
            if q:
                print(fn)
                CHECK_QUERY(session, data, q, f"{instance_id}: {fn}")


def CHECK_DIAGPACK(file, sessions_or_errors, is_cluster=False, innodbMutex=False, allMembers=1, schemaStats=False, slowQueries=False, localTarget=False):
    EXPECT_EQ(0o600, os.stat(file).st_mode & 0o777, file) # TSFR_1_3_1
    diag_type = "basic"
    expected_files_per_instance = g_expected_files_per_instance[:]
    if "_hl" in file:
        diag_type = "highLoad"
        expected_files_per_instance = g_expected_files_per_instance_high_load[:]
        schemaStats = True
        slowQueries = True
    elif "_sq" in file:
        diag_type = "slowQuery"
        expected_files_per_instance = g_expected_files_per_instance_slow_query[:]
        schemaStats = True
        slowQueries = True
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
        if diag_type == "basic":
            ZEXPECT_NOT_EMPTY(zf, "mysqlsh.log")
        filelist = zf.namelist()
        if is_cluster:
            ZEXPECT_NOT_EMPTY(zf, "cluster_accounts.tsv")
            CHECK_METADATA(sessions_or_errors[0][1], file)
            if allMembers and localTarget:
                ZEXPECT_NOT_EMPTY(zf, "ping.txt")
        else:
            EXPECT_FALSE(zfpath(zf, "cluster_accounts.tsv") in filelist)
        ZEXPECT_EXISTS(zf, "schema_tables_without_a_PK.tsv")
        if schemaStats:
            ZEXPECT_EXISTS(zf, "schema_object_overview.tsv")
            ZEXPECT_EXISTS(zf, "schema_top_biggest_tables.tsv")
        else:
            ZEXPECT_NOT_EXISTS(zf, "schema_object_overview.tsv")
            ZEXPECT_NOT_EXISTS(zf, "schema_top_biggest_tables.tsv")
        first = True
        for instance_id, session_or_error in sessions_or_errors:
            prefix = ""
            if instance_id is not None:
                prefix = f"{instance_id}."
            if __version_num > 80000:
                if slowQueries:
                    ZEXPECT_EXISTS(zf, f"{prefix}slow_log.tsv")
                    ZEXPECT_EXISTS(zf, f"{prefix}slow_log.yaml")
                else:
                    ZEXPECT_NOT_EXISTS(zf, f"{prefix}slow_log.tsv")
                    ZEXPECT_NOT_EXISTS(zf, f"{prefix}slow_log.yaml")
            if not allMembers:
                if first:
                    ZEXPECT_EXISTS(zf, f"{prefix}global_variables.tsv")
                    first = False
                else:
                    ZEXPECT_NOT_EXISTS(zf, f"{prefix}global_variables.tsv")
                    continue
            if zfpath(zf, f"{prefix}connect_error.txt") in filelist:
                EXPECT_IN(session_or_error, read_text(zfpath(zf, f"{prefix}connect_error.txt")))
            else:
                assert type(session_or_error) != str, f"{instance_id} = {session_or_error}"
                CHECK_INSTANCE(session_or_error, zf, instance_id, expected_files_per_instance, innodbMutex=innodbMutex, localTarget=localTarget)

def RESET(outpath=None):
    if outpath:
        outpath = os.path.join(outdir, outpath)
        if os.path.exists(outpath):
            os.remove(outpath)
    WIPE_OUTPUT()





g_fn_serial = 1

def run_collect(uri, path, options={}, *, password="root", **kwargs):
    global g_fn_serial
    if not options:
        options = {}
    options.update(kwargs)
    if path is None:
        path = os.path.join(outdir, f"diag.{g_fn_serial}.zip")
        g_fn_serial += 1
    args = [uri] if uri else []
    args += ["--passwords-from-stdin", "-e", f"util.debug.collectDiagnostics({repr(path)}, {repr(options)})"]
    print(args)
    testutil.call_mysqlsh(args, password+"\n", ["MYSQLSH_TERM_COLOR_MODE=nocolor"], mysqlshrec)
    return path

def run_collect_hl(uri, path, options={}, *, password="root",env=[],**kwargs):
    global g_fn_serial
    if not options:
        options = {}
    options.update(kwargs)
    if "delay" not in options:
        options["delay"] = 1
    if path is None:
        path = os.path.join(outdir, f"diag_hl.{g_fn_serial}.zip")
        g_fn_serial += 1
    args = [uri] if uri else []
    args += ["-e", f"util.debug.collectHighLoadDiagnostics({repr(path)}, {repr(options)})"]
    print(args)
    testutil.call_mysqlsh(args, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]+env, mysqlshrec)
    return path

def run_collect_sq(uri, path, query="select * from mysql.user join mysql.db", options={}, *, password="root", env=[],**kwargs):
    global g_fn_serial
    if not options:
        options = {}
    options.update(kwargs)
    if "delay" not in options:
        options["delay"] = 1
    if path is None:
        path = os.path.join(outdir, f"diag_sq.{g_fn_serial}.zip")
        g_fn_serial += 1
    if isinstance(query, str):
        query = query.replace("'", "''")
    args = [uri] if uri else []
    args += ["-e", f"util.debug.collectSlowQueryDiagnostics({repr(path)}, {repr(query)}, {repr(options)})"]
    print(args)
    testutil.call_mysqlsh(args, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]+env, mysqlshrec)
    return path

def CHECK_ALL_ERROR(check_fn, options={}, uri=hostname_uri+"/mysql", query="select * from mysql.user join mysql.db", path=None, keep_file=False, nobasic=False):
    if not nobasic:
        outpath = run_collect(uri, path, options)
        check_fn(outpath)
        RESET(None if keep_file else outpath)
    outpath = run_collect_hl(uri, path, options)
    check_fn(outpath)
    RESET(None if keep_file else outpath)
    outpath = run_collect_sq(uri, path, query, options)
    check_fn(outpath)
    RESET(None if keep_file else outpath)

def CHECK_ALL(check_fn, options={}, uri=hostname_uri+"/mysql", query="select * from mysql.user join mysql.db", path=None, keep_file=False, nobasic=False):
    if not nobasic:
        outpath = run_collect(uri, path, options)
        check_fn(outpath)
        RESET(None if keep_file else outpath)
    outpath = run_collect_hl(uri, path, options)
    check_fn(outpath)
    RESET(None if keep_file else outpath)
    outpath = run_collect_sq(uri, path, query, options)
    check_fn(outpath)
    RESET(None if keep_file else outpath)

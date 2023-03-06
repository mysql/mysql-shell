# imports
import re
import shutil

options_arg_no = None

def setup_copy_tests(arg_no, src_options = {}):
    global options_arg_no
    options_arg_no = arg_no
    testutil.deploy_sandbox(__mysql_sandbox_port1, "root", {}, src_options)
    globals()["src_session"] = mysql.get_session(__sandbox_uri1)
    testutil.import_data(__sandbox_uri1, os.path.join(__data_path, "sql", "sakila-schema.sql"), "", "utf8mb4")
    testutil.import_data(__sandbox_uri1, os.path.join(__data_path, "sql", "sakila-data.sql"), "", "utf8mb4")
    setup_session(__sandbox_uri1)
    analyze_tables(session)
    session.run_sql("CREATE EVENT sakila.existing_event ON SCHEDULE EVERY 1 YEAR DO BEGIN END")
    testutil.deploy_sandbox(__mysql_sandbox_port2, "root", { "local_infile": 1 })
    globals()["tgt_session"] = mysql.get_session(__sandbox_uri2)

def cleanup_copy_tests():
    testutil.destroy_sandbox(__mysql_sandbox_port1)
    testutil.destroy_sandbox(__mysql_sandbox_port2)

def setup_session(uri):
    shell.connect(uri)
    session.run_sql("SET NAMES 'utf8mb4'")

def analyze_tables(s):
    for schema, in s.run_sql("SHOW SCHEMAS").fetch_all():
        if schema in ("mysql", "sys", "performance_schema", "information_schema"):
            continue
        for table, in s.run_sql("SHOW TABLES IN !", [ schema ]).fetch_all():
            s.run_sql("ANALYZE TABLE !.!", [ schema, table ])

def dump_and_load_options(options):
    d = options.copy()
    l = {}
    # files are not compressed
    d["compression"] = "none"
    # different names
    if "defaultCharacterSet" in d:
        l["characterSet"] = d["defaultCharacterSet"]
    # options with different default values
    l["loadData"] = not d.get("ddlOnly", False)
    l["loadDdl"] = not d.get("dataOnly", False)
    l["loadUsers"] = d.get("users", True)
    # load options
    for opt in ["analyzeTables",
                "deferTableIndexes",
                "handleGrantErrors",
                "ignoreExistingObjects",
                "ignoreVersion",
                "loadIndexes",
                "maxBytesPerTransaction",
                "schema",
                "sessionInitSql",
                "skipBinlog",
                "updateGtidSet",
                "dryRun"]: # in order to simulate dry run only loader should use it
        if opt in d:
            l[opt] = d[opt]
            del d[opt]
    # common options
    for opt in ["showProgress",
                "threads"]:
        if opt in d:
            l[opt] = d[opt]
    return (d, l)

def get_sysvar(s, name):
    return s.run_sql(f"SHOW GLOBAL VARIABLES WHERE variable_name IN ('{name}')").fetch_one()[1]

def get_ssl_config(s, uri = None):
    config = {}
    if uri:
        if isinstance(uri, dict):
            config = uri
        else:
            config = shell.parse_uri(uri)
    if "host" not in config:
        config["host"] = __host
    if "port" not in config:
        config["port"] = get_sysvar(s, "port")
    datadir = get_sysvar(s, "datadir")
    config["ssl-key"] = os.path.join(datadir, "client-key.pem")
    config["ssl-cert"] = os.path.join(datadir, "client-cert.pem")
    return config

def get_x_config(s, uri = None):
    config = {}
    if uri:
        if isinstance(uri, dict):
            config = uri
        else:
            config = shell.parse_uri(uri)
    if "host" not in config:
        config["host"] = __host
    config["scheme"] = "mysqlx"
    config["port"] = get_sysvar(s, "mysqlx_port")
    return config

def create_test_user(s):
    s.run_sql(f"CREATE USER {test_user_account} IDENTIFIED BY ? REQUIRE X509;", [test_user_pwd])
    s.run_sql(f"GRANT ALL ON *.* TO {test_user_account}")

def EXPECT_SUCCESS_IMPL(method, connectionData, options, src, setup, *args):
    dump_cb = None
    if method.__name__ == "copy_instance":
        dump_cb = util.dump_instance
    elif method.__name__ == "copy_schemas":
        dump_cb = util.dump_schemas
    elif method.__name__ == "copy_tables":
        dump_cb = util.dump_tables
    else:
        raise Exception(f"Unsupported method: {method.__name__}")
    if "showProgress" not in options:
        options["showProgress"] = False
    if method.__name__ == "copy_instance" and options.get("users", True) and "excludeUsers" not in options:
        options["excludeUsers"] = [ "'root'@'%'" ]
    dump_options, load_options = dump_and_load_options(options)
    test_output = 'copy'
    shutil.rmtree(test_output, True)
    wipeout_server(tgt_session)
    setup_session(src)
    if setup:
        setup()
    # dump
    EXPECT_NO_THROWS(lambda: dump_cb(*args, test_output, dump_options), "dump should not throw")
    # load
    setup_session(connectionData)
    EXPECT_NO_THROWS(lambda: util.load_dump(test_output, load_options), "load should not throw")
    expected = snapshot_instance(tgt_session)
    # run copy operation
    wipeout_server(tgt_session)
    setup_session(src)
    if setup:
        setup()
    WIPE_STDOUT()
    EXPECT_NO_THROWS(lambda: method(*args, connectionData, options), "copy should not throw")
    actual = snapshot_instance(tgt_session)
    # verify that dump&load created the same result as copy
    EXPECT_JSON_EQ(expected, actual, "instances do not match")

def EXPECT_FAIL_IMPL(method, error, msg, connectionData, options, src, setup, *args):
    is_re = is_re_instance(msg)
    full_msg = f"{re.escape(error) if is_re else error}: Util.{method.__name__}: {msg.pattern if is_re else msg}"
    if is_re:
        full_msg = re.compile("^" + full_msg)
    if not session.is_open():
        setup_session(src)
    if setup:
        setup()
    WIPE_STDOUT()
    EXPECT_THROWS(lambda: method(*args, connectionData, options), full_msg)

def TEST_BOOL_OPTION(option):
    global options_arg_no
    tgt = "mysql://user@host:3306"
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Bool, but is Null", tgt, { option: None })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' Bool expected, but value is String", tgt, { option: "dummy" })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Bool, but is Array", tgt, { option: [] })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Bool, but is Map", tgt, { option: {} })

def TEST_STRING_OPTION(option):
    global options_arg_no
    tgt = "mysql://user@host:3306"
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type String, but is Null", tgt, { option: None })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type String, but is Integer", tgt, { option: 5 })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type String, but is Integer", tgt, { option: -5 })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type String, but is Array", tgt, { option: [] })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type String, but is Map", tgt, { option: {} })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type String, but is Bool", tgt, { option: False })

def TEST_UINT_OPTION(option):
    global options_arg_no
    tgt = "mysql://user@host:3306"
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type UInteger, but is Null", tgt, { option: None })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' UInteger expected, but Integer value is out of range", tgt, { option: -5 })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' UInteger expected, but value is String", tgt, { option: "dummy" })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type UInteger, but is Array", tgt, { option: [] })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type UInteger, but is Map", tgt, { option: {} })

def TEST_ARRAY_OF_STRINGS_OPTION(option):
    global options_arg_no
    tgt = "mysql://user@host:3306"
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Array, but is Integer", tgt, { option: 5 })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Array, but is Integer", tgt, { option: -5 })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Array, but is String", tgt, { option: "dummy" })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Array, but is Map", tgt, { option: {} })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Array, but is Bool", tgt, { option: False })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' String expected, but value is Null", tgt, { option: [ None ] })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' String expected, but value is Integer", tgt, { option: [ 5 ] })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' String expected, but value is Integer", tgt, { option: [ -5 ] })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' String expected, but value is Map", tgt, { option: [ {} ] })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' String expected, but value is Bool", tgt, { option: [ False ] })

def TEST_MAP_OF_STRINGS_OPTION(option):
    global options_arg_no
    tgt = "mysql://user@host:3306"
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Map, but is Integer", tgt, { option: 5 })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Map, but is Integer", tgt, { option: -5 })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Map, but is String", tgt, { option: "dummy" })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Map, but is Array", tgt, { option: [] })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Map, but is Bool", tgt, { option: False })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' String expected, but value is Null", tgt, { option: { "key": None } })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' String expected, but value is Integer", tgt, { option: { "key": 5 } })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' String expected, but value is Integer", tgt, { option: { "key": -5 } })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' String expected, but value is Map", tgt, { option: { "key": {} } })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' String expected, but value is Bool", tgt, { option: { "key": False } })

def TEST_MAP_OF_ARRAY_OF_STRINGS_OPTION(option):
    global options_arg_no
    tgt = "mysql://user@host:3306"
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Map, but is Integer", tgt, { option: 5 })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Map, but is Integer", tgt, { option: -5 })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Map, but is String", tgt, { option: "dummy" })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Map, but is Array", tgt, { option: [] })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' is expected to be of type Map, but is Bool", tgt, { option: False })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' Array expected, but value is Integer", tgt, { option: { "key": 5 } })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' Array expected, but value is Integer", tgt, { option: { "key": -5 } })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' Array expected, but value is Map", tgt, { option: { "key": {} } })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' Array expected, but value is Bool", tgt, { option: { "key": False } })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' String expected, but value is Integer", tgt, { option: { "key": [ 5 ] } })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' String expected, but value is Integer", tgt, { option: { "key": [ -5 ] } })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' String expected, but value is Map", tgt, { option: { "key": [ {} ] } })
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no}: Option '{option}' String expected, but value is Bool", tgt, { option: { "key": [ False ] } })

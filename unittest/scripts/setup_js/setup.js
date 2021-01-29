var sandbox_uris = {}
sandbox_uris[__mysql_sandbox_port1] = __sandbox_uri1;
sandbox_uris[__mysql_sandbox_port2] = __sandbox_uri2;
sandbox_uris[__mysql_sandbox_port3] = __sandbox_uri3;

var add_instance_options = {HoSt:localhost, port: 0000, PassWord:'root', scheme:'mysql'};

// Gets the initial UUID of the given instance caching the data
// for further requests
var sandbox_uuids = {}
function get_sandbox_uuid(port){
    if (!sandbox_uuids.hasOwnProperty(port)) {
        var mySession = mysql.getSession(sandbox_uris[port]);
        var result = mySession.runSql("select @@server_uuid as uuid");
        sandbox_uuids[port] = result.fetchOneObject().uuid;
        mySession.close()
    }

    return sandbox_uuids[port];
}

// Gets the initial auto.cnf file for the given sandbox
var auto_cnfs = {}
var auto_cnf_paths = {}
function get_sandbox_auto_cnf(port){
    if (!auto_cnfs.hasOwnProperty(port)) {
        var auto_path = testutil.getSandboxLogPath(port);
        var auto_path = auto_path.replace("error.log", "auto.cnf");
        auto_cnf_paths[port] = auto_path;
        auto_cnfs[port] = os.loadTextFile(auto_path);
    }

    return auto_cnfs[port];
}


// Update the sandbox UUID on auto.cnf replacing the last
// characters of the current UUID with the value provided
// at sequence
function update_sandbox_uuid(port, sequence) {
    var uuid = get_sandbox_uuid(port)
    var new_uuid = uuid.substring(0, uuid.length - sequence.length) + sequence;
    var new_auto_cnf = [];
    function update_auto_cnf(value, index, array) {
        if(value.startsWith("server-uuid")) {
            new_auto_cnf.push("server-uuid = " + new_uuid);
        } else {
            new_auto_cnf.push(value);
        }
    }
    var auto_cnf = get_sandbox_auto_cnf(port)
    auto_cnf.split("\n").forEach(update_auto_cnf);
    auto_cnfs[port] = new_auto_cnf.join("\n");
    sandbox_uuids[port] = new_uuid;
    testutil.createFile(auto_cnf_paths[port], auto_cnfs[port]);

    return new_uuid;
}

function create_root_from_anywhere(session, clear_super_read_only) {
  var super_read_only = get_sysvar(session, "super_read_only");
  var super_read_only_enabled = super_read_only === "ON";
  session.runSql("SET SQL_LOG_BIN=0");
  if (clear_super_read_only && super_read_only_enabled)
    set_sysvar(session, "super_read_only", 0);
  session.runSql("CREATE USER root@'%' IDENTIFIED BY 'root'");
  session.runSql("GRANT ALL ON *.* to root@'%' WITH GRANT OPTION");
  if (clear_super_read_only && super_read_only_enabled)
    set_sysvar(session, "super_read_only", 1);
  session.runSql("SET SQL_LOG_BIN=1");
}

function get_socket_path(session, uri = undefined) {
  if (undefined === uri) {
    uri = session.uri;
  }

  var row = session.runSql(`SELECT @@${"mysql" === shell.parseUri(uri).scheme ? "socket" : "mysqlx_socket"}, @@datadir`).fetchOne();

  if (row[0][0] == '/')
    p = row[0];
  else if (row[1][row[1].length-1] == '/')
    p = row[1] + row[0];
  else
    p = row[1] + "/" + row[0];
  if (p.length > 100) {
    testutil.fail("socket path is too long (>100): " + p);
  }
  return p;
}

function run_nolog(session, query) {
    session.runSql("set session sql_log_bin=0");
    session.runSql(query);
    session.runSql("set session sql_log_bin=1");
}

var kGrantsForPerformanceSchema = ["GRANT SELECT ON `performance_schema`.`replication_applier_configuration` TO `admin`@`%` WITH GRANT OPTION",
"GRANT SELECT ON `performance_schema`.`replication_applier_status_by_coordinator` TO `admin`@`%` WITH GRANT OPTION",
"GRANT SELECT ON `performance_schema`.`replication_applier_status_by_worker` TO `admin`@`%` WITH GRANT OPTION",
"GRANT SELECT ON `performance_schema`.`replication_applier_status` TO `admin`@`%` WITH GRANT OPTION",
"GRANT SELECT ON `performance_schema`.`replication_connection_configuration` TO `admin`@`%` WITH GRANT OPTION",
"GRANT SELECT ON `performance_schema`.`replication_connection_status` TO `admin`@`%` WITH GRANT OPTION",
"GRANT SELECT ON `performance_schema`.`replication_group_member_stats` TO `admin`@`%` WITH GRANT OPTION",
"GRANT SELECT ON `performance_schema`.`replication_group_members` TO `admin`@`%` WITH GRANT OPTION",
"GRANT SELECT ON `performance_schema`.`threads` TO `admin`@`%` WITH GRANT OPTION",
"GRANT SELECT ON `mysql`.`func` TO `admin`@`%` WITH GRANT OPTION"];

/**
 * Verifies if a variable is defined, returning true or false accordingly
 * @param cb An anonymous function that simply executes the variable to be
 * verified, example:
 *
 * defined(function(){myVar})
 *
 * Will return true if myVar is defined or false if not.
 */
function defined(cb) {
  try {
    cb();
    return true;
  } catch {
    return false
  }
}

function hasOciEnvironment(context) {
  if (['OS', 'MDS'].indexOf(context) == -1) {
    return false
  }

  let variables = ['OCI_CONFIG_HOME',
                   'OCI_COMPARTMENT_ID',
                   'OS_NAMESPACE',
                   'OS_BUCKET_NAME'];
  if (context == 'MDS') {
    variables = variables.concat(['MDS_URI']);
  }

  let missing=[];
  for (var index = 0; index < variables.length; index ++) {
    if (!defined(function(){eval(variables[index])})) {
      missing.push(variables[index])
    }
  }
  if (missing.length) {
    return false;
  }
  return true;
}


function hasAuthEnvironment(context) {
  if (['LDAP_SIMPLE', 'LDAP_SASL', 'LDAP_KERBEROS', 'KERBEROS'].indexOf(context) == -1) {
    return false
  }

  var variables = [];

  if (context == 'LDAP_SIMPLE') {
    variables = ['LDAP_SIMPLE_SERVER_HOST',
                 'LDAP_SIMPLE_SERVER_PORT',
                 'LDAP_SIMPLE_BIND_BASE_DN',
                 'LDAP_SIMPLE_USER',
                 'LDAP_SIMPLE_PWD',
                 'LDAP_SIMPLE_AUTH_STRING'];
  } else if (context == 'LDAP_SASL') {
    variables = ['LDAP_SASL_SERVER_HOST',
                 'LDAP_SASL_SERVER_PORT',
                 'LDAP_SASL_BIND_BASE_DN',
                 'LDAP_SASL_USER',
                 'LDAP_SASL_PWD',
                 'LDAP_SASL_GROUP_SEARCH_FILTER',
                 'MYSQL_PLUGIN_DIR'];
  } else if (context == 'LDAP_KERBEROS') {
    variables = ['LDAP_KERBEROS_SERVER_HOST',
                 'LDAP_KERBEROS_SERVER_PORT',
                 'LDAP_KERBEROS_BIND_BASE_DN',
                 'LDAP_KERBEROS_USER_SEARCH_ATTR',
                 'LDAP_KERBEROS_USER',
                 'LDAP_KERBEROS_PWD',
                 'LDAP_KERBEROS_AUTH_STRING',
                 'MYSQL_PLUGIN_DIR'];
  } else if (context == 'KERBEROS') {
    variables = ['KERBEROS_USER',
                 'KERBEROS_PWD',
                 'MYSQL_PLUGIN_DIR'];
  }

  let missing=[];
  for (var index = 0; index < variables.length; index ++) {
    if (!defined(function(){eval(variables[index])})) {
      missing.push(variables[index])
    }
  }

  if (missing.length) {
    shell.log("warning", `Missing Variables: ${missing}`)
    return false;
  }
  return true;
}


function set_sysvar(session, variable, value) {
  session.runSql("SET GLOBAL " + variable + " = ?", [value]);
}

function get_sysvar(session, variable, type) {
  var close_session = false;
  var pos = 0;
  var query = "";
  // Check if the variable session is an int, if so it's the member_port and we need to establish a session
  if (typeof session == "number") {
    session = shell.connect("mysql://root:root@localhost:" + session);
    close_session = true;
  }

  if (type === undefined)
    type = "GLOBAL";

  if (type == "GLOBAL") {
    query = "SELECT @@GLOBAL." + variable;
  } else if (type == "PERSISTED") {
    query = "SELECT * from performance_schema.persisted_variables WHERE Variable_name like '%" + variable + "%'";
    pos = 1;
  }

  var row = session.runSql(query).fetchOne();

  // Close the session if established in this function
  if (close_session)
    session.close();

  if (row == null)
    return "";

  return row[pos];
}

function ensure_plugin_enabled(plugin_name, session, plugin_soname) {
  if (plugin_soname === undefined)
    plugin_soname = plugin_name;

  var os = session.runSql('select @@version_compile_os').fetchOne()[0];
  if (os == "Win32" || os == "Win64") {
    session.runSql("INSTALL PLUGIN " + plugin_name + " SONAME '" + plugin_soname + ".dll';");
  }
  else {
    session.runSql("INSTALL PLUGIN " + plugin_name + " SONAME '" + plugin_soname + ".so';");
  }
}

function ensure_plugin_disabled(plugin_name, session) {
  var rs = session.runSql("SELECT COUNT(1) FROM INFORMATION_SCHEMA.PLUGINS WHERE PLUGIN_NAME like '" + plugin_name + "';");
  var is_installed = rs.fetchOne()[0];

  if (is_installed) {
    session.runSql("UNINSTALL PLUGIN " + plugin_name + ";");
  }
}

function restart_gr_plugin(session, close) {
  close = typeof close !== 'undefined' ? close : false;
  var owns_session = false;

  // Check if the variable session is an int, if so it's the member_port and we need to establish a session
  if (typeof session == "number") {
    session = shell.connect("mysql://root:root@localhost:" + session);
    owns_session = true;
  }
  session.runSql("STOP GROUP_REPLICATION;");
  session.runSql("START GROUP_REPLICATION;");

  // Close the session if established in this function
  if (owns_session && close)
    session.close();
}

function get_query_single_result(session, query) {
  var close_session = false;

  // Check if the variable session is an int, if so it's the member_port and we need to establish a session
  if (typeof session == "number") {
    session = shell.connect("mysql://root:root@localhost:" + session);
    close_session = true;
  }

  if (query === undefined)
    testutil.fail("query argument for get_query_single_result() can't be empty");

  var row = session.runSql(query).fetchOne();

  // Close the session if established in this function
  if (close_session)
    session.close();

  if (row == null)
    testutil.fail("Query returned no results");

  return row[0];
}

function get_persisted_gr_sysvars(session) {
  var close_session = false;

  // Check if the variable session is an int, if so it's the member_port and we need to establish a session
  if (typeof session == "number") {
    session = shell.connect("mysql://root:root@localhost:" + session);
    close_session = true;
  }

  var query = "SELECT * from performance_schema.persisted_variables WHERE Variable_name like '%group_replication%'";

  var ret = "";

  var result = session.runSql(query);
  var row = result.fetchOne();

  if (row == null) {
    if (close_session)
      session.close();

    testutil.fail("Query returned no results");
  }

  while (row) {
    var ret_line = row[0] + " = " + row[1];
    ret += "\n" + ret_line;
    row = result.fetchOne();
  }

  // Close the session if established in this function
  if (close_session)
    session.close();

  return ret;
}

function number_of_gr_recovery_accounts(session) {
  // All internal recovery users have the prefix: 'mysql_innodb_cluster_'.
  var res = session.runSql(
    "SELECT COUNT(*)  FROM mysql.user u " +
    "WHERE u.user LIKE 'mysql_innodb_cluster_%'");
  var row = res.fetchOne();
  return row[0];
}

function get_all_gr_recovery_accounts(session) {
  var close_session = false;

  // Check if the variable session is an int, if so it's the member_port and we need to establish a session
  if (typeof session == "number") {
    session = shell.connect("mysql://root:root@localhost:" + session);
    close_session = true;
  }

  var query = "SELECT USER, HOST FROM mysql.user WHERE USER LIKE 'mysql_innodb_cluster_%'";

  var ret = "";

  var result = session.runSql(query);
  var row = result.fetchOne();

  if (row == null) {
    if (close_session)
      session.close();

    testutil.fail("Query returned no results");
  }

  while (row) {
    var ret_line = row[0] + ", " + row[1];
    ret += "\n" + ret_line;
    row = result.fetchOne();
  }

  // Close the session if established in this function
  if (close_session)
    session.close();

  return ret;
}

function get_gr_recovery_user_passwd(session) {
  var close_session = false;

  // Check if the variable session is an int, if so it's the member_port and we need to establish a session
  if (typeof session == "number") {
    session = shell.connect("mysql://root:root@localhost:" + session);
    close_session = true;
  }

  var query = "SELECT User_name, User_password FROM mysql.slave_master_info";

  var ret = "";

  var result = session.runSql(query);
  var row = result.fetchOne();

  if (row == null) {
    if (close_session)
      session.close();

    testutil.fail("Query returned no results");
  }

  while (row) {
    var ret_line = row[0] + ", " + row[1];
    ret += "\n" + ret_line;
    row = result.fetchOne();
  }

  // Close the session if established in this function
  if (close_session)
    session.close();

  return ret;
}

function disable_auto_rejoin(session, port) {
  var close_session = false;

  // Check if the variable session is an int, if so it's the member_port and we need to establish a session
  if (typeof session == "number") {
    var port = session;
    session = shell.connect("mysql://root:root@localhost:" + session);
    close_session = true;
  }

  testutil.changeSandboxConf(port, "group_replication_start_on_boot", "OFF");

  if (__version_num > 80011)
    session.runSql("RESET PERSIST group_replication_start_on_boot");

  // Close the session if established in this function
  if (close_session)
    session.close();
}

function wait_member_state_from(session, member_port, state) {
  // wait for the given member to appear in the given state from the point of
  // view of a specific session
  const timeout = 60;
  for (i = 0; i < timeout; i++) {
    var r = session.runSql("SELECT member_state FROM performance_schema.replication_group_members WHERE member_port=?", [member_port]).fetchOne();
    if (!r) {
      testutil.fail("member_state query returned no rows for "+member_port);
      break;
    }
    if (r[0] == state) return;
    os.sleep(1);
  }
  testutil.fail("Timeout while waiting for "+member_port+" to become "+state+" when queried from "+session.runSql("select @@port").fetchOne()[0]);
}

function normalize_cluster_options(options) {
  // normalize output of cluster.options(), to make order of tags to be always the same
  options["defaultReplicaSet"]["tags"][".global"] = options["defaultReplicaSet"]["tags"]["global"];
  delete options["defaultReplicaSet"]["tags"]["global"];
  return options;
}

function normalize_rs_options(options) {
  // normalize output of rs.options(), to make order of tags to be always the same
  options["replicaSet"]["tags"][".global"] = options["replicaSet"]["tags"]["global"];
  delete options["replicaSet"]["tags"]["global"];
  return options;
}

// Check if the instance exists in the Metadata schema
function exist_in_metadata_schema(port) {
  if (typeof port == "number") {
    var result = session.runSql("SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.instances WHERE instance_name = '" + hostname + ":" + port + "';");
  } else {
    var result = session.runSql(
      'SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.instances where CAST(mysql_server_uuid AS char ascii) = CAST(@@server_uuid AS char ascii);');
  }

  var row = result.fetchOne();
  return row[0] != 0;
}

function reset_instance(session) {
  session.runSql("STOP SLAVE");
  session.runSql("SET GLOBAL super_read_only=0");
  session.runSql("SET GLOBAL read_only=0");
  session.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
  var r = session.runSql("SHOW SCHEMAS");
  var rows = r.fetchAll();
  for (var i in rows) {
      var row = rows[i];
      if (["mysql", "performance_schema", "sys", "information_schema"].includes(row[0]))
          continue;
      session.runSql("DROP SCHEMA "+row[0]);
  }
  var r = session.runSql("SELECT user,host FROM mysql.user");
  var rows = r.fetchAll();
  for (var i in rows) {
      var row = rows[i];
      if (["mysql.sys", "mysql.session", "mysql.infoschema"].includes(row[0]))
          continue;
      if (row[0] == "root" && (row[1] == "localhost" || row[1] == "%"))
          continue;
      session.runSql("DROP USER ?@?", [row[0], row[1]]);
  }
  session.runSql("RESET MASTER");
  session.runSql("RESET SLAVE ALL");
}

var SANDBOX_PORTS = [__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3];
var SANDBOX_LOCAL_URIS = [__sandbox_uri1, __sandbox_uri2, __sandbox_uri3];
var SANDBOX_URIS = [__hostname_uri1, __hostname_uri2, __hostname_uri3];

var s = mysql.getSession(__mysqluripwd+"?ssl-mode=disabled");
var r = s.runSql("SELECT @@hostname, @@report_host").fetchOne();
var __mysql_hostname = r[0];
var __mysql_report_host = r[1];
var __mysql_host = __mysql_report_host ? __mysql_report_host : __mysql_hostname;

// Address that appear in pre-configured sandboxes that set report_host to 127.0.0.1
var __address1 = "127.0.0.1:" + __mysql_sandbox_port1;
var __address2 = "127.0.0.1:" + __mysql_sandbox_port2;
var __address3 = "127.0.0.1:" + __mysql_sandbox_port3;
var __address4 = "127.0.0.1:" + __mysql_sandbox_port4;

// Address that appear in raw sandboxes, that show the real hostname
var __address1r = __mysql_host + ":" + __mysql_sandbox_port1;
var __address2r = __mysql_host + ":" + __mysql_sandbox_port2;
var __address3r = __mysql_host + ":" + __mysql_sandbox_port3;
var __address4r = __mysql_host + ":" + __mysql_sandbox_port4;

var CLUSTER_ADMIN = "AdminUser";
var CLUSTER_ADMIN_PWD = "AdminUserPwd";
var __sandbox_admin_uri1 = 'mysql://AdminUser:AdminUserPwd@localhost:' + __mysql_sandbox_port1;
var __sandbox_admin_uri2 = 'mysql://AdminUser:AdminUserPwd@localhost:' + __mysql_sandbox_port2;
var __sandbox_admin_uri3 = 'mysql://AdminUser:AdminUserPwd@localhost:' + __mysql_sandbox_port3;
var __sandbox_admin_uri4 = 'mysql://AdminUser:AdminUserPwd@localhost:' + __mysql_sandbox_port4;

// JSON utils

// Find a key in a json object recursively
function json_find_key(json, key) {
  if (key in json) {
    return json[key];
  }

  for (var k in json) {
    if (type(json[k]) == 'Object' || type(json[k]) == 'm.Map') {
      var o = json_find_key(json[k], key);
      if (o != undefined)
        return o;
    } else if (type(json[k]) == 'Array' || type(json[k]) == 'm.Array') {
      for (var i in json[k]) {
        if (type(json[k][i]) != "String") {
          var o = json_find_key(json[k][i], key);
          if (o != undefined)
            return o;
        }
      }
    }
  }

  return undefined;
}

function wait(timeout, wait_interval, condition) {
  if (__replaying) wait_interval = 0;
  waiting = 0;
  res = condition();
  while(!res && waiting < timeout) {
    os.sleep(wait_interval);
    waiting = waiting + 1;
    res = condition();
  }
  return res;
}

// --------

function begin_dba_log_sql(level) {
  shell.options["dba.logSql"] = level ? level : 1;
  var dummy = testutil.fetchDbaSqlLog(true);
}

function end_dba_log_sql(level) {
  var logs = testutil.fetchDbaSqlLog(false);
  shell.options["dba.logSql"] = 0;
  return logs;
}

// -------- Test Expectations


function EXPECT_NO_SQL(instance, logs, allowed_stmts) {
  var fail = false;
  for(var i in logs) {
    var line = logs[i];
    if (line.startsWith(instance)) {
      var bad = false;
      for (var j in allowed_stmts) {
        if (!line.split(": ")[1].startsWith(allowed_stmts[j])) {
          bad = true;
          break;
        }
      }
      if (bad) {
        println("UNEXPECTED SQL DETECTED:", line);
        fail = true;
      }
    }
  }
  EXPECT_FALSE(fail);
}

function EXPECT_SQL(instance, logs, expected_stmt) {
  var fail = true;
  for(var i in logs) {
    var line = logs[i];
    if (line.startsWith(instance)) {
      if (line.split(": ")[1].startsWith(expected_stmt)) {
        fail = false;
        break;
      }
    }
  }
  if (fail) {
    println("MISSING EXPECTED SQL:");
    for(var i in logs) {
      var line = logs[i];
      if (line.startsWith(instance)) {
        println("\t", line);
      }
    }
  }
  EXPECT_FALSE(fail);
}

function EXPECT_EQ(expected, actual, note) {
  if (note === undefined)
    note = "";
  if (repr(expected) != repr(actual)) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Tested values don't match as expected:</red> " + note + "\n\t<yellow>Actual: </yellow> " + repr(actual) + "\n\t<yellow>Expected:</yellow> " + repr(expected);
    testutil.fail(context);
  }
}

function EXPECT_GT(value1, value2, note) {
  if (note === undefined)
    note = "";
  if (value1 <= value2) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>EXPECT_GT failed:</red> " + note + "\n\t"+repr(value1)+" expected to be > "+repr(value2)+" but isn't.";
    testutil.fail(context);
  }
}

function EXPECT_JSON_EQ(expected, actual, note) {

  function compare_values(expected, actual, path) {
    if (typeof expected != typeof actual) {
      println(" =>", path, "mismatched values. Expected:\n", expected, "\nActual:\n", actual, "\n");
      return 1;
    } else if (typeof expected == "object") {
      return compare_objects(expected, actual, path);
    } else {
      jexpected = JSON.stringify(expected, undefined, 2);
      jactual = JSON.stringify(actual, undefined, 2);
      if (jexpected != jactual) {
        println(" =>", path, "mismatched values. Expected:\n", jexpected, "\nActual:\n", jactual, "\n");
        return 1;
      }
    }
    return 0;
  }

  function compare_objects(expected, actual, path) {
    ndiffs = 0
    for (k in expected) {
      if (actual[k] === undefined) {
        ndiffs += 1;
        println(" =>", path + "." + k, "expected but missing from result");
      } else {
        ndiffs += compare_values(expected[k], actual[k], path + "." + k);
      }
    }
    for (k in actual) {
      if (expected[k] === undefined) {
        ndiffs += 1;
        println(" =>", path + "." + k, "unexpected but found in result");
      }
    }
    return ndiffs;
  }

  if (note === undefined)
    note = "";

  if (typeof expected == "object" && typeof actual == "object" && !Array.isArray(expected) && !Array.isArray(actual)) {
    diffs = compare_objects(expected, actual, "$");
    if (diffs > 0) {
      var context = "<b>Context:</b> " + __test_context + "\n<red>Tested values don't match as expected.</red> "+note;
      testutil.fail(context);
    }
  } else {
    expected = JSON.stringify(expected, undefined, 2);
    actual = JSON.stringify(actual, undefined, 2);
    if (expected != actual) {
      var context = "<b>Context:</b> " + __test_context + "\n<red>Tested values don't match as expected:</red> "+note+"\n\t<yellow>Actual:</yellow> " + actual + "\n\t<yellow>Expected:</yellow> " + expected;
      testutil.fail(context);
    }
  }
}

function EXPECT_CONTAINS(expected, actual) {
  if (actual.indexOf(expected) < 0) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Tested text doesn't contain expected text:</red>\n\t<yellow>Actual:</yellow> " + actual + "\n\t<yellow>Expected:</yellow> " + expected;
    testutil.fail(context);
  }
}

function EXPECT_ARRAY_CONTAINS(expected, actual) {
  for (var i = 0; i < actual.length; ++i) {
    if (expected === actual[i]) return;
  }

  var context = "<b>Context:</b> " + __test_context + "\n<red>Tested array doesn't contain expected value:</red>\n\t<yellow>Actual:</yellow> " + JSON.stringify(actual) + "\n\t<yellow>Expected:</yellow> " + expected;
  testutil.fail(context);
}

function EXPECT_NE(expected, actual, note) {
  if (note === undefined)
    note = "";
  if (expected == actual) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Tested values don't differ as expected:</red> " + note + "\n\t<yellow>Actual Value:</yellow> " + actual + "\n\t<yellow>Checked Value:</yellow> " + expected;
    testutil.fail(context);
  }
}

function EXPECT_NOT_CONTAINS(expected, actual) {
  if (actual.indexOf(expected) >= 0) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Tested text contains checked text, but it shouldn't:</red>\n\t<yellow>Value:</yellow> " + actual + "\n\t<yellow>Checked Value:</yellow> " + expected;
    testutil.fail(context);
  }
}

function EXPECT_ARRAY_NOT_CONTAINS(expected, actual) {
  for (var i = 0; i < actual.length; ++i) {
    if (expected === actual[i]) {
      var context = "<b>Context:</b> " + __test_context + "\n<red>Tested array contains checked value, but it shouldn't:</red>\n\t<yellow>Array:</yellow> " + JSON.stringify(actual) + "\n\t<yellow>Checked Value:</yellow> " + expected;
      testutil.fail(context);
    }
  }
}

function EXPECT_TRUE(value, note) {
  if (note === undefined)
    note = "";
  else
    note = ": "+note;
  if (!value) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Tested value expected to be true but is false</red>"+note;
    testutil.fail(context);
  }
}

function EXPECT_FALSE(value, note) {
  if (note === undefined)
    note = "";
  else
    note = ": "+note;
  if (value) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Tested value expected to be false but is true</red>"+note;
    testutil.fail(context);
  }
}

function EXPECT_THROWS(func, etext) {
  if (typeof(etext) != "string" && typeof(etext) != "object") {
      testutil.fail("EXPECT_THROWS expects string or array, " +typeof(etext) + " given");
  }
  try {
    func();
    testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Missing expected exception throw like " + etext + "</red>");
  } catch (err) {
    testutil.dprint("Got exception as expected: " + JSON.stringify(err));
    if (typeof(etext) === "string") {
        if (err.message.indexOf(etext) < 0) {
          testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Exception expected:</red> " + etext + "\n\t<yellow>Actual:</yellow> " + err.message);
        }
    } else if (typeof(etext) === "object") {
        var found = false;
        for (i in etext) {
            if (err.message.indexOf(etext[i]) >= 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            var msg = "<b>Context:</b> " + __test_context + "\n<red>One of the exceptions expected:</red>\n";
            for (i in etext) {
                msg += etext[i] + "\n";
            }
            msg += "<yellow>Actual:</yellow> " + err.message;
            testutil.fail(msg);
        }
    }
  }
}

function EXPECT_THROWS_TYPE(func, etext, type) {
  try {
    func();
    testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Missing expected exception throw like " + etext + "</red>");
  } catch (err) {
    testutil.dprint("Caught exception: " + JSON.stringify(err));
    if (err.message.indexOf(etext) < 0) {
      testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Exception expected:</red> " + etext + "\n\t<yellow>Actual:</yellow> " + err.message);
    }
    if (err.type  !== type) {
      testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Exception type expected:</red> " + type + "\n\t<yellow>Actual:</yellow> " + err.type);
    }
  }
}

function EXPECT_NO_THROWS(func, context) {
  try {
    func();
  } catch (err) {
    testutil.dprint("Unexpected exception: " + JSON.stringify(err));
    testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Unexpected exception thrown (" + context + "): " + err.message + "</red>");
  }
}

function EXPECT_OUTPUT_CONTAINS(text) {
  var out = testutil.fetchCapturedStdout(false);
  var err = testutil.fetchCapturedStderr(false);
  if (out.indexOf(text) < 0 && err.indexOf(text) < 0) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
    testutil.fail(context);
  }
}

function EXPECT_OUTPUT_CONTAINS_ONE_OF(text) {
    var out = testutil.fetchCapturedStdout(false);
    var err = testutil.fetchCapturedStderr(false);
    var found = false;
    for (i in text) {
        if (out.indexOf(text[i]) >= 0 || err.indexOf(text[i]) >= 0) {
            found = true;
            break;
        }
    }
    if (!found) {
        var context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:\n</red>";
        for (i in text) {
            context += text[i] + "\n";
        }
        context += "<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
        testutil.fail(context);
    }
}

function EXPECT_STDOUT_CONTAINS(text) {
  var out = testutil.fetchCapturedStdout(false);
  var err = testutil.fetchCapturedStderr(false);
  if (out.indexOf(text) < 0) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
    testutil.fail(context);
  }
}

function EXPECT_STDOUT_CONTAINS_ONE_OF(text) {
    var out = testutil.fetchCapturedStdout(false);
    var err = testutil.fetchCapturedStderr(false);
    var found = false;
    for (i in text) {
        if (out.indexOf(text[i])>= 0) {
            found = true;
            break;
        }
    }
    if (!found) {
      var context = "<b>Context:</b> " + __test_context + "\n<red>Missing STDOUT output:</red>\n";
      for (i in text) {
          context += text[i] + "\n";
      }
      context += "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
      testutil.fail(context);
    }
}

function EXPECT_STDERR_CONTAINS_ONE_OF(text) {
    var out = testutil.fetchCapturedStdout(false);
    var err = testutil.fetchCapturedStderr(false);
    var found = false;
    for (i in text) {
        if (err.indexOf(text[i])>= 0) {
            found = true;
            break;
        }
    }
    if (!found) {
      var context = "<b>Context:</b> " + __test_context + "\n<red>Missing STDERR output:</red>\n";
      for (i in text) {
          context += text[i] + "\n";
      }
      context += "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
      testutil.fail(context);
    }
}

function EXPECT_STDERR_CONTAINS(text) {
  var out = testutil.fetchCapturedStdout(false);
  var err = testutil.fetchCapturedStderr(false);
  if (err.indexOf(text) < 0) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Missing error output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
    testutil.fail(context);
  }
}

function __split_trim_join(text) {
  const needle = '\n';
  const s = text.split(needle);
  s.forEach(function (item, index) { this[index] = item.trimEnd(); }, s);
  return {'str': s.join(needle), 'array': s};
}

function __check_wildcard_match(expected, actual) {
  if (0 === expected.length) {
    return expected == actual;
  }

  const needle = '[[*]]';
  const strings = expected.split(needle);
  var start = 0;

  for (const str of strings) {
    const idx = actual.indexOf(str, start);

    if (idx < 0) {
      return false;
    }

    start = idx + str.length;
  }

  if (strings[strings.length - 1] === '') {
    // expected ends with wildcard
    start = actual.length;
  }

  return start === actual.length;
}

function __multi_value_compare(expected, actual) {
  const start = expected.indexOf('{{');

  if (start < 0) {
    return __check_wildcard_match(expected, actual);
  } else {
     const end = expected.indexOf('}}');
     const pre = expected.substring(0, start);
     const post = expected.substring(end + 2);
     const opts = expected.substring(start + 2, end);

     for (const item of opts.split('|')) {
       if (__check_wildcard_match(pre + item + post, actual)) {
         return true;
       }
     }

     return false;
  }
}

function __find_line_matching(expected, actual_lines, start_at = 0) {
  var actual_index = start_at;

  while (actual_index < actual_lines.length) {
    if (__multi_value_compare(expected, actual_lines[actual_index])) {
      break;
    } else {
      ++actual_index;
    }
  }

  return actual_index;
}

function __diff_with_error(arr, error, idx) {
  const needle = '\n';
  const copy = [...arr];
  copy[idx] += error;
  return copy.join(needle);
}

function __check_multiline_expect(expected_lines, actual_lines) {
  var diff = '';
  var matches = true;
  const needle = '\n';

  while ('' === expected_lines[0] || '[[*]]' === expected_lines[0]) {
    expected_lines.shift();
  }

  var actual_index = __find_line_matching(expected_lines[0], actual_lines);

  if (actual_index < actual_lines.length) {
    for (var expected_index = 0; expected_index < expected_lines.length; ++expected_index) {
      if ('[[*]]' === expected_lines[expected_index]) {
        if (expected_index == expected_lines.length - 1) {
          continue;  // Ignores [[*]] if at the end of the expectation
        } else {
          ++expected_index;

          actual_index = __find_line_matching(expected_lines[expected_index], actual_lines, actual_index + expected_index - 1);

          if (actual_index < actual_lines.length) {
            actual_index -= expected_index;
          } else {
            matches = false;
            diff = __diff_with_error(expected_lines, '<yellow><------ INCONSISTENCY</yellow>', expected_index);
            break;
          }
        }
      }

      if ((actual_index + expected_index) >= actual_lines.length) {
        matches = false;
        diff = __diff_with_error(expected_lines, '<yellow><------ MISSING</yellow>', expected_index);
        break;
      }

      if (!__multi_value_compare(expected_lines[expected_index], actual_lines[actual_index + expected_index])) {
        matches = false;
        diff = __diff_with_error(expected_lines, '<yellow><------ INCONSISTENCY</yellow>', expected_index);
        break;
      }
    }
  } else {
    matches = false;
    diff = __diff_with_error(expected_lines, '<yellow><------ INCONSISTENCY</yellow>', 0);
  }

  return {'matches': matches, 'diff': diff};
}


function __check_output_contains_multiline(input_text, output_type) {
    if (!["both", "err", "out"].includes(output_type))
        testutil.fail("EXPECT_CONTAINS_MULTILINE got unknown argument: " + output_type + " allowed: both, err, out");
    const out = __split_trim_join(testutil.fetchCapturedStdout(false));
    const err = __split_trim_join(testutil.fetchCapturedStderr(false));

    var for_match = [];
    if (typeof(input_text) === "object") {
        for_match = input_text;
    } else if (typeof(input_text) === "string"){
        for_match = [input_text];
    } else {
        testutil.fail("EXPECT_CONTAINS_MULTILINE got unknown argument: " + input_text + " allowed: string or array(string)");
    }

    var found = false;
    for (i in for_match) {
        const text = __split_trim_join(for_match[i]);
        var out_result = {"matches": false, "diff":""};
        var err_result = {"matches": false, "diff":""};
        if (output_type === "both") {
            out_result = __check_multiline_expect(text.array, out.array);
            err_result = __check_multiline_expect(text.array, err.array);
        } else if (output_type === "err") {
            err_result = __check_multiline_expect(text.array, err.array);
        } else if (output_type === "out") {
            out_result = __check_multiline_expect(text.array, out.array);
        }
        found = out_result.matches || err_result.matches;
        if (found)
            return {"found": found, "out":out, "err":err, "out_result":out_result, "err_result":err_result};
    }
    return {"found": found, "out":out, "err":err, "out_result":{"matches": false, "diff":""}, "err_result":{"matches": false, "diff":""}};
}

function EXPECT_OUTPUT_CONTAINS_MULTILINE_ONE_OF(t) {
    var ret = __check_output_contains_multiline(t, "both");
    if (!ret.found) {
        var context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red>\n";
        if (typeof(t) === "string") {
            context += text.str;
        } else {
            for (i in t) {
                context += t[i] + "\n";
            }
        }
        context += "\n<yellow>Actual stdout:</yellow> " + ret.out.str + "\n<yellow>Actual stderr:</yellow> " + ret.err.str + "\n<yellow>Diff with stdout:</yellow>\n" + ret.out_result.diff + "\n<yellow>Diff with stderr:</yellow>\n" + ret.err_result.diff;
        testutil.fail(context);
    }
}

function EXPECT_STDOUT_CONTAINS_MULTILINE_ONE_OF(t) {
    var ret = __check_output_contains_multiline(t, "out");
    if (!ret.found) {
        var context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red>\n";
        if (typeof(t) === "string") {
            context += text.str;
        } else {
            for (i in t) {
                context += t[i] + "\n";
            }
        }
        context += "\n<yellow>Actual stdout:</yellow> " + ret.out.str + "\n<yellow>Actual stderr:</yellow> " + ret.err.str + "\n<yellow>Diff with stdout:</yellow>\n" + ret.out_result.diff;
        testutil.fail(context);
    }
}

function EXPECT_STDERR_CONTAINS_MULTILINE_ONE_OF(t) {
    var ret = __check_output_contains_multiline(t, "err");
    if (!ret.found) {
        var context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red>\n";
        if (typeof(t) === "string") {
            context += text.str;
        } else {
            for (i in t) {
                context += t[i] + "\n";
            }
        }
        context += "\n<yellow>Actual stdout:</yellow> " + ret.out.str + "\n<yellow>Actual stderr:</yellow> " + ret.err.str + "\n<yellow>Diff with stderr:</yellow>\n" + ret.err_result.diff;
        testutil.fail(context);
    }
}


function EXPECT_OUTPUT_CONTAINS_MULTILINE(t) {
  const out = __split_trim_join(testutil.fetchCapturedStdout(false));
  const err = __split_trim_join(testutil.fetchCapturedStderr(false));
  const text = __split_trim_join(t);
  const out_result = __check_multiline_expect(text.array, out.array);
  const err_result = __check_multiline_expect(text.array, err.array);

  if (!out_result.matches && !err_result.matches) {
    const context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text.str + "\n<yellow>Actual stdout:</yellow> " + out.str + "\n<yellow>Actual stderr:</yellow> " + err.str + "\n<yellow>Diff with stdout:</yellow>\n" + out_result.diff + "\n<yellow>Diff with stderr:</yellow>\n" + err_result.diff;
    testutil.fail(context);
  }
}

function EXPECT_STDOUT_CONTAINS_MULTILINE(t) {
  const out = __split_trim_join(testutil.fetchCapturedStdout(false));
  const err = __split_trim_join(testutil.fetchCapturedStderr(false));
  const text = __split_trim_join(t);
  const out_result = __check_multiline_expect(text.array, out.array);

  if (!out_result.matches) {
    const context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text.str + "\n<yellow>Actual stdout:</yellow> " + out.str + "\n<yellow>Actual stderr:</yellow> " + err.str + "\n<yellow>Diff with stdout:</yellow>\n" + out_result.diff;
    testutil.fail(context);
  }
}

function EXPECT_STDERR_CONTAINS_MULTILINE(t) {
  const out = __split_trim_join(testutil.fetchCapturedStdout(false));
  const err = __split_trim_join(testutil.fetchCapturedStderr(false));
  const text = __split_trim_join(t);
  const err_result = __check_multiline_expect(text.array, err.array);

  if (!err_result.matches) {
    const context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text.str + "\n<yellow>Actual stdout:</yellow> " + out.str + "\n<yellow>Actual stderr:</yellow> " + err.str + "\n<yellow>Diff with stderr:</yellow>\n" + err_result.diff;
    testutil.fail(context);
  }
}

function EXPECT_OUTPUT_MATCHES(re) {
  var out = testutil.fetchCapturedStdout(false);
  var err = testutil.fetchCapturedStderr(false);
  if (!re.test(out) && !re.test(err)) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Missing match for:</red> " + re.source + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
    testutil.fail(context);
  }
}

function EXPECT_STDOUT_MATCHES(re) {
  var out = testutil.fetchCapturedStdout(false);
  var err = testutil.fetchCapturedStderr(false);
  if (!re.test(out)) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Missing match for:</red> " + re.source + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
    testutil.fail(context);
  }
}

function EXPECT_STDERR_MATCHES(re) {
  var out = testutil.fetchCapturedStdout(false);
  var err = testutil.fetchCapturedStderr(false);
  if (!re.test(err)) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Missing match for:</red> " + re.source + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
    testutil.fail(context);
  }
}

function EXPECT_SHELL_LOG_CONTAINS(text) {
  var log_path = testutil.getShellLogPath();
  var match_list = testutil.grepFile(log_path, text);
  if (match_list.length === 0) {
    var log_out = testutil.catFile(log_path);
    var context = "<b>Context:</b> " + __test_context + "\n<red>Missing log output:</red> " + text + "\n<yellow>Actual log output:</yellow> " + log_out;
    testutil.fail(context);
  }
}

function EXPECT_SHELL_LOG_NOT_CONTAINS(text) {
  var log_path = testutil.getShellLogPath();
  var match_list = testutil.grepFile(log_path, text);
  if (match_list.length !== 0) {
    var log_out = testutil.catFile(log_path);
    var context = "<b>Context:</b> " + __test_context + "\n<red>Unexpected log output:</red> " + text + "\n<yellow>Full log output:</yellow> " + log_out;
    testutil.fail(context);
  }
}

function WIPE_SHELL_LOG() {
  var log_path = testutil.getShellLogPath();
  testutil.wipeFileContents(log_path);
}

function EXPECT_STDOUT_EMPTY() {
  var out = testutil.fetchCapturedStdout(false);
  if (out != "") {
    testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Stdout expected to be empty but contains</red>: " + out);
  }
}

function EXPECT_STDERR_EMPTY() {
  var err = testutil.fetchCapturedStderr(false);
  if (err != "") {
    testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Stderr expected to be empty but contains</red>: " + err);
  }
}

function EXPECT_OUTPUT_NOT_CONTAINS(text) {
  var out = testutil.fetchCapturedStdout(false);
  var err = testutil.fetchCapturedStderr(false);
  if (out.indexOf(text) >= 0 || err.indexOf(text) >= 0) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Unexpected output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
    testutil.fail(context);
  }
}

function EXPECT_STDOUT_NOT_CONTAINS(text) {
  var out = testutil.fetchCapturedStdout(false);
  var err = testutil.fetchCapturedStderr(false);
  if (out.indexOf(text) >= 0) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Unexpected output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
    testutil.fail(context);
  }
}

function EXPECT_STDERR_NOT_CONTAINS(text) {
  var out = testutil.fetchCapturedStdout(false);
  var err = testutil.fetchCapturedStderr(false);
  if (err.indexOf(text) >= 0) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Unexpected output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
    testutil.fail(context);
  }
}

function EXPECT_NEXT_OUTPUT_EXACT(text) {
  var line = testutil.fetchCapturedStdout(true);
  EXPECT_CONTAINS(text, line);
}

function EXPECT_NEXT_OUTPUT(text) {
  var line = testutil.fetchCapturedStdout(true);
  // allow empty lines before the expected text
  while (line == "\n") line = testutil.fetchCapturedStdout(true);
  EXPECT_CONTAINS(text, line);
}

function WIPE_STDOUT() {
  var line;
  while ((line = testutil.fetchCapturedStdout(true)) !== "");
}

function WIPE_STDERR() {
  var line;
  while ((line = testutil.fetchCapturedStderr(true)) !== "");
}

function WIPE_OUTPUT() {
  WIPE_STDOUT();
  WIPE_STDERR();
}

// -------- InnoDB Cluster Scenarios

// Various test scenarios for InnoDB cluster, both normal cases and
// common error conditions

// ** Non-cluster/non-group standalone instances
function StandaloneScenario(ports) {
  for (i in ports) {
    testutil.deploySandbox(ports[i], "root", { report_host: hostname });
  }
  // Always connect to the 1st port
  this.session = shell.connect("mysql://root:root@localhost:" + ports[0]);

  this.destroy = function () {
    this.session.close();
    for (i in ports) {
      testutil.destroySandbox(ports[i]);
    }
  }
  return this;
}

// ** Cluster based scenarios
function ClusterScenario(ports, create_cluster_options, sandboxConfiguration) {
  for (i in ports) {
    testutil.deploySandbox(ports[i], "root", {report_host: hostname});

    if (sandboxConfiguration) {
      var uri = `root:root@localhost:${ports[i]}`;
      dba.configureInstance(uri, sandboxConfiguration);
    }

    if (testutil.versionCheck(__version, "<", "8.0.0")) {
      testutil.snapshotSandboxConf(ports[i]);
    }
  }

  this.session = shell.connect("mysql://root:root@localhost:" + ports[0]);

  if (create_cluster_options) {
    if (!create_cluster_options.hasOwnProperty("gtidSetIsComplete")) {
      create_cluster_options["gtidSetIsComplete"] = true;
    }
  } else {
    create_cluster_options = { gtidSetIsComplete: true };
  }

  this.cluster = dba.createCluster("cluster", create_cluster_options);

  var allowList = create_cluster_options["ipAllowlist"];

  for (i in ports) {
    if (i > 0) {
      var uri = "mysql://root:root@localhost:" + ports[i];
      if (allowList === undefined) {
        this.cluster.addInstance(uri);
      } else {
        this.cluster.addInstance(uri, {ipAllowlist: allowList});
      }
      testutil.waitMemberState(ports[i], "ONLINE");

      if (testutil.versionCheck(__version, "<", "8.0.0")) {
        dba.configureLocalInstance(uri, {mycnfPath: testutil.getSandboxConfPath(ports[i])});
      }
    }
  }

  this.destroy = function () {
    this.session.close();
    this.cluster.disconnect();
    for (i in ports) {
      testutil.destroySandbox(ports[i]);
    }
  }

  // The following make_* methods will setup a specific scenario from the
  // cluster. They should be considered mutually exclusive and can be called
  // only once.

  // Make this an unmanaged/non-InnoDB cluster GR group
  // by dropping the metadata
  this.make_unmanaged = function () {
    this.session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
  }

  // Make the cluster quorumless by killing all but given members and
  // wait them to become UNREACHABLE
  this.make_no_quorum = function (survivor_ports) {
    for (i in ports) {
      if (survivor_ports.indexOf(ports[i]) < 0) {
        testutil.killSandbox(ports[i]);
        var state = testutil.waitMemberState(ports[i], "UNREACHABLE,(MISSING)");
        if (state != "UNREACHABLE" && state != "(MISSING)")
          testutil.fail("Member " + ports[i] + " got into state that was not supposed to: " + state);
      }
    }
  }

  return this;
}

function validate_crud_functions(crud, expected) {
  var actual = dir(crud);

  // Ensures expected functions are on the actual list
  var missing = [];
  for (exp_funct of expected) {
    var pos = actual.indexOf(exp_funct);
    if (pos == -1) {
      missing.push(exp_funct);
    }
    else {
      actual.splice(pos, 1);
    }
  }

  if (missing.length == 0) {
    print("All expected functions are available\n");
  }
  else {
    print("Missing Functions:", missing);
  }

  // help is ignored cuz it's always available
  if (actual.length == 0 || (actual.length == 1 && actual[0] == 'help')) {
    print("No additional functions are available\n")
  }
  else {
    print("Extra Functions:", actual);
  }
}

function WARNING_SKIPPED_TEST(reason) {
  return false;
}

function get_mysqlx_uris(uri) {
  var u = shell.parseUri(uri);
  delete u.scheme;
  u.port *= 10;
  u = shell.unparseUri(u);
  return ["mysqlx://" + u, u];
}

function get_mysqlx_endpoint(uri) {
  const u = shell.parseUri(uri);
  return shell.unparseUri({ 'host': u.host, 'port': u.port * 10 });
}

var protocol_error_msg = "The provided URI uses the X protocol, which is not supported by this command.";

function EXPECT_THROWS_ERROR(msg, f, ...args) {
  EXPECT_THROWS(function () { f(...args); }, msg);
}

function CHECK_MYSQLX_EXPECT_THROWS_ERROR(msg, f, classic, ...args) {
  for (const uri of get_mysqlx_uris(classic)) {
    EXPECT_THROWS_ERROR(msg, f, uri, ...args);
  }
}

function EXPECT_DBA_THROWS_PROTOCOL_ERROR(context, f, classic, ...args) {
  CHECK_MYSQLX_EXPECT_THROWS_ERROR(`${context}: ${protocol_error_msg}`, f, classic, ...args);
}

function EXPECT_CLUSTER_THROWS_PROTOCOL_ERROR(context, f, classic, ...args) {
  for (const uri of get_mysqlx_uris(classic)) {
    const endpoint = get_mysqlx_endpoint(classic);
    WIPE_OUTPUT();
    EXPECT_THROWS_ERROR(`${context}: Could not open connection to '${endpoint}': ${protocol_error_msg}`, f, uri, ...args);
    EXPECT_STDOUT_CONTAINS(`Unable to connect to the target instance '${endpoint}'. Please verify the connection settings, make sure the instance is available and try again.`);
  }
}

function wait(timeout, wait_interval, condition){
  waiting = 0;
  res = condition();
  while(!res && waiting < timeout) {
    os.sleep(wait_interval);
    waiting = waiting + 1;
    res = condition();
  }
  return res;
}

function cleanup_sandbox(port) {
    println ('Stopping the sandbox at ' + port + ' to delete it...');
    try {
      stop_options = {}
      stop_options['password'] = 'root';
      if (__sandbox_dir != '')
        stop_options['sandboxDir'] = __sandbox_dir;

      dba.stopSandboxInstance(port, stop_options);
    } catch (err) {
      println(err.message);
    }

    options = {}
    if (__sandbox_dir != '')
      options['sandboxDir'] = __sandbox_dir;

    var deleted = false;

    print('Try deleting sandbox at: ' + port);
    deleted = wait(10, 1, function() {
      try {
        dba.deleteSandboxInstance(port, options);

        println(' succeeded');
        return true;
      } catch (err) {
        println(' failed: ' + err.message);
        return false;
      }
    });
    if (deleted) {
      println('Delete succeeded at: ' + port);
    } else {
      println('Delete failed at: ' + port);
    }
}

// Starting 8.0.24 the client lib started reporting connection error using
// host:port format, previous versions used just the host.
//
// This function is used to format the host description accordingly.
function libmysql_host_description(hostname, port) {
  if (testutil.versionCheck(__libmysql_version_id, ">", "8.0.23")) {
    return hostname + ":" + port;
  }

  return hostname;
}

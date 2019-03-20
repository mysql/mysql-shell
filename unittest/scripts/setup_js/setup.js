
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

function set_sysvar(session, variable, value) {
  session.runSql("SET GLOBAL "+variable+" = ?", [value]);
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

  if (type == undefined)
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

function get_query_single_result(session, query) {
  var close_session = false;

  // Check if the variable session is an int, if so it's the member_port and we need to establish a session
  if (typeof session == "number") {
    session = shell.connect("mysql://root:root@localhost:" + session);
    close_session = true;
  }

  if (query == undefined)
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

var SANDBOX_PORTS = [__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3];
var SANDBOX_LOCAL_URIS = [__sandbox_uri1, __sandbox_uri2, __sandbox_uri3];
var SANDBOX_URIS = [__hostname_uri1, __hostname_uri2, __hostname_uri3];

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
        var o = json_find_key(json[k][i], key);
        if (o != undefined)
          return o;
      }
    }
  }

  return undefined;
}

// -------- Test Expectations

function EXPECT_EQ(expected, actual, note) {
  if (note == undefined)
    note = "";
  if (repr(expected) != repr(actual)) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Tested values don't match as expected:</red> "+note+"\n\t<yellow>Actual:</yellow> " + repr(actual) + "\n\t<yellow>Expected:</yellow> " + repr(expected);
    testutil.fail(context);
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
  if (note == undefined)
    note = "";
  if (expected == actual) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Tested values don't differ as expected:</red> "+note+"\n\t<yellow>Actual Value:</yellow> " + actual + "\n\t<yellow>Checked Value:</yellow> " + expected;
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

function EXPECT_TRUE(value) {
  if (!value) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Tested value expected to be true but is false</red>";
    testutil.fail(context);
  }
}

function EXPECT_FALSE(value) {
  if (value) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Tested value expected to be false but is true</red>";
    testutil.fail(context);
  }
}

function EXPECT_THROWS(func, etext) {
  try {
    func();
    testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Missing expected exception throw like " + etext + "</red>");
  } catch (err) {
    if (err.message.indexOf(etext) < 0) {
      testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Exception expected:</red> " + etext + "\n\t<yellow>Actual:</yellow> " + err.message);
    }
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

function EXPECT_STDOUT_CONTAINS(text) {
  var out = testutil.fetchCapturedStdout(false);
  var err = testutil.fetchCapturedStderr(false);
  if (out.indexOf(text) < 0) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
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

function EXPECT_SHELL_LOG_CONTAINS(text) {
  var log_path = testutil.getShellLogPath();
  var match_list = testutil.grepFile(log_path, text);
  if (match_list.length === 0){
    var log_out = testutil.catFile(log_path);
    var context = "<b>Context:</b> " + __test_context + "\n<red>Missing log output:</red> " + text + "\n<yellow>Actual log output:</yellow> " + log_out;
    testutil.fail(context);
  }
}

function EXPECT_SHELL_LOG_NOT_CONTAINS(text) {
  var log_path = testutil.getShellLogPath();
  var match_list = testutil.grepFile(log_path, text);
  if (match_list.length !== 0){
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


// -------- InnoDB Cluster Scenarios

// Various test scenarios for InnoDB cluster, both normal cases and
// common error conditions

// ** Non-cluster/non-group standalone instances
function StandaloneScenario(ports) {
  for (i in ports) {
    testutil.deploySandbox(ports[i], "root");
  }
  // Always connect to the 1st port
  this.session = shell.connect("mysql://root:root@localhost:"+ports[0]);

  this.destroy = function() {
    this.session.close();
    for (i in ports) {
      testutil.destroySandbox(ports[i]);
    }
  }
  return this;
}

// ** Cluster based scenarios
function ClusterScenario(ports, topology_mode="pm") {
  for (i in ports) {
    testutil.deploySandbox(ports[i], "root", {report_host: hostname});
  }

  this.session = shell.connect("mysql://root:root@localhost:"+ports[0]);
  if (topology_mode == "mm") {
    this.cluster = dba.createCluster("cluster", {multiPrimary: true, force: true});
  } else {
    this.cluster = dba.createCluster("cluster");
  }
  for (i in ports) {
    if (i > 0) {
      this.cluster.addInstance("mysql://root:root@localhost:"+ports[i]);
      testutil.waitMemberState(ports[i], "ONLINE");
    }
  }

  this.destroy = function() {
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
  this.make_unmanaged = function() {
    this.session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
  }

  // Make the cluster quorumless by killing all but given members and
  // wait them to become UNREACHABLE
  this.make_no_quorum = function(survivor_ports) {
    for (i in ports) {
      if (survivor_ports.indexOf(ports[i]) < 0) {
        testutil.killSandbox(ports[i]);
        var state = testutil.waitMemberState(ports[i], "UNREACHABLE,(MISSING)");
        if (state != "UNREACHABLE" && state != "(MISSING)")
          testutil.fail("Member "+ports[i]+" got into state that was not supposed to: "+state);
      }
    }
  }

  return this;
}

function validate_crud_functions(crud, expected)
{
    var actual = dir(crud);

    // Ensures expected functions are on the actual list
    var missing = [];
    for(exp_funct of expected){
        var pos = actual.indexOf(exp_funct);
        if(pos == -1){
            missing.push(exp_funct);
        }
        else{
            actual.splice(pos, 1);
        }
    }

    if(missing.length == 0){
        print ("All expected functions are available\n");
    }
    else{
        print("Missing Functions:", missing);
    }

    // help is ignored cuz it's always available
    if (actual.length == 0 || (actual.length == 1 && actual[0] == 'help')) {
        print("No additional functions are available\n")
    }
    else{
        print("Extra Functions:", actual);
    }
}

function validateMember(memberList, member){
	if (memberList.indexOf(member) != -1){
		print(member + ": OK\n");
	}
	else{
		print(member + ": Missing\n");
	}
}

function validateNotMember(memberList, member){
	if (memberList.indexOf(member) != -1){
		print(member + ": Unexpected\n");
	}
	else{
		print(member + ": OK\n");
	}
}

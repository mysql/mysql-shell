
function create_root_from_anywhere(session) {
  session.runSql("SET SQL_LOG_BIN=0");
  session.runSql("CREATE USER root@'%' IDENTIFIED BY 'root'");
  session.runSql("GRANT ALL ON *.* to root@'%' WITH GRANT OPTION");
  session.runSql("SET SQL_LOG_BIN=1");
}

function set_sysvar(variable, value) {
  session.runSql("SET GLOBAL "+variable+" = ?", [value]);
}

function get_sysvar(variable) {
  return session.runSql("SHOW GLOBAL VARIABLES LIKE ?", [variable]).fetchOne()[1];
}

// -------- Test Expectations

function EXPECT_EQ(expected, actual) {
  if (expected != actual) {
    var context = "<red>Tested values don't match as expected:</red>\n\t<yellow>Actual:</yellow> " + actual + "\n\t<yellow>Expected:</yellow> " + expected;
    testutil.fail(context);
  }
}

function EXPECT_CONTAINS(expected, actual) {
  if (actual.indexOf(expected) < 0) {
    var context = "<red>Tested text doesn't contain expected text:</red>\n\t<yellow>Actual:</yellow> " + actual + "\n\t<yellow>Expected:</yellow> " + expected;
    testutil.fail(context);
  }
}

function EXPECT_NE(expected, actual) {
  if (expected == actual) {
    var context = "<red>Tested values don't differ as expected:</red>\n\t<yellow>Actual Value:</yellow> " + actual + "\n\t<yellow>Checked Value:</yellow> " + expected;
    testutil.fail(context);
  }
}

function EXPECT_NOT_CONTAINS(expected, actual) {
  if (actual.indexOf(expected) >= 0) {
    var context = "<red>Tested text contains checked text, but it shouldn't:</red>\n\t<yellow>Value:</yellow> " + actual + "\n\t<yellow>Checked Value:</yellow> " + expected;
    testutil.fail(context);
  }
}

function EXPECT_TRUE(value) {
  if (!value) {
    var context = "<red>Tested value expected to be true but is false</red>";
    testutil.fail(context);
  }
}

function EXPECT_FALSE(value) {
  if (value) {
    var context = "<red>Tested value expected to be false but is true</red>";
    testutil.fail(context);
  }
}

function EXPECT_THROWS(func, etext) {
  try {
    func();
    testutil.fail("<red>Missing expected exception throw like " + etext + "</red>");
  } catch (err) {
    if (err.message.indexOf(etext) < 0) {
      testutil.fail("<red>Exception expected:</red> " + etext + "\n\t<yellow>Actual:</yellow> " + err.message);
    }
  }
}

function EXPECT_OUTPUT_CONTAINS(text) {
  var out = testutil.fetchCapturedStdout();
  var err = testutil.fetchCapturedStderr();
  if (out.indexOf(text) < 0 && err.indexOf(text) < 0) {
    var context = "<red>Missing output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
    testutil.fail(context);
  }
}

function EXPECT_STDOUT_CONTAINS(text) {
  var out = testutil.fetchCapturedStdout();
  var err = testutil.fetchCapturedStderr();
  if (out.indexOf(text) < 0) {
    var context = "<red>Missing output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
    testutil.fail(context);
  }
}

function EXPECT_STDERR_CONTAINS(text) {
  var out = testutil.fetchCapturedStdout();
  var err = testutil.fetchCapturedStderr();
  if (err.indexOf(text) < 0) {
    var context = "<red>Missing error output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
    testutil.fail(context);
  }
}

function EXPECT_STDOUT_EMPTY() {
  var out = testutil.fetchCapturedStdout();
  if (out != "") {
    testutil.fail("<red>Stdout expected to be empty but contains</red>: " + out);
  }
}

function EXPECT_STDERR_EMPTY() {
  var err = testutil.fetchCapturedStderr();
  if (err != "") {
    testutil.fail("<red>Stderr expected to be empty but contains</red>: " + err);
  }
}

function EXPECT_OUTPUT_NOT_CONTAINS(text) {
  var out = testutil.fetchCapturedStdout();
  var err = testutil.fetchCapturedStderr();
  if (out.indexOf(text) >= 0 || err.indexOf(text) >= 0) {
    var context = "<red>Unexpected output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
    testutil.fail(context);
  }
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
function ClusterScenario(ports) {
  for (i in ports) {
    testutil.deploySandbox(ports[i], "root");
  }

  this.session = shell.connect("mysql://root:root@localhost:"+ports[0]);
  this.cluster = dba.createCluster("cluster");
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
        if (state != "UNREACHABLE")
          testutil.fail("Member "+ports[i]+" got into state that was not supposed to: "+state);
      }
    }
  }

  return this;
}

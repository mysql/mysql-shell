//@{!__dbug_off}

// Crash recovery tests for addInstance()

//@<> Prepare

testutil.deploySandbox(__mysql_sandbox_port1, "root")
testutil.deploySandbox(__mysql_sandbox_port2, "root")

shell.connect(__sandbox_uri1)

shell.options["dba.logSql"]=1;

var cluster;

if (__version_num >= 80027) {
  cluster = dba.createCluster("mycluster", {communicationStack: "xcom"});
} else {
  cluster = dba.createCluster("mycluster");
}

mysql.getSession(__sandbox_uri1).runSql("set global general_log=1");

//@<> run addInstance in a loop, fail during it and see if we can recover

function try_once(fail_after) {
  // shell.options.verbose=1;

  testutil.setTrap("mysql", ["sql !regex select.*|show.*", "++match_counter > "+fail_after], {msg: "debug break"});

  try {
    println("### Adding instance...");
    cluster.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});
    println("### Add worked on the 1st try: fail_after=", fail_after);
    return 42;
  } catch (error) {
    println("### Add failed as expected", error["message"]);
    EXPECT_TRUE(error["message"].search("debug break") >= 0);
  }

  testutil.clearTraps("mysql");
  testutil.resetTraps("mysql");

  // MD for instance shouldn't be added if the add fails
   shell.dumpRows(session.runSql("select count(*) from mysql_innodb_cluster_metadata.instances"));
  EXPECT_EQ(1, session.runSql("select count(*) from mysql_innodb_cluster_metadata.instances").fetchOne()[0]);

  if (shell.options.verbose) {
    shell.options.verbose=0;
    // clear everything to prevent verbose output from triggering a failure
    testutil.wipeAllOutput();
  }

  try {
    println("### Checking status()");

    print(cluster.status());
  } catch (error) {
    println("### exception on status():", error["message"]);
  }

  // shell.options.verbose=1;
  // check if we can retry
  try {
    println("### Retrying addInstance");
    cluster.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});
    println("### addInstance retry OK");

    num_instances = session.runSql("select count(*) from mysql_innodb_cluster_metadata.instances").fetchOne()[0];
    EXPECT_EQ(2, num_instances);
  } catch (error) {
    println("addInstance retry failed", error["message"]);
    throw error;
  }

  if (shell.options.verbose) {
    shell.options.verbose=0;
    // clear everything to prevent verbose output from triggering a failure
    testutil.wipeAllOutput();
  }


  println("### Cleaning up");
  cluster.removeInstance(__sandbox_uri2);
}

// set skip to 1 for full coverage sure, but increase it for shorter runtime
skip = 7;

for (iter = 10; iter < 100; iter += skip) {
  println();
  println("##############################################################");
  println("### Testing addInstance() recovery after "+iter+" statements");
  println("##############################################################");
  println();

  ok = false;
  r = null;
  EXPECT_NO_THROWS(function(){ r = try_once(iter); ok = true; });
  if (r == 42) {
    break;
  }
  if (!ok) {
    print("aborting at iteration ", iter);
    break;
  }
}

// If version >= 8.0.27, test with "MySQL" communication stack too
if (__version_num >= 80027) {
  testutil.clearTraps("mysql");
  testutil.resetTraps("mysql");

  cluster.dissolve({force: 1});
  cluster = dba.createCluster("mycluster", {communicationStack: "mysql"});

  skip = 30;

  for (iter = 10; iter < 100; iter += skip) {
    println();
    println("##############################################################");
    println("### Testing addInstance() recovery after "+iter+" statements");
    println("##############################################################");
    println("### Using MySQL Communication Stack");
    println();

    ok = false;
    r = null;
    EXPECT_NO_THROWS(function(){ r = try_once(iter); ok = true; });
    if (r == 42) {
      break;
    }
    if (!ok) {
      print("aborting at iteration ", iter);
      break;
    }
  }
}

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@{!__dbug_off}

// Crash recovery tests for createCluster()

//@<> Prepare

testutil.deploySandbox(__mysql_sandbox_port1, "root")

shell.connect(__sandbox_uri1)

shell.options["dba.logSql"]=1;

//@<> run createCluster in a loop, fail during it and see if we can recover

function try_once(fail_after, verbose) {
  shell.options.verbose=verbose;

  testutil.setTrap("mysql", ["sql !regex select.*|show.*", "++match_counter > "+fail_after], {msg: "debug break"});

  try {
    println("### Creating cluster...");
    c = dba.createCluster("cluster");
    println("### Create worked on the 1st try after executing", fail_after, "stmts");
    if (shell.options.verbose)
      testutil.wipeAllOutput();
    return 42;
  } catch (error) {
    println("### Create failed as expected", error["message"]);
    EXPECT_TRUE(error["message"].search("debug break") >= 0);
  }

  // either the MD schema must not exist or the failure indicator must be set
  EXPECT_TRUE(session.runSql("select * from mysql.slave_master_info where channel_name = '__mysql_innodb_cluster_creating_cluster__'").fetchOne() || !session.runSql("show schemas like 'mysql_innodb_cluster_metadata'").fetchOne());

  testutil.clearTraps("mysql");
  testutil.resetTraps("mysql");

  if (shell.options.verbose) {
    shell.options.verbose=0;
    // clear everything to prevent verbose output from triggering a failure
    testutil.wipeAllOutput();
  }

  shell.options.verbose=verbose;
  // check if we can retry
  try {
    println("### Retrying createCluster");
    c = dba.createCluster("cluster");
    println("### createCluster retry OK");

    num_clusters = session.runSql("select count(*) from mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
    EXPECT_EQ(1, num_clusters);

    num_instances = session.runSql("select count(*) from mysql_innodb_cluster_metadata.instances").fetchOne()[0];
    EXPECT_EQ(1, num_instances);

    EXPECT_FALSE(session.runSql("select * from mysql.slave_master_info where channel_name = '__mysql_innodb_cluster_creating_cluster__'").fetchOne());
  } catch (error) {
    println("createCluster retry failed", error["message"]);
    throw error;
  }

  if (shell.options.verbose) {
    shell.options.verbose=0;
    // clear everything to prevent verbose output from triggering a failure
    testutil.wipeAllOutput();
  }


  println("### Cleaning up");
  if (c) {
    c.dissolve();
    session.runSql("set global super_read_only=0");
    session.runSql("drop schema if exists mysql_innodb_cluster_metadata");
    session.runSql("reset master");    
  }
}

everything = false;

// set skip to 1 for full coverage sure, but increase it for shorter runtime
if (everything) {
  skip = 1;
  start = 1;
} else {
  skip = 11;
  start = 20;
}
verbose = 0;

for (iter = start; iter < 100; iter += skip) {
  println();
  println("##############################################################");
  println("### Testing createCluster() recovery after "+iter+" statements");
  println("##############################################################");
  println();

  ok = false;
  r = null;
  EXPECT_NO_THROWS(function(){ r = try_once(iter, verbose); ok = true; });
  if (r == 42) {
    break;
  }
  if (!ok) {
    print("aborting at iteration ", iter);
    break;
  }
}

//@<> Cleanup

testutil.clearTraps("mysql");
testutil.resetTraps("mysql");

testutil.destroySandbox(__mysql_sandbox_port1)

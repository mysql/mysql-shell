// Assumptions: smart deployment rountines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");

//@ connect to instance
shell.connect(__sandbox_uri1);
var singleSession = session;

//@ create first cluster
// Regression for BUG#270621122: Deprecate memberSslMode (ensure no warning is showed for createCluster)
if (__have_ssl)
  var single = dba.createCluster('single', {memberSslMode: 'REQUIRED'});
else
  var single = dba.createCluster('single', {memberSslMode: 'DISABLED'});

//@ Success adding instance
// Regression for BUG#270621122: Deprecate memberSslMode
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
single.addInstance(__sandbox_uri2, {memberSslMode: 'AUTO'});

// Waiting for the second added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

// Connect to the future new seed node
shell.connect(__sandbox_uri2);
var singleSession2 = session;

//@ Check auto_increment values for single-primary
// TODO(alfredo) this test currently fails (never worked?) because of Bug #27084767
//var row = singleSession.runSql("select @@auto_increment_increment, @@auto_increment_offset").fetchOne();
//testutil.expectEq(1, row[0]);
//testutil.expectEq(2, row[1]);
//var row = singleSession2.runSql("select @@auto_increment_increment, @@auto_increment_offset").fetchOne();
//testutil.expectEq(1, row[0]);
//testutil.expectEq(2, row[1]);

single.disconnect();
//@ Get the cluster back
// don't redirect to primary, since we're killing it
var single = dba.getCluster(null, {connectToPrimary:false});

// Kill the seed instance
testutil.killSandbox(__mysql_sandbox_port1);
testutil.waitMemberState(__mysql_sandbox_port1, "(MISSING),UNREACHABLE");

//@ Restore the quorum
single.forceQuorumUsingPartitionOf({host: localhost, port: __mysql_sandbox_port2, user: 'root', password:'root'});

//@ Success adding instance to the single cluster
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
single.addInstance(__sandbox_uri3);

//@ Remove the instance from the cluster
single.removeInstance({host: localhost, port: __mysql_sandbox_port3});

//@ create second cluster
shell.connect(__sandbox_uri3);
var multiSession = session;

// We must use clearReadOnly because the instance 3 was removed from the cluster before
// (BUG#26422638)
if (__have_ssl)
  var multi = dba.createCluster('multi', {memberSslMode:'REQUIRED', multiMaster:true, force:true, clearReadOnly: true});
else
  var multi = dba.createCluster('multi', {memberSslMode:'DISABLED', multiMaster:true, force:true, clearReadOnly: true});

//@ Failure adding instance from multi cluster into single
add_instance_options['port'] = __mysql_sandbox_port3;
single.addInstance(add_instance_options);

// Drops the metadata on the multi cluster letting a non managed replication group
dba.dropMetadataSchema({force:true});

//@ Failure adding instance from an unmanaged replication group into single
add_instance_options['port'] = __mysql_sandbox_port3;
single.addInstance(add_instance_options);

//@ Failure adding instance already in the InnoDB cluster
add_instance_options['port'] = __mysql_sandbox_port2;
single.addInstance(add_instance_options);

//@ Finalization
// Will close opened sessions and delete the sandboxes ONLY if this test was executed standalone
singleSession.close();
singleSession2.close();
multiSession.close();

single.disconnect();
multi.disconnect();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);

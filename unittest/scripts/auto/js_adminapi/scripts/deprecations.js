//@{!__dbug_off}

//@<> Setup
var scene = new ClusterScenario([__mysql_sandbox_port1]);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
var cluster = scene.cluster;
shell.connect(__sandbox_uri1);

//@<> A deprecation warning must be emitted when using the command cluster.rescan() with "addInstances" and/or "removeInstances" options
EXPECT_NO_THROWS(function() { cluster.rescan({addInstances: "auto"}); });
EXPECT_OUTPUT_CONTAINS(`The 'addInstances' and 'removeInstances' options are deprecated. Please use 'addUnmanaged' and/or 'removeObsolete' instead.`);
WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { cluster.rescan({removeInstances: "auto"}); });
EXPECT_OUTPUT_CONTAINS(`The 'addInstances' and 'removeInstances' options are deprecated. Please use 'addUnmanaged' and/or 'removeObsolete' instead.`);
WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { cluster.rescan({addInstances: "auto", removeInstances: "auto"}); });
EXPECT_OUTPUT_CONTAINS(`The 'addInstances' and 'removeInstances' options are deprecated. Please use 'addUnmanaged' and/or 'removeObsolete' instead.`);
WIPE_OUTPUT();

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port2);

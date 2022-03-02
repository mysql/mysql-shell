//@ {VER(>=8.0.11)}

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip});

shell.connect(__sandbox_uri1);
rs = dba.createReplicaSet("rset", {gtidSetIsComplete:true});
rs.addInstance(__sandbox2);

// Tests assuming MD Schema 2.0.0
// ------------------------------

//@<> MD2 - Prepare metadata 2.0
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];

//@<> MD2 - listRouters() - empty
rs.listRouters();

EXPECT_STDOUT_CONTAINS_MULTILINE(`
{
    "replicaSetName": "rset",
    "routers": {}
}
`);

//@<> MD2 - Insert test router instances
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, '', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'r2', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'r3', 'mysqlrouter', 'routerhost1', '8.0.19', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);

session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, '', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', '{\"ROEndpoint\": \"6481\", \"RWEndpoint\": \"6480\", \"ROXEndpoint\": \"6483\", \"RWXEndpoint\": \"6482\"}', ?, NULL, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', '{\"ROEndpoint\": 6481, \"RWEndpoint\": 6480, \"ROXEndpoint\": 6483, \"RWXEndpoint\": 6482}', ?, NULL, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'foobar', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);

//@<> MD2 - listRouters() - full list
// NOTE: whenever new MD versions are added to the shell, the list of upgradeRequired routers will change
rs.listRouters();

EXPECT_STDOUT_CONTAINS_MULTILINE(`
{
    "replicaSetName": "rset",
    "routers": {
        "routerhost1::": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost1::r2": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost1::r3": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "version": "8.0.19"
        },
        "routerhost1::system": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": "6481",
            "roXPort": "6483",
            "rwPort": "6480",
            "rwXPort": "6482",
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost2::": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost2::foobar": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost2::system": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": "6481",
            "roXPort": "6483",
            "rwPort": "6480",
            "rwXPort": "6482",
            "upgradeRequired": true,
            "version": "8.0.18"
        }
    }
}
`);

//@<> MD2 - listRouters() - filtered
rs.listRouters({onlyUpgradeRequired:1});

EXPECT_STDOUT_CONTAINS_MULTILINE(`
{
    "replicaSetName": "rset",
    "routers": {
        "routerhost1::": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost1::r2": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost1::system": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": "6481",
            "roXPort": "6483",
            "rwPort": "6480",
            "rwXPort": "6482",
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost2::": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost2::foobar": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost2::system": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": "6481",
            "roXPort": "6483",
            "rwPort": "6480",
            "rwXPort": "6482",
            "upgradeRequired": true,
            "version": "8.0.18"
        }
    }
}
`);

WIPE_STDOUT();

rs.listRouters({onlyUpgradeRequired:0});

EXPECT_STDOUT_CONTAINS_MULTILINE(`
{
    "replicaSetName": "rset",
    "routers": {
        "routerhost1::": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost1::r2": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost1::r3": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "version": "8.0.19"
        },
        "routerhost1::system": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": "6481",
            "roXPort": "6483",
            "rwPort": "6480",
            "rwXPort": "6482",
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost2::": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost2::foobar": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost2::system": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": "6481",
            "roXPort": "6483",
            "rwPort": "6480",
            "rwXPort": "6482",
            "upgradeRequired": true,
            "version": "8.0.18"
        }
    }
}
`);

//@<> MD2 - removeRouterMetadata() (should fail)
EXPECT_THROWS_TYPE(function() { rs.removeRouterMetadata("bogus"); }, "Invalid router instance 'bogus'", "ArgumentError");
EXPECT_THROWS_TYPE(function() { rs.removeRouterMetadata("bogus::bla"); }, "Invalid router instance 'bogus::bla'", "ArgumentError");
EXPECT_THROWS_TYPE(function() { rs.removeRouterMetadata("routerhost1:r2"); }, "Invalid router instance 'routerhost1:r2'", "ArgumentError");
EXPECT_THROWS_TYPE(function() { rs.removeRouterMetadata("::bla"); }, "Invalid router instance '::bla'", "ArgumentError");
EXPECT_THROWS_TYPE(function() { rs.removeRouterMetadata("foo::bla::"); }, "Invalid router instance 'foo::bla::'", "ArgumentError");
EXPECT_THROWS_TYPE(function() { rs.removeRouterMetadata("127.0.0.1"); }, "Invalid router instance '127.0.0.1'", "ArgumentError");

//@<> MD2 - removeRouterMetadata()
rs.removeRouterMetadata("routerhost1");
rs.removeRouterMetadata("routerhost1::r2");
rs.removeRouterMetadata("routerhost2::system");

EXPECT_THROWS(function(){rs.removeRouterMetadata("routerhost1");}, "Invalid router instance 'routerhost1'");

//@<> MD2 - listRouters() after removed routers
rs.listRouters();

EXPECT_STDOUT_CONTAINS_MULTILINE(`
{
    "replicaSetName": "rset",
    "routers": {
        "routerhost1::r3": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "version": "8.0.19"
        },
        "routerhost1::system": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": "6481",
            "roXPort": "6483",
            "rwPort": "6480",
            "rwXPort": "6482",
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost2::": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost2::foobar": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        }
    }
}
`);

// Non-numeric values for RoPort, RwPort, RoXPort, or RwXPort must not result in an error (BUG#33602309)
shell.connect(__sandbox_uri1);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'bug#33602309', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', '{\"ROEndpoint\": \"abc\", \"RWEndpoint\": \"6480:foo\", \"ROXEndpoint\": \"\", \"RWXEndpoint\": \"foo6482\"}', ?, NULL, NULL)", [cluster_id]);

//@<> listRouters() when endpoint are not numeric values
rs.listRouters();

EXPECT_STDOUT_CONTAINS_MULTILINE(`
{
    "replicaSetName": "rset",
    "routers": {
        "routerhost1::bug#33602309": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": "abc",
            "roXPort": "",
            "rwPort": "6480:foo",
            "rwXPort": "foo6482",
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost1::r3": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "version": "8.0.19"
        },
        "routerhost1::system": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": "6481",
            "roXPort": "6483",
            "rwPort": "6480",
            "rwXPort": "6482",
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost2::": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost2::foobar": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        }
    }
}
`);

// BUG#30594844 : remove_router_metadata() gets primary wrong  --- BEGIN ---
//@<> Connect to SECONDARY and get replicaset.
rs.status();
shell.connect(__sandbox_uri2);
rs2 = dba.getReplicaSet();

//@<> removeRouterMetadata should succeed with current session on SECONDARY (redirected to PRIMARY).
EXPECT_NO_THROWS(function() { rs2.removeRouterMetadata("routerhost2"); });

//@<> Verify router data was removed (routerhost2).
rs.listRouters();

EXPECT_STDOUT_CONTAINS_MULTILINE(`
{
    "replicaSetName": "rset",
    "routers": {
        "routerhost1::bug#33602309": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": "abc",
            "roXPort": "",
            "rwPort": "6480:foo",
            "rwXPort": "foo6482",
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost1::r3": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "version": "8.0.19"
        },
        "routerhost1::system": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": "6481",
            "roXPort": "6483",
            "rwPort": "6480",
            "rwXPort": "6482",
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost2::foobar": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        }
    }
}
`);

// BUG#30594844 : remove_router_metadata() gets primary wrong  --- END ---

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

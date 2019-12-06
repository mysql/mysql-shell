//@<OUT> MD2 - listRouters() - empty
{
    "clusterName": "cluster", 
    "routers": {}
}

//@<OUT> MD2 - listRouters() - full list
{
    "clusterName": "cluster", 
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
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
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
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
            "upgradeRequired": true, 
            "version": "8.0.18"
        }
    }
}

//@<OUT> MD2 - listRouters() - filtered
{
    "clusterName": "cluster", 
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
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
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
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
            "upgradeRequired": true, 
            "version": "8.0.18"
        }
    }
}
{
    "clusterName": "cluster", 
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
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
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
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
            "upgradeRequired": true, 
            "version": "8.0.18"
        }
    }
}

//@<ERR> MD2 - removeRouterMetadata() (should fail)
Cluster.removeRouterMetadata: Invalid router instance 'bogus' (ArgumentError)
Cluster.removeRouterMetadata: Invalid router instance 'bogus::bla' (ArgumentError)
Cluster.removeRouterMetadata: Invalid router instance 'routerhost1:r2' (ArgumentError)
Cluster.removeRouterMetadata: Invalid router instance '::bla' (ArgumentError)
Cluster.removeRouterMetadata: Invalid router instance 'foo::bla::' (ArgumentError)
Cluster.removeRouterMetadata: Invalid router instance '127.0.0.1' (ArgumentError)

//@# MD2 - removeRouterMetadata()
||

//@<OUT> MD2 - listRouters() after removed routers
{
    "clusterName": "cluster", 
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
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
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

//@ removeRouterMetadata should succeed with current session on SECONDARY (redirected to PRIMARY).
||

//@<OUT> Verify router data was removed (routerhost2).
{
    "clusterName": "cluster",
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
            "roPort": 6481,
            "roXPort": 6483,
            "rwPort": 6480,
            "rwXPort": 6482,
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

//@<OUT> MD1 - listRouters() - empty
{
    "clusterName": "mycluster", 
    "routers": {}
}

//@<OUT> MD1 - listRouters() - full list
{
    "clusterName": "mycluster", 
    "routers": {
        "routerhost1::": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }, 
        "routerhost1::r1": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "1.0.0"
        }, 
        "routerhost1::r2": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "8.0.18"
        }, 
        "routerhost1::r3": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "version": "8.0.19"
        }, 
        "routerhost1::system": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::foobar": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::system": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }
    }
}

//@<OUT> MD1 - listRouters() - filtered
{
    "clusterName": "mycluster", 
    "routers": {
        "routerhost1::": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }, 
        "routerhost1::r1": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "1.0.0"
        }, 
        "routerhost1::r2": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "8.0.18"
        }, 
        "routerhost1::system": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::foobar": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::system": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }
    }
}
{
    "clusterName": "mycluster", 
    "routers": {
        "routerhost1::": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }, 
        "routerhost1::r1": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "1.0.0"
        }, 
        "routerhost1::r2": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "8.0.18"
        }, 
        "routerhost1::r3": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "version": "8.0.19"
        }, 
        "routerhost1::system": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::foobar": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::system": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }
    }
}

//@<ERR> MD1 - removeRouterMetadata() (should fail)
Cluster.removeRouterMetadata: Invalid router instance 'bogus' (ArgumentError)
Cluster.removeRouterMetadata: Invalid router instance 'bogus::bla' (ArgumentError)
Cluster.removeRouterMetadata: Invalid router instance 'routerhost1:r2' (ArgumentError)
Cluster.removeRouterMetadata: Invalid router instance '::bla' (ArgumentError)
Cluster.removeRouterMetadata: Invalid router instance 'foo::bla::' (ArgumentError)
Cluster.removeRouterMetadata: Invalid router instance '127.0.0.1' (ArgumentError)

//@# MD1 - removeRouterMetadata()
||

//@<OUT> MD1 - listRouters() after removed routers
{
    "clusterName": "mycluster", 
    "routers": {
        "routerhost1::r1": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "1.0.0"
        },
        "routerhost1::r3": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "version": "8.0.19"
        }, 
        "routerhost1::system": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::foobar": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "<= 8.0.18"
        }
    }
}

//@# MD1 - disconnected cluster object
||The cluster object is disconnected. Please use <Dba>.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please use <Dba>.getCluster to obtain a fresh cluster handle. (RuntimeError)


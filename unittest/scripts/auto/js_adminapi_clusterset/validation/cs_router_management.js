//@<OUT> clusterset.routingOptions() with all defaults
{
    "domainName": "clusterset",
    "global": {
        "invalidated_cluster_policy": "drop_all",
        "target_cluster": "primary"
    },
    "routers": {
        "routerhost1::system": {},
        "routerhost2::": {},
        "routerhost2::another": {},
        "routerhost2::system": {}
    }
}

//@<OUT> clusterset.setRoutingOption for a router, all valid values
Routing option 'target_cluster' successfully updated in router 'routerhost1::system'.
{
    "routerhost1::system": {
        "target_cluster": "primary"
    }
}
Routing option 'target_cluster' successfully updated in router 'routerhost1::system'.
{
    "routerhost1::system": {
        "target_cluster": "cluster"
    }
}
Routing option 'target_cluster' successfully updated in router 'routerhost1::system'.
{
    "routerhost1::system": {
        "target_cluster": "cluster"
    }
}
Routing option 'target_cluster' successfully updated in router 'routerhost1::system'.
{
    "routerhost1::system": {
        "target_cluster": "cluster"
    }
}
Routing option 'target_cluster' successfully updated in router 'routerhost1::system'.
{
    "routerhost1::system": {
        "target_cluster": "cluster"
    }
}
Routing option 'invalidated_cluster_policy' successfully updated in router 'routerhost1::system'.
{
    "routerhost1::system": {
        "invalidated_cluster_policy": "drop_all",
        "target_cluster": "cluster"
    }
}
Routing option 'invalidated_cluster_policy' successfully updated in router 'routerhost1::system'.
{
    "routerhost1::system": {
        "invalidated_cluster_policy": "accept_ro",
        "target_cluster": "cluster"
    }
}

//@<OUT> Resetting router option value for a single router,
Routing option 'target_cluster' successfully updated in router 'routerhost1::system'.
Routing option 'target_cluster' successfully updated in router 'routerhost1::system'.
Routing option 'invalidated_cluster_policy' successfully updated in router 'routerhost1::system'.
Routing option 'invalidated_cluster_policy' successfully updated in router 'routerhost1::system'.
{
    "routerhost1::system": {}
}

//@<OUT> clusterset.setRoutingOption all valid values
Routing option 'target_cluster' successfully updated.
{
    "domainName": "clusterset",
    "global": {
        "invalidated_cluster_policy": "drop_all",
        "target_cluster": "primary"
    },
    "routers": {
        "routerhost1::system": {},
        "routerhost2::": {},
        "routerhost2::another": {},
        "routerhost2::system": {}
    }
}
Routing option 'target_cluster' successfully updated.
{
    "domainName": "clusterset",
    "global": {
        "invalidated_cluster_policy": "drop_all",
        "target_cluster": "cluster"
    },
    "routers": {
        "routerhost1::system": {},
        "routerhost2::": {},
        "routerhost2::another": {},
        "routerhost2::system": {}
    }
}
Routing option 'invalidated_cluster_policy' successfully updated.
{
    "domainName": "clusterset",
    "global": {
        "invalidated_cluster_policy": "drop_all",
        "target_cluster": "cluster"
    },
    "routers": {
        "routerhost1::system": {},
        "routerhost2::": {},
        "routerhost2::another": {},
        "routerhost2::system": {}
    }
}
Routing option 'invalidated_cluster_policy' successfully updated.
{
    "domainName": "clusterset",
    "global": {
        "invalidated_cluster_policy": "accept_ro",
        "target_cluster": "cluster"
    },
    "routers": {
        "routerhost1::system": {},
        "routerhost2::": {},
        "routerhost2::another": {},
        "routerhost2::system": {}
    }
}
Routing option 'target_cluster' successfully updated in router 'routerhost1::system'.
{
    "domainName": "clusterset",
    "global": {
        "invalidated_cluster_policy": "accept_ro",
        "target_cluster": "cluster"
    },
    "routers": {
        "routerhost1::system": {
            "target_cluster": "cluster"
        },
        "routerhost2::": {},
        "routerhost2::another": {},
        "routerhost2::system": {}
    }
}
Routing option 'target_cluster' successfully updated in router 'routerhost2::system'.
{
    "domainName": "clusterset",
    "global": {
        "invalidated_cluster_policy": "accept_ro",
        "target_cluster": "cluster"
    },
    "routers": {
        "routerhost1::system": {
            "target_cluster": "cluster"
        },
        "routerhost2::": {},
        "routerhost2::another": {},
        "routerhost2::system": {
            "target_cluster": "replicacluster"
        }
    }
}
Routing option 'invalidated_cluster_policy' successfully updated in router 'routerhost2::'.
{
    "domainName": "clusterset",
    "global": {
        "invalidated_cluster_policy": "accept_ro",
        "target_cluster": "cluster"
    },
    "routers": {
        "routerhost1::system": {
            "target_cluster": "cluster"
        },
        "routerhost2::": {
            "invalidated_cluster_policy": "accept_ro"
        },
        "routerhost2::another": {},
        "routerhost2::system": {
            "target_cluster": "replicacluster"
        }
    }
}

//@<OUT> Resetting clusterset routing option
Routing option 'target_cluster' successfully updated.
Routing option 'invalidated_cluster_policy' successfully updated.
Routing option 'target_cluster' successfully updated in router 'routerhost2::system'.
{
    "domainName": "clusterset",
    "global": {
        "invalidated_cluster_policy": "drop_all",
        "target_cluster": "primary"
    },
    "routers": {
        "routerhost1::system": {
            "target_cluster": "cluster"
        },
        "routerhost2::": {
            "invalidated_cluster_policy": "accept_ro"
        },
        "routerhost2::another": {},
        "routerhost2::system": {}
    }
}

//@<OUT> clusterset.listRouters
Routing option 'target_cluster' successfully updated in router 'routerhost1::system'.
Routing option 'target_cluster' successfully updated in router 'routerhost2::system'.
{
    "domainName": "clusterset",
    "routers": {
        "routerhost1::system": {
            "hostname": "routerhost1",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": "6481",
            "roXPort": "6483",
            "rwPort": "6480",
            "rwXPort": "6482",
            "targetCluster": "cluster",
            "version": "8.0.27"
        },
        "routerhost2::": {
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "targetCluster": null,
            "version": "8.0.27"
        },
        "routerhost2::another": {
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "targetCluster": null,
            "version": "8.0.27"
        },
        "routerhost2::system": {
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": "mysqlro.sock",
            "roXPort": "mysqlxro.sock",
            "rwPort": "mysql.sock",
            "rwXPort": "mysqlx.sock",
            "targetCluster": "replicacluster",
            "version": "8.0.27"
        }
    }
}
{
    "routerhost1::system": {
        "hostname": "routerhost1",
        "lastCheckIn": "2021-01-01 11:22:33",
        "roPort": "6481",
        "roXPort": "6483",
        "rwPort": "6480",
        "rwXPort": "6482",
        "targetCluster": "cluster",
        "version": "8.0.27"
    }
}
{
    "routerhost2::system": {
        "hostname": "routerhost2",
        "lastCheckIn": "2021-01-01 11:22:33",
        "roPort": "mysqlro.sock",
        "roXPort": "mysqlxro.sock",
        "rwPort": "mysql.sock",
        "rwXPort": "mysqlx.sock",
        "targetCluster": "replicacluster",
        "version": "8.0.27"
    }
}

//@<OUT> clusterset.listRouters() warning re-bootstrap
{
    "domainName": "clusterset",
    "routers": {
        "routerhost1::system": {
            "hostname": "routerhost1",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": "6481",
            "roXPort": "6483",
            "rwPort": "6480",
            "rwXPort": "6482",
            "targetCluster": "cluster",
            "version": "8.0.27"
        },
        "routerhost2::": {
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "targetCluster": "replicacluster",
            "version": "8.0.27"
        },
        "routerhost2::another": {
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "routerErrors": [
                "WARNING: Router needs to be re-bootstraped."
            ],
            "rwPort": null,
            "rwXPort": null,
            "targetCluster": "cluster",
            "version": "8.0.27"
        },
        "routerhost2::system": {
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": "mysqlro.sock",
            "roXPort": "mysqlxro.sock",
            "routerErrors": [
                "WARNING: Router needs to be re-bootstraped."
            ],
            "rwPort": "mysql.sock",
            "rwXPort": "mysqlx.sock",
            "targetCluster": "replicacluster",
            "version": "8.0.27"
        }
    }
}

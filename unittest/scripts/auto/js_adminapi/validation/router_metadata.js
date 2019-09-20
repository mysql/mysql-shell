//@<OUT> listRouters() - empty
{
    "clusterName": "cluster", 
    "routers": {}
}

//@<OUT> listRouters() - full list
{
    "clusterName": "cluster", 
    "routers": {
        "routerhost1": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
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
            "version": "<= 8.0.18"
        }, 
        "routerhost2": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::foobar": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::system": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
            "version": "<= 8.0.18"
        }
    }
}

//@<OUT> listRouters() - filtered
{
    "clusterName": "cluster", 
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
        }
    }
}
{
    "clusterName": "cluster", 
    "routers": {
        "routerhost1": {
            "hostname": "routerhost1", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
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
            "version": "<= 8.0.18"
        }, 
        "routerhost2": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::foobar": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::system": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": 6481, 
            "roXPort": 6483, 
            "rwPort": 6480, 
            "rwXPort": 6482, 
            "version": "<= 8.0.18"
        }
    }
}

//@<ERR> removeRouterMetadata() (should fail)
Cluster.removeRouterMetadata: Invalid router instance 'bogus' (ArgumentError)
Cluster.removeRouterMetadata: Invalid router instance 'bogus::bla' (ArgumentError)
Cluster.removeRouterMetadata: Invalid router instance 'routerhost1:r2' (ArgumentError)
Cluster.removeRouterMetadata: Invalid router instance '::bla' (ArgumentError)
Cluster.removeRouterMetadata: Invalid router instance 'foo::bla::' (ArgumentError)
Cluster.removeRouterMetadata: Invalid router instance '127.0.0.1' (ArgumentError)

//@# removeRouterMetadata()
||

//@<OUT> listRouters() after removed routers
{
    "clusterName": "cluster", 
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
            "version": "<= 8.0.18"
        }, 
        "routerhost2": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "version": "<= 8.0.18"
        }, 
        "routerhost2::foobar": {
            "hostname": "routerhost2", 
            "lastCheckIn": null, 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "version": "<= 8.0.18"
        }
    }
}


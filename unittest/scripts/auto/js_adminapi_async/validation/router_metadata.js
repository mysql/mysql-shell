//@<OUT> MD2 - listRouters() - empty
{
    "replicaSetName": "rset", 
    "routers": {}
}

//@<OUT> MD2 - listRouters() - full list
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
ReplicaSet.removeRouterMetadata: Invalid router instance 'bogus' (ArgumentError)
ReplicaSet.removeRouterMetadata: Invalid router instance 'bogus::bla' (ArgumentError)
ReplicaSet.removeRouterMetadata: Invalid router instance 'routerhost1:r2' (ArgumentError)
ReplicaSet.removeRouterMetadata: Invalid router instance '::bla' (ArgumentError)
ReplicaSet.removeRouterMetadata: Invalid router instance 'foo::bla::' (ArgumentError)
ReplicaSet.removeRouterMetadata: Invalid router instance '127.0.0.1' (ArgumentError)

//@# MD2 - removeRouterMetadata()
||

//@<OUT> MD2 - listRouters() after removed routers
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

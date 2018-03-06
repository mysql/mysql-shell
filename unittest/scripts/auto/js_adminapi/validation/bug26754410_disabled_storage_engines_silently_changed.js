//@<OUT> checkInstanceConfiguration with disabled_storage_engines error.
{
    "config_errors": [
        {
            "action": "config_update+restart", 
            "current": "MyISAM,BLACKHOLE,ARCHIVE", 
            "option": "disabled_storage_engines", 
            "required": "MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE"
        }
    ], 
    "errors": [], 
    "restart_required": true, 
    "status": "error"
}

//@<OUT> configureLocalInstance disabled_storage_engines updated but needs restart.
{
    "config_errors": [
        {
            "action": "restart", 
            "current": "MyISAM,BLACKHOLE,ARCHIVE", 
            "option": "disabled_storage_engines", 
            "required": "MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE"
        }
    ], 
    "errors": [], 
    "restart_required": true, 
    "status": "error"
}

//@<OUT> configureLocalInstance still indicate that a restart is needed.
{
    "config_errors": [
        {
            "action": "restart", 
            "current": "MyISAM,BLACKHOLE,ARCHIVE", 
            "option": "disabled_storage_engines", 
            "required": "MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE"
        }
    ], 
    "errors": [], 
    "restart_required": true, 
    "status": "error"
}

//@ Restart sandbox.
||

//@<OUT> configureLocalInstance no issues after restart.
{
    "status": "ok"
}

//@ Remove disabled_storage_engines option from configuration and restart.
||

//@<OUT> checkInstanceConfiguration no disabled_storage_engines in my.cnf (error).
{
    "config_errors": [
        {
            "action": "config_update+restart",
            "current": "<no value>",
            "option": "disabled_storage_engines",
            "required": "MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE"
        }
    ],
    "errors": [],
    "restart_required": true,
    "status": "error"
}

//@<OUT> configureLocalInstance no disabled_storage_engines in my.cnf (needs restart).
{
    "config_errors": [
        {
            "action": "restart",
            "current": "<no value>",
            "option": "disabled_storage_engines",
            "required": "MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE"
        }
    ],
    "errors": [],
    "restart_required": true,
    "status": "error"
}

//@<OUT> configureLocalInstance no disabled_storage_engines in my.cnf (still needs restart).
{
    "config_errors": [
        {
            "action": "restart",
            "current": "<no value>",
            "option": "disabled_storage_engines",
            "required": "MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE"
        }
    ],
    "errors": [],
    "restart_required": true,
    "status": "error"
}

//@ Restart sandbox again.
||

//@<OUT> configureLocalInstance no issues again after restart.
{
    "status": "ok"
}

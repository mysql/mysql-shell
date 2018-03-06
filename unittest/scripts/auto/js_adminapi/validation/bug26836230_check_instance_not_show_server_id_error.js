//@<OUT> checkInstanceConfiguration with server_id error.
{
    "config_errors": [
        {
            "action": "config_update+restart", 
            "current": "0", 
            "option": "server_id", 
            "required": "<unique ID>"
        }
    ], 
    "errors": [], 
    "restart_required": true, 
    "status": "error"
}

//@<OUT> configureLocalInstance server_id updated but needs restart.
{
    "config_errors": [
        {
            "action": "restart",
            "current": "0",
            "option": "server_id",
            "required": "<unique ID>"
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
            "current": "0",
            "option": "server_id",
            "required": "<unique ID>"
        }
    ],
    "errors": [],
    "restart_required": true,
    "status": "error"
}

//@ Restart sandbox 1.
||

//@<OUT> configureLocalInstance no issues after restart for sandobox 1.
{
    "status": "ok"
}

//@<OUT> checkInstanceConfiguration no server_id in my.cnf (error). {VER(>=8.0.3)}
{
    "config_errors": [
        {
            "action": "restart",
            "current": "1",
            "option": "server_id",
            "required": "<unique ID>"
        }
    ],
    "errors": [],
    "restart_required": true,
    "status": "error"
}

//@<OUT> configureLocalInstance no server_id in my.cnf (needs restart). {VER(>=8.0.3)}
{
    "config_errors": [
        {
            "action": "restart",
            "current": "1",
            "option": "server_id",
            "required": "<unique ID>"
        }
    ],
    "errors": [],
    "restart_required": true,
    "status": "error"
}

//@<OUT> configureLocalInstance no server_id in my.cnf (still needs restart). {VER(>=8.0.3)}
{
    "config_errors": [
        {
            "action": "restart",
            "current": "1",
            "option": "server_id",
            "required": "<unique ID>"
        }
    ],
    "errors": [],
    "restart_required": true,
    "status": "error"
}

//@ Restart sandbox 2. {VER(>=8.0.3)}
||

//@<OUT> configureLocalInstance no issues after restart for sandbox 2. {VER(>=8.0.3)}
{
    "status": "ok"
}

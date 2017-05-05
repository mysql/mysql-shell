#@ Initialization
||

#@<OUT> check instance with invalid parallel type.
{
    "config_errors": [
        {
            "action": "server_update",
            "current": "DATABASE",
            "option": "slave_parallel_type",
            "required": "LOGICAL_CLOCK"
        }
    ],
    "errors": [],
    "restart_required": false,
    "status": "error"
}

#@ Create cluster (succeed: parallel type updated).
||

#@<OUT> check instance with invalid commit order.
{
    "config_errors": [
        {
            "action": "server_update",
            "current": "OFF",
            "option": "slave_preserve_commit_order",
            "required": "ON"
        }
    ],
    "errors": [],
    "restart_required": false,
    "status": "error"
}

#@ Adding instance to cluster (succeed: commit order updated).
||

#@<OUT> check instance with invalid type and commit order.
{
    "config_errors": [
        {
            "action": "server_update",
            "current": "DATABASE",
            "option": "slave_parallel_type",
            "required": "LOGICAL_CLOCK"
        },
        {
            "action": "server_update",
            "current": "OFF",
            "option": "slave_preserve_commit_order",
            "required": "ON"
        }
    ],
    "errors": [],
    "restart_required": false,
    "status": "error"
}

#@<OUT> configure instance and update type and commit order with valid values.
{
    "status": "ok"
}

#@<OUT> check instance, no invalid values after configure.
{
    "status": "ok"
}

#@ Adding instance to cluster (succeed: nothing to update).
||

#@ Finalization
||

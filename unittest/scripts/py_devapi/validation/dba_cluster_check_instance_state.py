#@ Initialization
||

#@ Connect
||

#@<OUT> check_instance_state: two arguments
{
    "reason": "{{recoverable|new}}",
    "state": "ok"
}

#@<OUT> check_instance_state: single argument
{
    "reason": "{{recoverable|new}}",
    "state": "ok"
}

#@ Failure: no arguments
||SystemError: ArgumentError: Invalid number of arguments in Cluster.check_instance_state, expected 1 to 2 but got 0

#@ Failure: more than two arguments
||SystemError: ArgumentError: Invalid number of arguments in Cluster.check_instance_state, expected 1 to 2 but got 3

#@ create cluster
||

#@ Adding instance
||

#@<OUT> check_instance_state: two arguments - added instance
{
    "reason": "{{recoverable|new}}",
    "state": "ok"
}

#@<OUT> check_instance_state: single argument - added instance
{
    "reason": "{{recoverable|new}}",
    "state": "ok"
}

#@ Failure: no arguments - added instance
||SystemError: ArgumentError: Invalid number of arguments in Cluster.check_instance_state, expected 1 to 2 but got 0

#@ Failure: more than two arguments - added instance
||SystemError: ArgumentError: Invalid number of arguments in Cluster.check_instance_state, expected 1 to 2 but got 3

#@ Finalization
||


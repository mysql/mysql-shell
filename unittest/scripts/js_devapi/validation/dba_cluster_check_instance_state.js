//@ Initialization
||

//@ Connect
||

//@ create cluster
||

//@<OUT> checkInstanceState: two arguments
{
    "reason": "{{recoverable|new}}",
    "state": "ok"
}

//@<OUT> checkInstanceState: single argument
{
    "reason": "{{recoverable|new}}",
    "state": "ok"
}

//@ Failure: no arguments
||Cluster.checkInstanceState: Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)

//@ Failure: more than two arguments
||Cluster.checkInstanceState: Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)

//@ Adding instance
||

//@<OUT> checkInstanceState: two arguments - added instance
{
    "reason": "{{recoverable|new}}",
    "state": "ok"
}

//@<OUT> checkInstanceState: single argument - added instance
{
    "reason": "{{recoverable|new}}",
    "state": "ok"
}

//@ Failure: no arguments - added instance
||Cluster.checkInstanceState: Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)

//@ Failure: more than two arguments - added instance
||Cluster.checkInstanceState: Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)

//@ Finalization
||

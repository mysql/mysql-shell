// Assumptions: smart deployment routines available
//@ Initialization
var connected = connect_to_sandbox([__mysql_sandbox_port1]);
if (connected)
    cleanup_sandbox(__mysql_sandbox_port1);

//@<OUT> Deploy sandbox
dba.deploySandboxInstance(__mysql_sandbox_port1, {sandboxDir:__sandbox_dir});

//@ Finalization
if (!connected)
    // if sandbox was not available when test start, clean it
    cleanup_sandbox(__mysql_sandbox_port1);

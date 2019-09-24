// Assumptions: smart deployment routines available
//@ Initialization
var connected = connect_to_sandbox([__mysql_sandbox_port1]);
if (connected)
    cleanup_sandbox(__mysql_sandbox_port1);

//@<OUT> Deploy sandbox
dba.deploySandboxInstance(__mysql_sandbox_port1, {sandboxDir:__sandbox_dir});

//@ BUG#28624006: Deploy sandbox with non-existing dir
dba.deploySandboxInstance(__mysql_sandbox_port1, {sandboxDir: __sandbox_dir + "invalid"});

//@ BUG#28624006: Deploy sandbox with file instead of directory
var file = __sandbox_dir + "/file";
testutil.touch(file);
dba.deploySandboxInstance(__mysql_sandbox_port1, {sandboxDir: file});
testutil.rmfile(file);

//@ Finalization
if (!connected)
    // if sandbox was not available when test start, clean it
    cleanup_sandbox(__mysql_sandbox_port1);

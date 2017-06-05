// Assumptions: smart deployment routines available

//@ Check Instance Configuration must work without a session
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, password:'root'});

//@ First Sandbox
reset_or_deploy_sandbox(__mysql_sandbox_port1);

//@ Check Instance Configuration ok with a session
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, password:'root'});

// Remove the sandbox
cleanup_sandbox(__mysql_sandbox_port1);

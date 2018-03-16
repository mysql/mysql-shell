//@ ConfigureLocalInstance should fail if there's no session nor parameters provided
||An open session is required to perform this operation.

//@# Error no write privileges
|The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB cluster.|
|ERROR: mycnfPath is not writable: <<<cnfPath>>>: Permission denied|
||Dba.configureLocalInstance: Unable to update MySQL configuration file

//@ Close session
||

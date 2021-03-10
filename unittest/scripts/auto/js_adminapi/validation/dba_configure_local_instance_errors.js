//@ ConfigureLocalInstance should fail if there's no session nor parameters provided
||An open session is required to perform this operation.

//@# Error no write privileges
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB cluster.|
|ERROR: mycnfPath is not writable: <<<cnfPath>>>: Permission denied|
||Unable to update MySQL configuration file

//@ Close session
||

//@ IPv6 not supported on versions below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot use host '::1' for instance '[::1]:<<<__mysql_sandbox_port1>>>' because it is an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14. Set the MySQL server 'report_host' variable to an IPv4 address or hostname that resolves an IPv4 address.|
||Unsupported IP address '::1'. IPv6 is only supported by Group Replication on MySQL version >= 8.0.14. (RuntimeError)

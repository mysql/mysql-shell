//@ deploy raw_sandbox, fix all issues and then disable log_bin (BUG#27305806) {VER(>=8.0.11)}
||

//@ ConfigureLocalInstance should detect path of configuration file by looking at the server (BUG#27305806) {VER(>=8.0.11)}
|Found configuration file being used by instance 'localhost:<<<__mysql_sandbox_port1>>>' at location: <<<mycnf_path>>>|
||Dba.configureLocalInstance: Cancelled

//@ Cleanup (BUG#27305806) {VER(>=8.0.11)}
||

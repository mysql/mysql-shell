
//@# mysql.getSession() socket, environment variable {__os_type != "windows"}
|root@localhost?ssl-mode=required <<<__mysql_sandbox_port1>>>|

//@# mysql.getSession() socket, environment variable, try override {__os_type != "windows"}
|root@localhost?ssl-mode=required <<<__mysql_sandbox_port1>>>|

//@# shell.connect() socket, environment variable {__os_type != "windows"}
|mysql://root@/<<<sockpath1.substr(1).replace(/\//g, "%2F")>>> <<<__mysql_sandbox_port1>>>|

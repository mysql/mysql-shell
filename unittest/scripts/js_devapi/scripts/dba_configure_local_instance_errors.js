//@<OUT> Error no write privileges
var cnfPath = __sandbox_dir + __mysql_sandbox_port1 + "/my.cnf";
dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port1, {mycnfPath:cnfPath, password:'root'});

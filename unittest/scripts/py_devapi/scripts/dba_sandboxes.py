# Assumptions: smart deployment routines available

#@ Deploy sandbox in dir with space
test_dir = __sandbox_dir + __path_splitter + "foo \' bar"
dba.deploy_sandbox_instance(__mysql_sandbox_port1, {'sandboxDir': test_dir, 'password': 'root'})

#@ Stop sandbox in dir with space
dba.stop_sandbox_instance(__mysql_sandbox_port1, {'sandboxDir': test_dir, 'password': 'root'})

#@ Delete sandbox in dir with space
try_delete_sandbox(__mysql_sandbox_port1, test_dir)

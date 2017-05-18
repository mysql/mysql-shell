# Assumptions: smart deployment routines available

#@ Deploy sandbox in dir with space
test_dir = __sandbox_dir + __path_splitter + "foo \' bar"
dba.deploy_sandbox_instance(__mysql_sandbox_port1, {'sandboxDir': test_dir, 'password': 'root'})

#@ Stop sandbox in dir with space
dba.stop_sandbox_instance(__mysql_sandbox_port1, {'sandboxDir': test_dir, 'password': 'root'})

#@ Delete sandbox in dir with space
try_delete_sandbox(__mysql_sandbox_port1, test_dir)

# Regression for BUG25485035:
# Deployment using paths longer than 89 chars should succeed.
#@ Deploy sandbox in dir with long path
test_dir = __sandbox_dir + __path_splitter + "012345678911234567892123456789312345678941234567895123456789612345678971234567898123456789"
dba.deploy_sandbox_instance(__mysql_sandbox_port1, {'sandboxDir': test_dir, 'password': 'root'})

#@ Stop sandbox in dir with long path
dba.stop_sandbox_instance(__mysql_sandbox_port1, {'sandboxDir': test_dir, 'password': 'root'})

#@ Delete sandbox in dir with long path
try_delete_sandbox(__mysql_sandbox_port1, test_dir)


#@<> Invalid config path check
util.check_for_server_upgrade(__mysql_uri, {"configPath": "/inv/alid"})
EXPECT_OUTPUT_CONTAINS("Invalid config path: /inv/alid")
WIPE_OUTPUT()

#@<> Valid config path check
cnf_path = os.path.join(__tmp_dir, 'test_cnf.cnf')
file = open(cnf_path, "w")
file.close()

util.check_for_server_upgrade(__mysql_uri, {"configPath": cnf_path})
EXPECT_STDOUT_NOT_CONTAINS("Invalid config path: " + cnf_path)
EXPECT_STDOUT_NOT_CONTAINS("Invalid config path")

os.remove(cnf_path)
WIPE_OUTPUT()

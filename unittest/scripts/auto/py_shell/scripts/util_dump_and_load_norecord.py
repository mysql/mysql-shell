# Special case integration tests of dump and load

#@<> INCLUDE dump_utils.inc

#@<> Setup
import time

outdir=__tmp_dir+"/ldtest"
try:
    testutil.rmdir(outdir, True)
except:
    pass
testutil.mkdir(outdir)


def prepare(sbport, options={}):
    # load test schemas
    uri="mysql://root:root@localhost:%s"%sbport
    datadir=__tmp_dir+"/ldtest/datadirs%s"%sbport
    testutil.mkdir(datadir)
    # 8.0 seems to create dirs automatically but not 5.7
    if __version_num < 80000:
        testutil.mkdir(datadir+"/testindexdir")
        testutil.mkdir(datadir+"/test datadir")
    options.update({
        "loose_innodb_directories": datadir,
        "early_plugin_load": "keyring_file."+("dll" if __os_type=="windows" else "so"),
        "keyring_file_data": datadir+"/keyring",
        "local_infile": "1"
    })
    testutil.deploy_sandbox(sbport, "root", options)

prepare(__mysql_sandbox_port1)
session1 = mysql.get_session(__sandbox_uri1)
session1.run_sql("set names utf8mb4")
session1.run_sql("create schema world")
testutil.import_data(__sandbox_uri1, __data_path+"/sql/world.sql", "world")

prepare(__mysql_sandbox_port2)
session2 = mysql.get_session(__sandbox_uri2)
session2.run_sql("set names utf8mb4")

#@<> Dump ddlOnly
shell.connect(__sandbox_uri1)
util.dump_instance(outdir+"/ddlonly", {"ddlOnly":True})

shell.connect(__sandbox_uri2)

#@<> load data which is not in the dump (fail)
util.load_dump(outdir+"/ddlonly", {"loadData":True, "loadDdl":False})

#@<> load ddl normally
util.load_dump(outdir+"/ddlonly")

compare_servers(session1, session2, check_rows=False)

#@<> Dump dataOnly
shell.connect(__sandbox_uri1)
util.dump_instance(outdir+"/dataonly", {"dataOnly":True})

shell.connect(__sandbox_uri2)

#@<> load data assuming tables already exist

# will fail because tables already exist
EXPECT_THROWS(lambda: util.load_dump(outdir+"/dataonly", {"dryRun":True}), "RuntimeError: Util.load_dump: Duplicate objects found in destination database")

# will pass because we're explicitly only loading data
util.load_dump(outdir+"/dataonly", {"dryRun":True, "loadDdl":False})

util.load_dump(outdir+"/dataonly", {"ignoreExistingObjects":True})

compare_servers(session1, session2)

wipeout_server(session2)

#@<> load ddl which is not in the dump (fail)
util.load_dump(outdir+"/dataonly", {"loadData":False, "loadDdl":True})


#@<> Source default and data is latin1, destination default is utf8mb4


#@<> Big blob in source, check max_allowed_packet in server and client


#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.rmdir(__tmp_dir+"/ldtest", True)

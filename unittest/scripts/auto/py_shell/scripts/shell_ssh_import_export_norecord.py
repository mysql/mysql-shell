#@ {False}
# TODO(rennox): Enable this once you are find the way to succeed on schema comparisins for the utf differences
#@ {has_ssh_environment()}

from _ssh_utils import *
from pathlib import Path

#@<> INCLUDE dump_utils.inc

#@<> setup
shell.options.useWizards = True

import random
import datetime
def random_date():
    s = datetime.datetime.strptime("1970-01-01", "%Y-%m-%d")
    e = datetime.datetime.strptime("2021-05-13 15:16:35", "%Y-%m-%d %H:%M:%S")
    return (s + datetime.timedelta(seconds=random.randint(0, (e-s).total_seconds()))).strftime("%Y-%m-%d %H:%M:%S")

if __os_type != "windows":
    def filename_for_output(filename):
        return filename
else:
    def filename_for_output(filename):
        long_path_prefix = r"\\?" "\\"
        return long_path_prefix + filename.replace("/", "\\")

outdir = os.path.join(__tmp_dir, "ldtest")
try:
    testutil.rmdir(outdir, True)
except:
    pass
testutil.mkdir(outdir)

shell.options.set("logLevel", 8)


known_hosts_file = reset_tmp_file("known_hosts_import")
config_file = reset_tmp_file("ssh_config_import")

create_ssh_conf(config_file, f"""
Host *
    UserKnownHostsFile {known_hosts_file}
    PubkeyAuthentication no
""")

def clean_server(a_session):
  result = a_session.run_sql("show databases")
  schemas = result.fetch_all()
  for schema in schemas:
    if not schema[0] in ["information_schema", "mysql", "performance_schema", "sys"]:
      a_session.run_sql("DROP SCHEMA IF EXISTS " + schema[0])

#@<> Initial connection, adds the server to the known hosts file
shell.options.useWizards = True
testutil.expect_prompt("The authenticity of host '{}' can't be established.".format(SSH_HOST), "y")
shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-password": SSH_PASS, "ssh-CONFIG-file": config_file})
EXPECT_TRUE(session.is_open(), "Unable to open connection using shell.connect through SSH tunnel")
clean_server(session)
session.close()

#@<> test import_table over SSH
shell.options["useWizards"] = False
EXPECT_EQ(0, len(shell.list_ssh_connections()))
shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-password": SSH_PASS, "ssh-config-file": config_file})
EXPECT_TRUE(session.is_open(), "Unable to open connection using shell.connect through SSH tunnel using password")
target_schema = "test_import"
session.run_sql("CREATE SCHEMA {}".format(target_schema))
session.run_sql("USE {}".format(target_schema))
session.run_sql("""CREATE TABLE `cities` (`ID` int(11) NOT NULL AUTO_INCREMENT, `Name` char(64) NOT NULL DEFAULT '', `CountryCode` char(3) NOT NULL DEFAULT '', `District` char(64) NOT NULL DEFAULT '', `Info` json DEFAULT NULL, PRIMARY KEY (`ID`)) ENGINE=InnoDB AUTO_INCREMENT=4080 DEFAULT CHARSET=utf8mb4""")
session.run_sql('SET GLOBAL local_infile = true')
EXPECT_NO_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "schema": target_schema, "table": 'cities' }), "Unable to import table")
session.run_sql("DROP SCHEMA {}".format(target_schema))
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(os.path.join(__import_data_path, "world_x_cities.dump")) + "' (209.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")
session.close()
EXPECT_EQ(0, len(shell.list_ssh_connections()))

#@<> test import/dump through ssh connection {VER(> 8.0.0)}

def load_dump_on_remote(dump_name):
  shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-password": SSH_PASS, "ssh-config-file": config_file})
  EXPECT_TRUE(session.is_open(), "Unable to open connection using shell.connect through SSH tunnel using password")
  session.run_sql("SET GLOBAL local_infile = ON")
  EXPECT_NO_THROWS(lambda: util.load_dump(os.path.join(outdir, dump_name), {"ignoreVersion": True}), "Unable to load dump")
  session.close()

def dump_remote_instance(dump_name):
  shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-password": SSH_PASS, "ssh-config-file": config_file})
  EXPECT_TRUE(session.is_open(), "Unable to open connection using shell.connect through SSH tunnel using password")
  EXPECT_NO_THROWS(lambda: util.dump_instance(os.path.join(outdir, dump_name)), "Unable to dump instance")
  session.close()

def compare_instances():
  local_session = mysql.get_session(__sandbox_uri1)
  remote_session = shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-password": SSH_PASS, "ssh-config-file": config_file})
  compare_servers(local_session, remote_session, check_rows=False, check_users=False)
  local_session.close()
  remote_session.close()


# 1: Creates a sandbox with some data and creates a dump
shell.options["useWizards"] = False
testutil.deploy_sandbox(__mysql_sandbox_port1, "root", {"local_infile":1})
shell.connect(__sandbox_uri1)
clean_server(session)
session.run_sql("set names utf8mb4")
session.run_sql("create schema sakila")
testutil.import_data(__sandbox_uri1, os.path.join(__data_path, "sql", "sakila-schema.sql"), "sakila")
testutil.import_data(__sandbox_uri1, os.path.join(__data_path, "sql", "sakila-data.sql"), "sakila")
EXPECT_NO_THROWS(lambda: util.dump_instance(os.path.join(outdir, "dump_from_local")), "Unable to dump instance")
session.close()
EXPECT_EQ(0, len(shell.list_ssh_connections()))

# 2: Connects to the instance over SSH to load the dump
load_dump_on_remote("dump_from_local")
EXPECT_EQ(0, len(shell.list_ssh_connections()))

# 3: Compares the data in sandbox and the instance over SSH
compare_instances()
EXPECT_EQ(0, len(shell.list_ssh_connections()))

# 4: Now creates a dump from the instance over SSH
dump_remote_instance("dump_from_ssh")
EXPECT_EQ(0, len(shell.list_ssh_connections()))

# 6 Cleans the data and load it from the ssh dump
shell.connect(__sandbox_uri1)
clean_server(session)
EXPECT_NO_THROWS(lambda: util.load_dump(os.path.join(outdir, "dump_from_ssh"), {"ignoreVersion": True}), "Unable to load dump")
session.close()


# 8 Compares both instances again
compare_instances()
EXPECT_EQ(0, len(shell.list_ssh_connections()))


# Clean the data in both places
local_session = mysql.get_session(__sandbox_uri1)
remote_session = mysql.get_session({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-password": SSH_PASS, "ssh-config-file": config_file})
clean_server(local_session)
clean_server(remote_session)
local_session.close()
remote_session.close()
EXPECT_EQ(0, len(shell.list_ssh_connections()))

#@<> Create some random data so we can test if dump doesn't have problems when big table ~ 100k rows is used {False}
shell.connect(__sandbox_uri1)
session.run_sql("CREATE SCHEMA randomdata")
tmp_sql = os.path.join(__tmp_dir, "tmp.sql")
with open(tmp_sql, "w") as f:
  f.write("CREATE TABLE randomdata.randtable (id INT PRIMARY KEY AUTO_INCREMENT, sdate DATETIME, edate DATETIME, rnumber INT);\n")
  i = 0
  f.write("INSERT INTO randomdata.randtable (sdate, edate, rnumber) VALUES")
  nrows = 100000
  parts = nrows / 1000
  while i < nrows:
    if i % parts  == 0 and i != 0:
      f.write(";\n")
      f.write("INSERT INTO randomdata.randtable (sdate, edate, rnumber) VALUES")
      comma = False
    elif i % parts == parts - 1:
      f.write('("{0}","{1}",{2})'.format(random_date(), random_date(), random.randint(0,2000)))
    else:
      f.write('("{0}","{1}",{2})'.format(random_date(), random_date(), random.randint(0,2000)))
      f.write(",")
    i += 1
  f.write(";")

EXPECT_NO_THROWS(lambda: util.dump_instance(os.path.join(outdir, "dump_from_local_big")), "Unable to dump instance")


# 2: Connects to the instance over SSH to load the dump
load_dump_on_remote("dump_from_local_big")
EXPECT_EQ(0, len(shell.list_ssh_connections()))

# 3: Compares the data in sandbox and the instance over SSH
compare_instances()
EXPECT_EQ(0, len(shell.list_ssh_connections()))

# 4: Now creates a dump from the instance over SSH
dump_remote_instance("dump_from_ssh_big")
EXPECT_EQ(0, len(shell.list_ssh_connections()))

# 6 Cleans the data and load it from the ssh dump
shell.connect(__sandbox_uri1)
clean_server(session)
EXPECT_NO_THROWS(lambda: util.load_dump(os.path.join(outdir, "dump_from_ssh_big"), {"ignoreVersion": True}), "Unable to load dump")
session.close()

# 8 Compares both instances again
compare_instances()
EXPECT_EQ(0, len(shell.list_ssh_connections()))


# Clean the data in both places
local_session = mysql.get_session(__sandbox_uri1)
remote_session = mysql.get_session({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-password": SSH_PASS, "ssh-config-file": config_file})
clean_server(local_session)
clean_server(remote_session)
local_session.close()
remote_session.close()
EXPECT_EQ(0, len(shell.list_ssh_connections()))

#2) Loads a dump using X protocol
shell.connect({"uri": f"mysqlx://{MYSQL_OVER_SSH_URI}0", "ssh": SSH_URI_NOPASS, "ssh-password": SSH_PASS, "ssh-config-file": config_file})
EXPECT_TRUE(session.is_open(), "Unable to open connection using shell.connect through SSH tunnel using password")
EXPECT_NO_THROWS(lambda: util.load_dump(os.path.join(outdir, "dump_from_ssh"), {"ignoreVersion": True}), "Unable to load dump")
# TODO(rennox): Fix SSH tunnel handling on multiple threads
#EXPECT_EQ(1, len(shell.list_ssh_connections()))

#3) Creates a dump using X protocol
EXPECT_NO_THROWS(lambda: util.dump_instance(os.path.join(outdir, "dump_from_ssh_x")), "Unable to dump instance")
session.close()
EXPECT_STDOUT_CONTAINS("Schemas dumped: 1")
#EXPECT_EQ(1, len(shell.list_ssh_connections()))

# check if ssh connections are properly closed, since util.*dump create multiple connections.
testutil.stop_sandbox(__mysql_sandbox_port1, {"wait":1})
testutil.destroy_sandbox(__mysql_sandbox_port1)

#@ {has_ssh_environment()}

from _ssh_utils import *
from pathlib import Path
import shutil
import os

known_hosts_file = reset_tmp_file("known_hosts_a")
config_file = reset_tmp_file("ssh_config_a")

create_ssh_conf(config_file, f"""
Host *
    UserKnownHostsFile {known_hosts_file}
    PubkeyAuthentication no
""")

shell.options["credentialStore.helper"] = "plaintext"

#@<> Initial connection, adds the server to the known hosts file
shell.options.useWizards = True
testutil.expect_prompt("The authenticity of host '{}' can't be established.".format(SSH_HOST), "y")
testutil.expect_prompt("Save password for 'ssh://{}'? ".format(SSH_URI_NOPASS), "n")
shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-password" : SSH_PASS, "ssh-CONFIG-file": config_file})
EXPECT_TRUE(session.is_open(), "Unable to open connection using shell.connect through SSH tunnel")


#@<>  WL#14246-TSFR_5_1 Check if dba functions are properly protected from using ssh connection
funcs = ["check_instance_configuration","configure_instance","configure_local_instance","configure_replica_set_instance","drop_metadata_schema","get_cluster","get_replica_set","reboot_cluster_from_complete_outage","upgrade_metadata"]
for f in funcs:
    EXPECT_THROWS(lambda: dba[f](), "InnoDB cluster functionality is not available through SSH tunneling")

funcs = ["create_cluster","create_replica_set"]
for f in funcs:
    EXPECT_THROWS(lambda: dba[f]("test"), "InnoDB cluster functionality is not available through SSH tunneling")


#@<>  WL#14246-TSFR_5_2 connect to the sshd and test sandbox commands
testutil.expect_prompt("Save password for 'ssh://{}'? ".format(SSH_URI_NOPASS), "N")
sess = shell.connect({"uri": MYSQL_OVER_SSH_URI,'ssh-password':SSH_PASS,
                "ssh": SSH_URI_NOPASS,
                "ssh-config-file": config_file})

EXPECT_TRUE(sess.is_open(), "Unable to open connection using shell.connect through SSH tunnel")

test_dir = os.path.join(testutil.get_sandbox_path(), "ssh_tests")
try:
    if os.path.exists(test_dir):
        shutil.rmtree(test_dir)
except OSError as e:
    print ("Error: %s - %s." % (e.filename, e.strerror))
os.makedirs(test_dir)

dba.deploy_sandbox_instance(__mysql_sandbox_port1, {"password": "root", "sandboxDir": test_dir})
dba.stop_sandbox_instance(__mysql_sandbox_port1, {"password": "root", "sandboxDir": test_dir})
dba.start_sandbox_instance(__mysql_sandbox_port1, {"sandboxDir": test_dir})
dba.kill_sandbox_instance(__mysql_sandbox_port1, {"sandboxDir": test_dir})
testutil.wait_sandbox_dead(__mysql_sandbox_port1)
dba.delete_sandbox_instance(__mysql_sandbox_port1, {"sandboxDir": test_dir})

session.close()
WIPE_STDOUT()


#@<> WL#14246 TSFR_5_3 Call the AdminAPI functions from the command line using the API command line integration while creating a SSH connection.
shell.store_credential("ssh://{}".format(SSH_URI_NOPASS), SSH_PASS)
funcs = ["check-instance-configuration", "configure-instance", "configure-local-instance", "configure-replica-set-instance", "drop-metadata-schema", "reboot-cluster-from-complete-outage", "upgrade-metadata"]
for f in funcs:
    testutil.call_mysqlsh(["--credential-store-helper=plaintext", "--ssh", SSH_URI_NOPASS, MYSQL_OVER_SSH_URI, "--ssh-config-file", config_file, "--", "dba", f], "", None, os.path.join(__bin_dir, "mysqlsh"))
    EXPECT_STDOUT_CONTAINS("InnoDB cluster functionality is not available through SSH tunneling")
    WIPE_STDOUT()


#@<> WL#14246 TSFR_5_4 Call the AdminAPI functions  from the command line using the file option (-f) while creating a SSH connection.
dba_test = os.path.join(__tmp_dir, "dba_test.js")
if os.path.exists(dba_test):
    os.remove(dba_test)

funcs = ["checkInstanceConfiguration","configureInstance","configureLocalInstance","configureReplicaSetInstance","dropMetadataSchema","getCluster","getReplicaSet","rebootClusterFromCompleteOutage","upgradeMetadata"]
for fun in funcs:
    with open(dba_test, "w") as file:
        file.write("dba.{}();".format(fun))
    testutil.call_mysqlsh(["--credential-store-helper=plaintext", "--ssh", SSH_URI_NOPASS, MYSQL_OVER_SSH_URI, "--ssh-config-file", config_file, "-f", dba_test], "", None, os.path.join(__bin_dir, "mysqlsh"))
    EXPECT_STDOUT_CONTAINS("InnoDB cluster functionality is not available through SSH tunneling")
    WIPE_STDOUT()

funcs = ["createCluster","createReplicaSet"]
for fun in funcs:
    with open(dba_test, "w") as file:
        file.write("dba.{}('test');".format(fun))
    testutil.call_mysqlsh(["--credential-store-helper=plaintext", "--ssh", SSH_URI_NOPASS, MYSQL_OVER_SSH_URI, "--ssh-config-file", config_file, "-f", dba_test], "", None, os.path.join(__bin_dir, "mysqlsh"))
    EXPECT_STDOUT_CONTAINS("InnoDB cluster functionality is not available through SSH tunneling")
    WIPE_STDOUT()

if os.path.exists(dba_test):
    os.remove(dba_test)

#@<> WL#14246 TSFR_5_5 Call the AdminAPI functions from the command line using the execute option (-e) while creating a SSH connection.
funcs = ["checkInstanceConfiguration","configureInstance","configureLocalInstance","configureReplicaSetInstance","dropMetadataSchema","getCluster","getReplicaSet","rebootClusterFromCompleteOutage","upgradeMetadata"]
for fun in funcs:
    testutil.call_mysqlsh(["--credential-store-helper=plaintext", "--ssh", SSH_URI_NOPASS, MYSQL_OVER_SSH_URI, "--ssh-config-file", config_file, "-e", "dba.{}();".format(fun)], "", None, os.path.join(__bin_dir, "mysqlsh"))
    EXPECT_STDOUT_CONTAINS("InnoDB cluster functionality is not available through SSH tunneling")
    WIPE_STDOUT()

funcs = ["createCluster","createReplicaSet"]
for fun in funcs:
    testutil.call_mysqlsh(["--credential-store-helper=plaintext", "--ssh", SSH_URI_NOPASS, MYSQL_OVER_SSH_URI, "--ssh-config-file", config_file, "-e", "dba.{}('test');".format(fun)], "", None, os.path.join(__bin_dir, "mysqlsh"))
    EXPECT_STDOUT_CONTAINS("InnoDB cluster functionality is not available through SSH tunneling")
    WIPE_STDOUT()


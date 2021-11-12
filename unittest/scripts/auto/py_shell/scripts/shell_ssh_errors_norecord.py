#@ {has_ssh_environment()}

from _ssh_utils import *
from pathlib import Path

#@<> setup
shell.options.useWizards = True

# Utility Files
known_hosts_file = reset_tmp_file("known_hosts_e")
config_file = reset_tmp_file("ssh_config_e")
symlinked_key = reset_tmp_file("symlinked_key")
key_with_pw = os.path.join(__test_data_path, "docker", "keys","with_password")
key_without_pw = os.path.join(__test_data_path, "docker", "keys","no_password")
os.chmod(os.path.join(__test_data_path, "docker", "keys"), 0o700)
os.chmod(key_with_pw, 0o600)
os.chmod(key_without_pw, 0o600)

# TODO(rennox): These should be customizable as well
if __os_type != "windows":
  key_with_pw_uri = "file:{}".format(key_with_pw)
else:
  key_with_pw_uri = "file:/{}".format(key_with_pw)


shell.options.set("logLevel", 8)

#@<> Check config file wrong permissions
default_config = f"""
Host *
    UserKnownHostsFile {known_hosts_file}
"""
create_ssh_conf(config_file, default_config, False)
EXPECT_THROWS(lambda: shell.connect({"uri": MYSQL_OVER_SSH_URI,
               "ssh": SSH_URI_NOPASS, "ssh-config-file": config_file}), "Invalid SSH configuration file: '{}' has insecure permissions. Expected u+rw only: Permission denied".format(config_file))

# Updates the file with the right permissions
create_ssh_conf(config_file, default_config)
WIPE_STDOUT()

#@<> Tunnel Creation Errors - Missing target MySQL URI
testutil.call_mysqlsh(["--ssh", SSH_URI_NOPASS])
EXPECT_STDOUT_CONTAINS("SSH tunnel can't be started because DB URI is missing")
WIPE_STDOUT()

#@<> Tunnel Creation Errors - Invalid SSH URI
testutil.call_mysqlsh(["--ssh", "invalid_user:ssh_pwd@invalid_host name", MYSQL_OVER_SSH_URI])
EXPECT_STDOUT_CONTAINS("Invalid URI: Illegal space found at position 33")
WIPE_STDOUT()

#@<> Tunnel Creation Errors - Non registered server non interactive
shell.options["useWizards"] = False
EXPECT_THROWS(lambda: shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-CONFIG-file": config_file}),
  "The authenticity of host '{}' can't be established.".format(SSH_HOST))
testutil.assert_no_prompts()

#@<> Tunnel Creation Errors - Non registered server interactive - rejecting the new server
shell.options["useWizards"] = True
testutil.expect_prompt("The authenticity of host '{}' can't be established.".format(SSH_HOST), "n")
EXPECT_THROWS(lambda: shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-CONFIG-file": config_file}),
  "Tunnel connection cancelled")
testutil.assert_no_prompts()

#@<> Tunnel Creation Errors - Non registered server interactive - accepting the new server
# WL#14246 - TSFR_11_2 connect to the sshd and create a tunnel without known host
testutil.expect_password(f"Please provide the password for ssh://{SSH_URI_NOPASS}: ", SSH_PASS)
testutil.expect_prompt("The authenticity of host '{}' can't be established.".format(SSH_HOST), "y")
sess = shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-CONFIG-file": config_file})
EXPECT_TRUE(sess.is_open(), "Unable to open connection using shell.connect through SSH tunnel")
sess.close()
testutil.assert_no_prompts()

####################

shell.options["credentialStore.helper"] = "plaintext"


####################
create_ssh_conf(config_file, f"""
Host *
    UserKnownHostsFile {known_hosts_file}
    PubkeyAuthentication yes
    PasswordAuthentication no
    KbdInteractiveAuthentication no
""")

#@<> Tunnel Creation Errors - No password for SSH Key
WIPE_SHELL_LOG()
shell.options["useWizards"] = False
EXPECT_THROWS(lambda: shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-identity-file": key_with_pw, "ssh-config-file": config_file}),
  "Authentication error opening SSH tunnel: Access denied")
testutil.assert_no_prompts()

#@<> Tunnel Creation Errors - Invalid stored password for SSH Key - Non Interactive
WIPE_SHELL_LOG()
shell.options["useWizards"] = False
shell.store_credential(f"ssh://{SSH_URI_NOPASS}", SSH_PASS)
shell.store_credential(key_with_pw_uri, "invalid")
EXPECT_THROWS(lambda: shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-identity-file": key_with_pw, "ssh-config-file": config_file}),
  "Authentication error opening SSH tunnel: Access denied")
EXPECT_SHELL_LOG_CONTAINS("Invalid password has been erased.")
testutil.assert_no_prompts()
shell.delete_credential(f"ssh://{SSH_URI_NOPASS}")

#@<> Tunnel Creation Errors - Invalid stored password for SSH Key - Interactive - good password in prompt
WIPE_SHELL_LOG()
shell.options["useWizards"] = True
shell.store_credential(f"ssh://{SSH_URI_NOPASS}", SSH_PASS)
shell.store_credential(key_with_pw_uri, "invalid")
testutil.expect_password("Please provide key passphrase for {}: ".format(key_with_pw), "password")
testutil.expect_prompt(f"Save password for '{key_with_pw_uri}'? [Y]es/[N]o/Ne[v]er (default No): ", "Y")
sess = shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-identity-file": key_with_pw, "ssh-config-file": config_file})
EXPECT_SHELL_LOG_CONTAINS("Invalid password has been erased.")
EXPECT_TRUE(sess.is_open(), "Unable to open connection using shell.connect through SSH tunnel")
sess.close()
testutil.assert_no_prompts()
shell.delete_credential(f"ssh://{SSH_URI_NOPASS}")
shell.delete_credential(key_with_pw_uri)


#@<> WL#14246 TSFR_4_2 Run command lines that uses SSH to create a connection setting --ssh to:
create_ssh_conf(config_file, f"""
Host *
    UserKnownHostsFile {known_hosts_file}
    PubkeyAuthentication no
""")

#@<> Tunnel Creation Errors - Invalid SSH host
testutil.call_mysqlsh(["--ssh", "ssh_user@invalid_host", MYSQL_OVER_SSH_URI])
EXPECT_STDOUT_CONTAINS("Cannot open SSH Tunnel: Failed to resolve hostname invalid_host")
WIPE_STDOUT()

#@<> Tunnel Creation Errors - Invalid User
test_ssh_uri_nopwd="unexisting_user@{host}:{port}".format(**SSH_URI_DATA)
shell.store_credential("ssh://" + test_ssh_uri_nopwd, "password")
testutil.call_mysqlsh(["--credential-store-helper=plaintext", "--ssh", test_ssh_uri_nopwd, MYSQL_OVER_SSH_URI, "--ssh-config-file", config_file], "Cancel", ["MYSQLSH_TERM_COLOR_MODE=nocolor"], os.path.join(__bin_dir, "mysqlsh"))
EXPECT_STDOUT_CONTAINS(f"ERROR: SSH Access denied, connection to \"{test_ssh_uri_nopwd}\" could not be established.")
EXPECT_STDOUT_CONTAINS(f"ERROR: Authentication error opening SSH tunnel: Access denied")
EXPECT_STDOUT_CONTAINS("Tunnel connection cancelled")
WIPE_STDOUT()


#@<> Tunnel Creation Errors - Invalid stored password - Interactive - bad password in prompt
WIPE_SHELL_LOG()
shell.store_credential(f"ssh://{SSH_URI_NOPASS}", "invalid")
testutil.expect_prompt("Access denied [R]etry/[C]ancel (default Cancel)", "r")
testutil.expect_password(f"Please provide the password for ssh://{SSH_URI_NOPASS}: ", "Whatever")
testutil.expect_prompt("Access denied [R]etry/[C]ancel (default Cancel)", "c")
EXPECT_THROWS(lambda: shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-config-file": config_file}), "Tunnel connection cancelled")
EXPECT_STDOUT_CONTAINS(f"ERROR: SSH Access denied, connection to \"{SSH_URI_NOPASS}\" could not be established.")
EXPECT_STDOUT_CONTAINS(f"ERROR: Authentication error opening SSH tunnel: Access denied")
WIPE_STDOUT()

#@<> Tunnel Creation Errors - Invalid stored password - Interactive - good password in prompt
WIPE_SHELL_LOG()
shell.store_credential(f"ssh://{SSH_URI_NOPASS}", "invalid")
testutil.expect_prompt("Access denied [R]etry/[C]ancel (default Cancel)", "r")
testutil.expect_password(f"Please provide the password for ssh://{SSH_URI_NOPASS}: ", SSH_PASS)
testutil.expect_prompt(f"Save password for 'ssh://{SSH_URI_NOPASS}'? [Y]es/[N]o/Ne[v]er (default No): ", "Y")
sess = shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-config-file": config_file})
EXPECT_SHELL_LOG_CONTAINS("Invalid password has been erased.")
EXPECT_TRUE(sess.is_open(), "Unable to open connection using shell.connect through SSH tunnel")
sess.close()
testutil.assert_no_prompts()



#@<> WL#14246 TSFR_7_4 create connection to invalid uri
shell.options["useWizards"] = False
conn = {'uri':None, "ssh": SSH_URI_NOPASS, "ssh-config-file": config_file}
conn["uri"] = "mysql://user@invalid_host:3306"
EXPECT_THROWS(lambda: shell.connect(conn), "Error opening MySQL session through SSH tunnel: Lost connection to MySQL server at 'reading initial communication packet'")
conn["uri"] = ""
EXPECT_THROWS(lambda: shell.connect(conn), "Invalid URI: Expected token at position 0 but no tokens left")
conn["uri"] = "mysql://user@host name:3306"
EXPECT_THROWS(lambda: shell.connect(conn), "Invalid URI: Illegal space found at position 17")


conn = {"uri":"mysqlx://user@invalid_host:3306"}
EXPECT_THROWS(lambda: shell.connect(conn), "MySQL Error (2005): Shell.connect: No such host is known 'invalid_host'")
conn = {"uri":""}
EXPECT_THROWS(lambda: shell.connect(conn), "Invalid URI: Expected token at position 0 but no tokens left")
conn = {"uri":"mysqlx://user@host name:33060"}
EXPECT_THROWS(lambda: shell.connect(conn), "Invalid URI: Illegal space found at position 18")

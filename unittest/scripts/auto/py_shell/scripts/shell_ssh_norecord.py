#@ {has_ssh_environment()}

from _ssh_utils import *
from pathlib import Path
import shutil

#@<> setup
shell.options.useWizards = True

# Utility Files
known_hosts_file = reset_tmp_file("known_hosts")
config_file = reset_tmp_file("ssh_config")
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

shell.options["credentialStore.helper"] = "plaintext"
shell.delete_all_credentials()

#@<> Check config file permission
default_config = f"""
Host *
    UserKnownHostsFile {known_hosts_file}
"""
create_ssh_conf(config_file, default_config, False)
EXPECT_THROWS(lambda: shell.connect({"uri": MYSQL_OVER_SSH_URI,
               "ssh": SSH_URI_NOPASS, "ssh-password" : SSH_PASS,  "ssh-config-file": config_file}),
               "Invalid SSH configuration file: '{}' has insecure permissions. Expected u+rw only: Permission denied".format(config_file))

# Updates the file with the right permissions
create_ssh_conf(config_file, default_config)

#this test must be executed as a first one to create valid known_hosts file so all futher connections won't need to confirm the fingerprint
# we will also test cAsEsEnSiTiViTy on 'ssh' and 'config-file'
#@<> WL#14246 - TSFR_11_2 connect to the sshd and create a tunnel without known host
testutil.expect_prompt("The authenticity of host '{}' can't be established.".format(SSH_HOST), "y")
testutil.expect_prompt("Save password for 'ssh://{}'? ".format(SSH_URI_NOPASS), "n")
sess = shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-password" : SSH_PASS, "ssh-CONFIG-file": config_file})
EXPECT_TRUE(sess.is_open(), "Unable to open connection using shell.connect through SSH tunnel")

# Test the new ssh-uri property on the Classic Session
EXPECT_EQ(f"ssh://{SSH_URI_NOPASS}", sess.ssh_uri)
sess.close()
testutil.assert_no_prompts()

def check_connection(obj, connection_data):
    s = None
    s_name = None
    if "connect" in dir(obj):
        s = obj.connect(connection_data)
        s_name = "shell.connect"
    else:
        s = obj.get_session(connection_data)
        s_name = "{}.get_session".format(obj.__name__)
    EXPECT_TRUE(s.is_open(), "Unable to open connection using {} through SSH tunnel".format(s_name))
    s.close()
    WIPE_OUTPUT()


#@<> credential helpers handling
create_ssh_conf(config_file, f"""
Host *
    UserKnownHostsFile {known_hosts_file}
    PubkeyAuthentication no
""")

for helper in shell.list_credential_helpers():
  shell.options["credentialStore.helper"] = helper
  shell.store_credential("ssh://{}".format(SSH_URI_NOPASS), SSH_PASS)
  shell.store_credential(MYSQL_OVER_SSH_URI_NOPASS, MYSQL_OVER_SSH_PASS)
  sess = shell.connect({"uri": MYSQL_OVER_SSH_URI_NOPASS,
                  "ssh": SSH_URI_NOPASS,
                  "ssh-config-file": config_file})
  EXPECT_TRUE(sess.is_open(), "Unable to open connection using shell.connect through SSH tunnel using password")
  sess.close()
  # TODO(rennox): This should be configurable
  shell.store_credential(key_with_pw_uri, "password")
  sess = shell.connect({"uri": MYSQL_OVER_SSH_URI,
               "ssh": SSH_URI_NOPASS,
               "ssh-identity-file": key_with_pw, "ssh-config-file": config_file})
  EXPECT_TRUE(sess.is_open(), "Unable to open connection using shell.connect through SSH tunnel using protected keyfile")
  sess.close()
  shell.delete_credential(key_with_pw_uri)
  shell.delete_credential(MYSQL_OVER_SSH_URI_NOPASS)
  shell.delete_credential("ssh://{}".format(SSH_URI_NOPASS))

#@<> WL#14246 - TSFR_2_1 create a SSH connection
conn = {"uri": MYSQL_OVER_SSH_URI,
        "ssh": SSH_URI_NOPASS, "ssh-password": SSH_PASS,
        "ssh-config-file": config_file}


for o in [(shell, MYSQL_OVER_SSH_URI), (mysqlx, f"{MYSQL_OVER_SSH_URI}0"), (mysql,MYSQL_OVER_SSH_URI)]:
    testutil.expect_prompt("Save password for 'ssh://{}'? ".format(SSH_URI_NOPASS), "N")
    conn["uri"] = o[1]
    check_connection(o[0], conn)
    testutil.assert_no_prompts()

conn = {"user": MYSQL_OVER_SSH_URI_DATA.user,
        "host": MYSQL_OVER_SSH_URI_DATA.host,
        "port": MYSQL_OVER_SSH_URI_DATA,
        "password":MYSQL_OVER_SSH_URI_DATA.password,
        "ssh": SSH_URI_NOPASS, "ssh-password": SSH_PASS,
        "ssh-config-file": config_file}
for o in [(shell, 3306), (mysqlx, 33060), (mysql, 3306)]:
    testutil.expect_prompt(f"Save password for 'ssh://{SSH_URI_NOPASS}'? ", "N")
    conn["port"] = o[1]
    check_connection(o[0], conn)
    testutil.assert_no_prompts()

#@<> test credential storage
create_ssh_conf(config_file, f"""
Host *
    UserKnownHostsFile {known_hosts_file}
    PubkeyAuthentication no
""")

shell.options["credentialStore.helper"] = "plaintext"
shell.options["credentialStore.savePasswords"] = "prompt"
EXPECT_EQ(0, len(shell.list_credentials()))
testutil.expect_prompt(f"Save password for 'ssh://{SSH_URI_NOPASS}'?", "y")
testutil.expect_password(f"Please provide the password for ssh://{SSH_URI_NOPASS}: ", SSH_PASS)
sess = shell.connect({"uri": MYSQL_OVER_SSH_URI,
               "ssh": SSH_URI_NOPASS,
               "ssh-config-file": config_file})
EXPECT_TRUE(sess.is_open(), "Unable to open connection using shell.connect through SSH tunnel")
sess.close()
testutil.assert_no_prompts()
EXPECT_EQ(1, len(shell.list_credentials()))
shell.delete_credential(f"ssh://{SSH_URI_NOPASS}")

shell.options["credentialStore.excludeFilters"] = [f"ssh://{SSH_URI_NOPASS}"]
testutil.expect_password(f"Please provide the password for ssh://{SSH_URI_NOPASS}: ", SSH_PASS)
sess = shell.connect({"uri": MYSQL_OVER_SSH_URI,
               "ssh": SSH_URI_NOPASS,
               "ssh-config-file": config_file})
EXPECT_TRUE(sess.is_open(), "Unable to open connection using shell.connect through SSH tunnel")
sess.close()
testutil.assert_no_prompts()
EXPECT_EQ(0, len(shell.list_credentials()))
shell.options["credentialStore.excludeFilters"] = []

shell.options["credentialStore.savePasswords"] = "never"
testutil.expect_password(f"Please provide the password for ssh://{SSH_URI_NOPASS}: ", SSH_PASS)
sess = shell.connect({"uri": MYSQL_OVER_SSH_URI,
               "ssh": SSH_URI_NOPASS,
               "ssh-config-file": config_file})
EXPECT_TRUE(sess.is_open(), "Unable to open connection using shell.connect through SSH tunnel")
sess.close()
testutil.assert_no_prompts()
EXPECT_EQ(0, len(shell.list_credentials()))

shell.options["credentialStore.savePasswords"] = "prompt"

#@<> WL#14246 Check socket connections through SSH
WIPE_OUTPUT()
\connect --ssh example.com root@/tmp%2F/mysql.sock
EXPECT_STDERR_CONTAINS("Socket connections through SSH are not supported")
WIPE_OUTPUT()
testutil.call_mysqlsh(["--ssh", "example.com", "root@/tmp%2F/mysql.sock"])
EXPECT_STDOUT_CONTAINS("Socket connections through SSH are not supported")
WIPE_OUTPUT()


#@<> try interactive authentication
create_ssh_conf(config_file, f"""
Host *
    UserKnownHostsFile {known_hosts_file}
    PubkeyAuthentication no
    PasswordAuthentication no
    GSSAPIAuthentication no
    KbdInteractiveAuthentication    yes
""")

# todo(kg): troubleshoot ssh data leakage between unit tests
# "WL#14246 TSFR_7_4 create connection to invalid uri" test step from
# shell_ssh_errors_norecord.py is leaking ssh connection entires between unit tests.
#
# moreover connections are not closed properly, and we have many of sshd's
# in defunct state on ssh jump server
#
# # ps fuxa
# testuser  165497  0.0  0.0      0     0 ?        Z    15:16   0:00 [sshd] <defunct>
# sshd      165505  0.0  0.0      0     0 ?        Z    15:17   0:00 [sshd] <defunct>
# sshd      165507  0.0  0.0      0     0 ?        Z    15:17   0:00 [sshd] <defunct>

EXPECT_EQ(0, len(shell.list_ssh_connections()))
testutil.expect_password("Password:", SSH_PASS)
sess = shell.connect({'uri':MYSQL_OVER_SSH_URI, 'ssh':SSH_URI_NOPASS, "ssh-config-file": config_file})
EXPECT_EQ(1, len(shell.list_ssh_connections()))
sess.close()
testutil.assert_no_prompts()

#@<> WL#14246 - TSFR_2_5 create a SSH connection without MySQL port
options = [{'user':'root', 'password':'sandbox','host':'127.0.0.1','ssh':'root@127.0.0.1:2222', 'ssh-password':'root'},
        {'uri':'root:sandbox@127.0.0.1', 'ssh':'root@127.0.0.1:2222', 'ssh-password':'root'}]

functions = [shell.connect, shell.open_session]

for f in functions:
    for o in options:
        EXPECT_THROWS(lambda: f(o), "Host and port for database are required in advance when using SSH tunneling functionality")


#@<> WL#14246-TSFR_6_2 create a SSH connection using a SSH key with no password
create_ssh_conf(config_file, f"""
Host *
    UserKnownHostsFile {known_hosts_file}
    PubkeyAuthentication yes
""")

conn = {"uri": None,
                "ssh": SSH_URI_NOPASS,
                "ssh-identity-file": key_without_pw,
                "ssh-config-file": config_file}
for o in [(shell, MYSQL_OVER_SSH_URI), (mysqlx, f"{MYSQL_OVER_SSH_URI}0"), (mysql,MYSQL_OVER_SSH_URI)]:
    conn["uri"] = o[1]
    check_connection(o[0], conn)

conn = {"user": "root", "host": "127.0.0.1", "port": 3306, "password":"sandbox",
                "ssh": SSH_URI_NOPASS,
                "ssh-identity-file": key_without_pw,
                "ssh-config-file": config_file}
for o in [(shell, 3306), (mysqlx, 33060), (mysql, 3306)]:
    conn["port"] = o[1]
    check_connection(o[0], conn)

testutil.call_mysqlsh(["--ssh-identity-file",  key_without_pw, "--ssh", SSH_URI_NOPASS, MYSQL_OVER_SSH_URI, "--ssh-config-file", config_file, "--sql", "-e", "SELECT 'this is an output' FROM DUAL"], "", None, os.path.join(__bin_dir, "mysqlsh"))
EXPECT_STDOUT_CONTAINS("this is an output")
WIPE_STDOUT()
testutil.call_mysqlsh(["--ssh-identity-file",  key_without_pw, "--ssh", SSH_URI_NOPASS, "--ssh-config-file", config_file,
                       "--port", "3306", "--user", "root", "--password=sandbox", "--host", "127.0.0.1", "--sql", "-e", "SELECT 'this is an output' FROM DUAL"], "", None, os.path.join(__bin_dir, "mysqlsh"))
EXPECT_STDOUT_CONTAINS("this is an output")
WIPE_STDOUT()


#@<> WL#14246-TSFR_6_3 create a SSH connection using a SSH key that requires a password:
shell.options["useWizards"] = False
conn = {"uri": MYSQL_OVER_SSH_URI,
               "ssh": SSH_URI_NOPASS,
               "ssh-identity-file": key_with_pw,
               "ssh-identity-file-password": "password", "ssh-config-file": config_file}

for o in [(shell, MYSQL_OVER_SSH_URI), (mysqlx, f"{MYSQL_OVER_SSH_URI}0"), (mysql,MYSQL_OVER_SSH_URI)]:
    conn["uri"] = o[1]
    check_connection(o[0], conn)

conn = {"user":"root", "host": "127.0.0.1", "port": 3306, "password":"sandbox",
               "ssh": SSH_URI_NOPASS,
               "ssh-identity-file": key_with_pw,
               "ssh-identity-file-password": "password", "ssh-config-file": config_file}

for o in [(shell, 3306), (mysqlx, 33060), (mysql, 3306)]:
    conn["port"] = o[1]
    check_connection(o[0], conn)

shell.store_credential(key_with_pw_uri, "password")
testutil.call_mysqlsh(["--credential-store-helper=plaintext", "--ssh-identity-file",  key_with_pw, "--ssh", SSH_URI_NOPASS, "--ssh-config-file", config_file, MYSQL_OVER_SSH_URI, "--sql", "-e", "\"SELECT 'this is an output' FROM DUAL\""], "n", None, os.path.join(__bin_dir, "mysqlsh"))
EXPECT_STDOUT_CONTAINS("this is an output")
WIPE_STDOUT()

testutil.call_mysqlsh(["--credential-store-helper=plaintext", "--ssh-identity-file",  key_with_pw, "--ssh", SSH_URI_NOPASS, "--ssh-config-file", config_file,
                       "--port", "3306", "--user", "root", "--password=sandbox", "--host", "127.0.0.1", "--sql", "-e", "\"SELECT 'this is an output' FROM DUAL\""], "n", None, os.path.join(__bin_dir, "mysqlsh"))
EXPECT_STDOUT_CONTAINS("this is an output")
shell.delete_credential(key_with_pw_uri)

#@<> WL#14246 connect to the sshd and create a tunnel using key with a bad password
shell.options["useWizards"] = True
create_ssh_conf(config_file, f"""
Host *
    UserKnownHostsFile {known_hosts_file}
    PubkeyAuthentication yes
    PasswordAuthentication no
    KbdInteractiveAuthentication no
""")

testutil.expect_prompt("Access denied [R]etry/[C]ancel (default Cancel)", "c")
EXPECT_THROWS(lambda: shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS,
                    "ssh-identity-file": key_with_pw,
                    "ssh-identity-file-password": "bad_password",
                    "ssh-config-file": config_file}),
    "Shell.connect: Tunnel connection cancelled")
EXPECT_STDOUT_CONTAINS(f'ERROR: Authentication error opening SSH tunnel: Access denied')
testutil.assert_no_prompts()

#@<> WL#14246 connect to mysql through tunnel and use a connection option
shell.options["useWizards"] = True
create_ssh_conf(config_file, f"""
Host *
    UserKnownHostsFile {known_hosts_file}
""")

testutil.expect_prompt("Save password for 'ssh://{}'? ".format(SSH_URI_NOPASS), "N")
shell.connect({"uri": f"{MYSQL_OVER_SSH_URI}?compression=True",
               "ssh": SSH_URI_NOPASS, "ssh-password" : SSH_PASS,
               "ssh-config-file": config_file})
EXPECT_STDOUT_CONTAINS("Your MySQL connection id is")
EXPECT_EQ("ON", session.run_sql("SHOW session status LIKE 'compression'").fetch_one()[1])
session.close()
testutil.assert_no_prompts()

#@<> WL#14246-TSFR_6_8 Create an SSH connection using the ssh-identity-file option
shell.options["useWizards"] = False
key_path = os.path.join(__test_data_path, "docker", "keys","missing_key")
conn = {"uri": MYSQL_OVER_SSH_URI,
                "ssh": SSH_URI_NOPASS,
                "ssh-identity-file": os.path.join(__test_data_path, "docker", "keys","missing_key"),
                "ssh-config-file": config_file}
EXPECT_THROWS(lambda: shell.connect(conn), "The path '{}' doesn't exist".format(key_path))

invalid_perms_file = os.path.join(__tmp_dir, "invalid_perms.file")
invalid_perms_dir = os.path.join(__tmp_dir, "invalid_perms")
if os.path.exists(invalid_perms_file):
    os.remove(invalid_perms_file)
if os.path.exists(invalid_perms_dir):
    shutil.rmtree(invalid_perms_dir)

Path(invalid_perms_file).touch()
os.chmod(invalid_perms_file, 0o444)
os.mkdir(invalid_perms_dir)
os.chmod(invalid_perms_dir, 0o777)
Path(os.path.join(invalid_perms_dir, "empty_file")).touch()
conn["ssh-identity-file"] = invalid_perms_file
EXPECT_THROWS(lambda: shell.connect(conn), "Invalid SSH Identity file: '{}' has insecure permissions. Expected u+rw only: Permission denied".format(invalid_perms_file))
os.chmod(invalid_perms_file, 0o000)
EXPECT_THROWS(lambda: shell.connect(conn), "Unable to open file '{}': Permission denied".format(invalid_perms_file))

conn["ssh-identity-file"] = os.path.join(invalid_perms_dir, "empty_file")
EXPECT_THROWS(lambda: shell.connect(conn), "Invalid SSH Identity file: '{}' has insecure permissions. Expected u+rw only: Permission denied".format(os.path.join(invalid_perms_dir, "empty_file")))
shutil.rmtree(invalid_perms_dir)
os.remove(invalid_perms_file)

#@<> WL#14246-TSFR_6_9 Create an SSH connection using the ssh-identity-file option.
shell.options["useWizards"] = False
os.symlink(key_with_pw, symlinked_key)
os.chmod(symlinked_key, 0o600)
conn = {"uri": MYSQL_OVER_SSH_URI,
               "ssh": SSH_URI_NOPASS,
               "ssh-identity-file": symlinked_key,
               "ssh-identity-file-password": "password", "ssh-config-file": config_file}
sess = shell.connect(conn)
EXPECT_TRUE(sess.is_open(), "Unable to open connection using shell.connect through SSH tunnel")
session.close()

os.remove(symlinked_key)
conn['ssh-identity-file'] = key_with_pw
sess = shell.connect(conn)
EXPECT_TRUE(sess.is_open(), "Unable to open connection using shell.connect through SSH tunnel")
session.close()


#@<> WL#14246 TSFR_7_3 create connection to invalid ssh
conns = [{'uri':MYSQL_OVER_SSH_URI, 'ssh':'ssh_user@invalid_host', 'ssh-password':'ssh_pwd'},
         {'uri':MYSQL_OVER_SSH_URI, 'ssh':'invalid_user@invalid_host', 'ssh-password':'ssh_pwd'}]
for c in conns:
    EXPECT_THROWS(lambda: shell.connect(c), "Cannot open SSH Tunnel: Failed to resolve hostname invalid_host")

EXPECT_THROWS(lambda: shell.connect({'uri':MYSQL_OVER_SSH_URI, 'ssh':'', 'ssh-password':'ssh_pwd'}), "Invalid URI: Expected token at position 0 but no tokens left.")
EXPECT_THROWS(lambda: shell.connect({'uri':MYSQL_OVER_SSH_URI, 'ssh':'ssh_user@host name', 'ssh-password':'ssh_pwd'}), "Invalid URI: Illegal space found at position 13")

#@<> WL#14246 - TSFR_8_1 create two connections that share the SSH tunnel
shell.options["useWizards"] = False
conn = {"uri": MYSQL_OVER_SSH_URI,
        "ssh": SSH_URI_NOPASS, "ssh-password": SSH_PASS,
        "ssh-config-file": config_file}
sess1 = mysql.get_session(conn)
ssh_connections = shell.list_ssh_connections()
EXPECT_EQ(1, len(ssh_connections))
sess2 = mysql.get_session(conn)
ssh_connections = shell.list_ssh_connections()
EXPECT_EQ(1, len(ssh_connections))
EXPECT_EQ(f"{MYSQL_OVER_SSH_HOST}:{MYSQL_OVER_SSH_URI_DATA.port}", ssh_connections[0].remote)
EXPECT_EQ(SSH_URI_NOPASS, ssh_connections[0].uri)
EXPECT_TRUE("timeCreated" in ssh_connections[0])

#@<> WL#14246 - TSFR_8_2 create two connections that share the SSH tunnel
shell.options["useWizards"] = False
conn = {"uri": MYSQL_OVER_SSH_URI,
        "ssh": SSH_URI_NOPASS, "ssh-password": SSH_PASS,
        "ssh-config-file": config_file}
sess1 = mysql.get_session(conn)
EXPECT_EQ(1, len(shell.list_ssh_connections()))
conn['ssh-password'] = SSH_USER_PASS
conn['ssh'] =  SSH_USER_URI_NOPASS
sess2 = mysql.get_session(conn)
ssh_connections = shell.list_ssh_connections()
EXPECT_EQ(2, len(ssh_connections))
EXPECT_EQ(f"{MYSQL_OVER_SSH_HOST}:{MYSQL_OVER_SSH_URI_DATA.port}", ssh_connections[0].remote)
EXPECT_EQ(SSH_URI_NOPASS, ssh_connections[0].uri)
EXPECT_EQ(f"{MYSQL_OVER_SSH_HOST}:{MYSQL_OVER_SSH_URI_DATA.port}", ssh_connections[1].remote)
EXPECT_EQ(SSH_USER_URI_NOPASS, ssh_connections[1].uri)
EXPECT_TRUE("timeCreated" in ssh_connections[0])
EXPECT_TRUE("timeCreated" in ssh_connections[1])
sess1.close()
sess2.close()
sess1 = None
sess2 = None


#@<>  WL#14246-TSFR_9_1 Create an SSH connection don't specifying a password for the SSH
shell.options["useWizards"] = True
create_ssh_conf(config_file, f"""
Host my-sample-host
    UserKnownHostsFile {known_hosts_file}
    PubkeyAuthentication yes
    HostName {SSH_URI_DATA.host}
    Port {SSH_URI_DATA.port}
    User {SSH_URI_DATA.user}

Host {SSH_URI_DATA.host}
    UserKnownHostsFile {known_hosts_file}
    PubkeyAuthentication yes
    Port {SSH_URI_DATA.port}
    User {SSH_URI_DATA.user}
""")

testutil.expect_password(f"Please provide the password for ssh://{SSH_URI_DATA.user}@my-sample-host:{SSH_URI_DATA.port}", SSH_PASS)
testutil.expect_prompt(f"Save password for 'ssh://{SSH_URI_DATA.user}@my-sample-host:{SSH_URI_DATA.port}'? ".format(SSH_URI_NOPASS), "N")
sess = shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": "my-sample-host", "ssh-config-file": config_file})

EXPECT_TRUE(sess.is_open(), "Unable to open connection using shell.connect through SSH tunnel")
sess = None
\disconnect
testutil.assert_no_prompts()
WIPE_OUTPUT()

shell.options["ssh.configFile"] = config_file
testutil.expect_password(f"Please provide the password for ssh://{SSH_URI_DATA.user}@my-sample-host:{SSH_URI_DATA.port}", SSH_PASS)
testutil.expect_prompt(f"Save password for 'ssh://{SSH_URI_DATA.user}@my-sample-host:{SSH_URI_DATA.port}'? ".format(SSH_URI_NOPASS), "N")
\connect --ssh my-sample-host root:sandbox@127.0.0.1:3306
shell.status()
EXPECT_STDOUT_CONTAINS("Connection:                   localhost via TCP/IP")
\disconnect
testutil.assert_no_prompts()
WIPE_OUTPUT()


#@<> WL#14246 TSFR_9_2 Create an SSH connection using an identify file but don't specifying a password for it
shell.options["useWizards"] = True
create_ssh_conf(config_file, f"""
Host my-sample-host
    UserKnownHostsFile {known_hosts_file}
    PubkeyAuthentication yes
    HostName {SSH_URI_DATA.host}
    Port {SSH_URI_DATA.port}
    User {SSH_URI_DATA.user}

Host {SSH_URI_DATA.host}
    UserKnownHostsFile {known_hosts_file}
    PubkeyAuthentication yes
    Port {SSH_URI_DATA.port}
    User {SSH_URI_DATA.user}
""")

testutil.expect_password("Please provide key passphrase for {}: ".format(key_with_pw), "password")
testutil.expect_prompt("Save password for 'file:{}'?".format(key_with_pw), "N")
sess = shell.connect({"uri": MYSQL_OVER_SSH_URI,
               "ssh": "my-sample-host",
               "ssh-identity-file": key_with_pw, "ssh-config-file": config_file})

EXPECT_TRUE(sess.is_open(), "Unable to open connection using shell.connect through SSH tunnel")
sess = None
\disconnect
testutil.assert_no_prompts()
WIPE_STDOUT()

#@<> WL#14246 TSFR_9_2 Create an SSH connection using an identify file in the SSH config but don't specifying a password for it
create_ssh_conf(config_file, f"""
Host my-sample-host
    UserKnownHostsFile {known_hosts_file}
    PubkeyAuthentication yes
    HostName {SSH_URI_DATA.host}
    Port {SSH_URI_DATA.port}
    User {SSH_URI_DATA.user}
    IdentityFile {key_with_pw}

Host {SSH_URI_DATA.host}
    UserKnownHostsFile {known_hosts_file}
    PubkeyAuthentication yes
    Port {SSH_URI_DATA.port}
    User {SSH_URI_DATA.user}
    IdentityFile {key_with_pw}
""")

old_config_file = shell.options["ssh.configFile"]
shell.options["ssh.configFile"] = config_file
testutil.expect_password("Passphrase for private key:", "password");
\connect --ssh my-sample-host root:sandbox@127.0.0.1:3306
shell.status()
EXPECT_STDOUT_CONTAINS("Connection:                   localhost via TCP/IP")
\disconnect
testutil.assert_no_prompts()
WIPE_OUTPUT()
shell.options["ssh.configFile"] = old_config_file



#@<> WL#14246 test all paths with non-ASCII characters {False}
non_ascii_dir = os.path.join(__tmp_dir,"zażółć")
if os.path.exists(non_ascii_dir):
  shutil.rmtree(non_ascii_dir)
os.mkdir(non_ascii_dir)
os.chmod(non_ascii_dir, 0o700)
non_ascii_ssh_config = os.path.join(non_ascii_dir, os.path.basename(config_file))
shutil.copyfile(config_file, non_ascii_ssh_config)
os.chmod(non_ascii_ssh_config, 0o600)

non_ascii_known_hosts = os.path.join(non_ascii_dir, os.path.basename(known_hosts_file))
shutil.copyfile(known_hosts_file, non_ascii_known_hosts)
os.chmod(non_ascii_known_hosts, 0o600)

non_ascii_ssh_key = os.path.join(non_ascii_dir, os.path.basename(key_with_pw))
shutil.copyfile(key_with_pw, non_ascii_ssh_key)
os.chmod(non_ascii_ssh_key, 0o600)

testutil.expect_password("Please provide key passphrase for {}".format(non_ascii_ssh_key), "password")
testutil.expect_prompt("Save password for 'file:{}'? ".format(non_ascii_ssh_key), "N")
sess = shell.connect({"uri": MYSQL_OVER_SSH_URI, "ssh": SSH_URI_NOPASS, "ssh-identity-file": non_ascii_ssh_key, "ssh-config-file": non_ascii_ssh_config})

EXPECT_TRUE(sess.is_open(), "Unable to open connection using shell.connect through SSH tunnel")
sess = None
\disconnect
testutil.assert_no_prompts()

#@<> WL#14246 TSFR_11_1 Check SSH stanza handling - using values from config - negative
shell.options["useWizards"] = True
create_ssh_conf(config_file, f"""
Host *
    UserKnownHostsFile {known_hosts_file}

Host dev
    HostName Whatever
    User unexisting
    Port 8888
""")

EXPECT_EQ(0, len(shell.list_ssh_connections()))
EXPECT_THROWS(lambda: shell.connect({'uri':MYSQL_OVER_SSH_URI, 'ssh':'dev', "ssh-config-file":config_file}),
    "Cannot open SSH Tunnel: Failed to resolve hostname Whatever")
EXPECT_STDOUT_CONTAINS(f"Opening SSH tunnel to dev:8888...")

#@<> WL#14246 TSFR_11_1 Check SSH stanza handling - using default value for missing port
shell.options["useWizards"] = True
create_ssh_conf(config_file, f"""
Host *
    UserKnownHostsFile {known_hosts_file}

Host dev
    HostName Whatever
    User unexisting
""")

EXPECT_EQ(0, len(shell.list_ssh_connections()))
EXPECT_THROWS(lambda: shell.connect({'uri':MYSQL_OVER_SSH_URI, 'ssh':'dev', "ssh-config-file":config_file}),
    "Cannot open SSH Tunnel: Failed to resolve hostname Whatever")
EXPECT_STDOUT_CONTAINS(f"Opening SSH tunnel to dev:22...")

#@<> WL#14246 TSFR_11_1 Check SSH stanza handling - overriding values from config
shell.options["useWizards"] = True
create_ssh_conf(config_file, f"""
Host *
    UserKnownHostsFile {known_hosts_file}

Host {SSH_USER_URI_DATA.host}
    User unexisting
    Port 3333
""")

EXPECT_EQ(0, len(shell.list_ssh_connections()))
testutil.expect_prompt(f"Save password for 'ssh://{SSH_USER_URI_NOPASS}'? ", "N")
testutil.expect_password(f"Please provide the password for ssh://{SSH_USER_URI_NOPASS}: ", SSH_USER_PASS)
shell.connect({'uri':MYSQL_OVER_SSH_URI, 'ssh':SSH_USER_URI_NOPASS, "ssh-config-file":config_file})
EXPECT_STDOUT_CONTAINS(f"Opening SSH tunnel to {SSH_USER_HOST}:{SSH_URI_DATA.port}...")
EXPECT_TRUE(session.is_open(), "Unable to open connection using shell.connect through SSH tunnel using protected keyfile")
session.close()
testutil.assert_no_prompts()

#@<> WL#14246 TSFR_11_1 Check SSH stanza handling - taking values from config - positive
create_ssh_conf(config_file, f"""
Host *
    UserKnownHostsFile {known_hosts_file}

Host dev
    HostName {SSH_USER_URI_DATA.host}
    User {SSH_USER_URI_DATA.user}
    Port {SSH_USER_URI_DATA.port}

Host prod
    HostName {SSH_URI_DATA.host}
    User {SSH_URI_DATA.user}
    Port {SSH_URI_DATA.port}
""")

shell.options["ssh.configFile"] = config_file

EXPECT_EQ(0, len(shell.list_ssh_connections()))
testutil.expect_password(f"Please provide the password for ssh://{SSH_USER_URI_DATA.user}@dev:{SSH_USER_URI_DATA.port}: ", SSH_USER_PASS)
testutil.expect_prompt(f"Save password for 'ssh://{SSH_USER_URI_DATA.user}@dev:{SSH_USER_URI_DATA.port}'? ", "N")
my_session = shell.open_session({'ssh':'dev', 'uri':MYSQL_OVER_SSH_URI})
EXPECT_EQ(1, len(shell.list_ssh_connections()))
my_session.close()
testutil.assert_no_prompts()

EXPECT_EQ(0, len(shell.list_ssh_connections()))
testutil.expect_prompt(f"Save password for 'ssh://{SSH_URI_DATA.user}@prod:{SSH_URI_DATA.port}'? ", "N")
testutil.expect_password(f"Please provide the password for ssh://{SSH_URI_DATA.user}@prod:{SSH_URI_DATA.port}: ", SSH_PASS)
my_session = shell.open_session({'ssh':'prod', 'uri':MYSQL_OVER_SSH_URI})
EXPECT_EQ(1, len(shell.list_ssh_connections()))
my_session.close()
testutil.assert_no_prompts()

#@<> WL#14246 TSFR_10_2 check shell options
\option ssh.configFile=/path/config
\option ssh.bufferSize=10250
\option -l
EXPECT_STDOUT_CONTAINS(" ssh.bufferSize                  10250")
EXPECT_STDOUT_CONTAINS(" ssh.configFile                  /path/config")

#@<> WL#14246 TSFR_10_4 check shell options
shell.options.set('ssh.configFile', '/path/config')
shell.options.set('ssh.bufferSize', 10250)
EXPECT_EQ('/path/config', shell.options["ssh.configFile"])
EXPECT_EQ(10250, int(shell.options["ssh.bufferSize"]))

#@<> WL#14246 10_7 check invalid values
\option ssh.bufferSize=10MB
EXPECT_STDERR_CONTAINS('ssh.bufferSize: Malformed option value.')
WIPE_OUTPUT()

shell.options.set('ssh.bufferSize','10MB')
EXPECT_STDERR_CONTAINS('ssh.bufferSize: Malformed option value.')
WIPE_OUTPUT()

EXPECT_THROWS(lambda: shell.options.set("ssh.bufferSize", -1), "ValueError: Options.set: ssh.bufferSize: value out of range")

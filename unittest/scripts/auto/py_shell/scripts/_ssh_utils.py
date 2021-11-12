import os
from mysqlsh import globals

shell_ = globals.shell
testutil_ = globals.testutil

# Reads the configuration from the env vars
SSH_URI = os.environ["SSH_URI"]

# NOTE: In the environment the SSH URI is defined with password, however for
# the shell that is invalid so we extract the password here to handle appart
SSH_URI_DATA = shell_.parse_uri(SSH_URI)
SSH_HOST = SSH_URI_DATA.host
SSH_PASS = SSH_URI_DATA.password
SSH_URI_NOPASS = "{user}@{host}:{port}".format(**SSH_URI_DATA)

SSH_USER_URI = os.environ["SSH_USER_URI"]
SSH_USER_URI_DATA = shell_.parse_uri(SSH_USER_URI)
SSH_USER_HOST = SSH_USER_URI_DATA.host
SSH_USER_PASS = SSH_USER_URI_DATA.password
SSH_USER_URI_NOPASS = "{user}@{host}:{port}".format(**SSH_USER_URI_DATA)

MYSQL_OVER_SSH_URI = os.environ["MYSQL_OVER_SSH_URI"]
MYSQL_OVER_SSH_URI_DATA = shell_.parse_uri(MYSQL_OVER_SSH_URI)
MYSQL_OVER_SSH_HOST = MYSQL_OVER_SSH_URI_DATA.host
MYSQL_OVER_SSH_PASS = MYSQL_OVER_SSH_URI_DATA.password
MYSQL_OVER_SSH_URI_NOPASS = "{user}@{host}:{port}".format(**MYSQL_OVER_SSH_URI_DATA)

def reset_tmp_file(name):
  tmpdir = os.environ["TMPDIR"]
  file_name = os.path.join(tmpdir, name)
  if os.path.exists(file_name):
    os.remove(file_name)
  return file_name

def create_ssh_conf(path, config, fix_permissions=True):
  testutil_.create_file(path, config)
  if fix_permissions:
    os.chmod(path, 0o600)

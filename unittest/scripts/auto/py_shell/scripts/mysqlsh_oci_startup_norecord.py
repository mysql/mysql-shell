#@{__with_oci==1}
import os

oci_folder = '.oci';
config_name = 'config'
def_key_name = 'oci_api_key'
def_public_key_name = 'oci_api_key_public'

oci_path = os.path.join(__home, oci_folder);
config_path = os.path.join(oci_path, config_name);
def_key_path = os.path.join(oci_path, def_key_name) + '.pem';
def_public_key_path = os.path.join(oci_path, def_public_key_name) + '.pem';

#@ Creates initial configuration
testutil.expect_prompt("Please enter your USER OCID:", "ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter your TENANCY OCID:", "ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter the number of a REGION listed above or type a custom region name:", "1");
testutil.expect_prompt("Please enter the number of an option listed above:", "1");
testutil.expect_prompt("Do you want to protect the API key with a passphrase? [Y/n]:", "");
testutil.expect_password("Enter a passphrase for the API key:", "MySamplePwd");
testutil.expect_password("Enter the passphrase again for confirmation:", "MySamplePwd");
testutil.expect_prompt("Do you want to write your passphrase to the config file? [y/N]:", "y");
util.configure_oci()

#@ Now start the shell using --oci
testutil.call_mysqlsh(["--oci", "-e", "config", "-i"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

#@ Tweak the password to make it incorrect
source = open(config_path,"r+")
lines = source.readlines()
source.seek(0)
for line in lines:
  if line.startswith("pass_phrase"):
    source.write("pass_phrase = WrongPassword\n")
  else:
    source.write(line)

source.truncate()
source.close()

testutil.call_mysqlsh(["--oci", "-e", "config", "-i", "--passwords-from-stdin"], "MySamplePwd", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

#@ Tweak the password to remove it
source = open(config_path,"r+")
lines = source.readlines()
source.seek(0)
for line in lines:
  if not line.startswith("pass_phrase"):
    source.write(line)

source.truncate()
source.close()

testutil.call_mysqlsh(["--oci", "-e", "config", "-i", "--passwords-from-stdin"], "MySamplePwd", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);


#@ Creates second configuration with no password
testutil.expect_prompt("Please enter your USER OCID:", "ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter your TENANCY OCID:", "ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter the number of a REGION listed above or type a custom region name:", "2");
testutil.expect_prompt("Please enter the number of an option listed above:", "1");
testutil.expect_password("The selected API key requires a passphrase:", "MySamplePwd");
testutil.expect_prompt("Do you want to write your passphrase to the config file? [y/N]:", "");
util.configure_oci('passwordless')

#@ Now start the shell using --oci=passwordless
testutil.call_mysqlsh(["--oci=passwordless", "-e", "config", "-i", "--passwords-from-stdin"], "MySamplePwd", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

#@<> Cleanup
os.remove(config_path);
os.remove(def_key_path);
os.remove(def_public_key_path);

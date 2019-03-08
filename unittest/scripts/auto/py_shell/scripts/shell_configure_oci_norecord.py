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

#@ Test first configuration attempt
testutil.expect_prompt("Please enter your USER OCID:", "invalid-format-ocid");
testutil.expect_prompt("Please enter your USER OCID:", "ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter your TENANCY OCID:", "invalid-format-tenancy-id");
testutil.expect_prompt("Please enter your TENANCY OCID:", "ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter the number of a REGION listed above or type a custom region name:", "8");
testutil.expect_prompt("Please enter the number of a REGION listed above or type a custom region name:", "");
testutil.expect_prompt("Please enter the number of a REGION listed above or type a custom region name:", "1");
testutil.expect_prompt("Please enter the number of an option listed above:", "1");
testutil.expect_prompt("Do you want to protect the API key with a passphrase? [Y/n]:", "");
testutil.expect_password("Enter a passphrase for the API key:", "MySamplePwd");
testutil.expect_password("Enter the passphrase again for confirmation:", "UnmatchedPwd");
testutil.expect_password("Enter the passphrase again for confirmation:", "MySamplePwd");
testutil.expect_prompt("Do you want to write your passphrase to the config file? [y/N]:", "y");

util.configure_oci()

print ("OCI Path Exists: %s" % os.path.exists(oci_path));
print ("Config File Exists: %s" % os.path.exists(config_path));
print ("Key Path Exists: %s" % os.path.exists(def_key_path));
print ("Public Key Path Exists: %s" % os.path.exists(def_public_key_path));

with open(config_path, 'r') as file:
  print(file.read())

#@ Second configuration attempt, profile already exists
util.configure_oci()

#@ Second configuration attempt, custom region, same key file
testutil.expect_prompt("Please enter your USER OCID:", "ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter your TENANCY OCID:", "ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter the number of a REGION listed above or type a custom region name:", "my-custom-region");
testutil.expect_prompt("Please enter the number of an option listed above:", "1");
testutil.expect_password("The selected API key requires a passphrase:", "WrongPwd");
testutil.expect_password("Wrong passphrase, please try again:", "MySamplePwd");
testutil.expect_prompt("Do you want to write your passphrase to the config file? [y/N]:", "y");
util.configure_oci('second')

with open(config_path, 'r') as file:
  print(file.read())


#@ Third configuration attempt, creates yet another key file
key_name = 'my_sample_key'
public_key_name = 'my_sample_key_public'
key_path = os.path.join(oci_path, key_name) + '.pem';
public_key_path = os.path.join(oci_path, public_key_name) + '.pem';

testutil.expect_prompt("Please enter your USER OCID:", "ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter your TENANCY OCID:", "ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter the number of a REGION listed above or type a custom region name:", "2");
testutil.expect_prompt("Please enter the number of an option listed above:", "2");
testutil.expect_prompt("Enter the name of the new API key:", "");
testutil.expect_prompt("Enter the name of the new API key:", "invalid key name");
testutil.expect_prompt("Enter the name of the new API key:", def_key_name);
testutil.expect_prompt("Enter the name of the new API key:", key_name);
testutil.expect_prompt("Do you want to protect the API key with a passphrase? [Y/n]:", "n");

util.configure_oci('third')

print ("Key Path Exists: %s" % os.path.exists(key_path));
print ("Public Key Path Exists: %s" % os.path.exists(public_key_path));

with open(config_path, 'r') as file:
  print(file.read())


#@ Fourth configuration attempt, uses an existing KEY without password
unexisting_path = os.path.join(oci_path, 'unexisting')
testutil.expect_prompt("Please enter your USER OCID:", "ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter your TENANCY OCID:", "ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter the number of a REGION listed above or type a custom region name:", "4");
testutil.expect_prompt("Please enter the number of an option listed above:", "3");
testutil.expect_prompt("Enter the location of the existing API key:", "");
testutil.expect_prompt("Enter the location of the existing API key:", oci_path);
testutil.expect_prompt("Enter the location of the existing API key:", unexisting_path);
testutil.expect_prompt("Enter the location of the existing API key:", key_path);

util.configure_oci('fourth')

print ("Key Path Exists: %s" % os.path.exists(key_path));
print ("Public Key Path Exists: %s" % os.path.exists(public_key_path));

with open(config_path, 'r') as file:
  print(file.read())

#@ Fifth configuration attempt, uses an existing KEY with password, savng password
testutil.expect_prompt("Please enter your USER OCID:", "ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter your TENANCY OCID:", "ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter the number of a REGION listed above or type a custom region name:", "4");
testutil.expect_prompt("Please enter the number of an option listed above:", "3");
testutil.expect_prompt("Enter the location of the existing API key:", def_key_path);
testutil.expect_password("The selected API key requires a passphrase:", "MySamplePwd");
testutil.expect_prompt("Do you want to write your passphrase to the config file? [y/N]:", "y");

util.configure_oci('fifth')

print ("Key Path Exists: %s" % os.path.exists(key_path));
print ("Public Key Path Exists: %s" % os.path.exists(public_key_path));


#@ Sixth configuration attempt, uses an existing KEY with password, NOT saving password
testutil.expect_prompt("Please enter your USER OCID:", "ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter your TENANCY OCID:", "ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq");
testutil.expect_prompt("Please enter the number of a REGION listed above or type a custom region name:", "4");
testutil.expect_prompt("Please enter the number of an option listed above:", "3");
testutil.expect_prompt("Enter the location of the existing API key:", def_key_path);
testutil.expect_password("The selected API key requires a passphrase:", "MySamplePwd");
testutil.expect_prompt("Do you want to write your passphrase to the config file? [y/N]:", "y");

util.configure_oci('fifth')

print ("Key Path Exists: %s" % os.path.exists(key_path));
print ("Public Key Path Exists: %s" % os.path.exists(public_key_path));

with open(config_path, 'r') as file:
  print(file.read())

#@ Cleanup
os.remove(config_path);
os.remove(def_key_path);
os.remove(def_public_key_path);
os.remove(key_path);
os.remove(public_key_path);
os.rmdir(oci_path);

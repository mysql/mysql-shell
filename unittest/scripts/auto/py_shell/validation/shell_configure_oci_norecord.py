#@<OUT> __global__
true

#@<OUT> Test first configuration attempt
This MySQL Shell Wizard will help you to configure a new OCI profile.
Configuring OCI for the profile 'DEFAULT'. Press Ctrl+C to cancel the wizard at any time.

For details about how to get the user OCID, tenancy OCID and region required for the OCI configuration visit:
  https://docs.cloud.oracle.com/iaas/Content/API/Concepts/apisigningkey.htm#How3

Please enter your USER OCID: invalid-format-ocid
WARNING: Invalid OCID Format.
Please enter your USER OCID: ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
Please enter your TENANCY OCID: invalid-format-tenancy-id
WARNING: Invalid OCID Format.
Please enter your TENANCY OCID: ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
The region of the tenancy needs to be specified, available options include:
  1) ca-toronto-1
  2) eu-frankfurt-1
  3) uk-london-1
  4) us-ashburn-1
  5) us-phoenix-1

Please enter the number of a REGION listed above or type a custom region name: 8
WARNING: Invalid option selected.
Please enter the number of a REGION listed above or type a custom region name: 
WARNING: The region name can not be empty or blank characters.
Please enter the number of a REGION listed above or type a custom region name: 1
To access Oracle Cloud Resources an API Key is required, available options include:
  1) Create new API key
  2) Use an existing API key

Please enter the number of an option listed above: 1
Do you want to protect the API key with a passphrase? [Y/n]: 
Enter a passphrase for the API key: ***********
Enter the passphrase again for confirmation: ************
WARNING: The confirmation passphrase does not match the initial passphrase.
Enter the passphrase again for confirmation: ***********
Do you want to write your passphrase to the config file? [y/N]: y

A new OCI profile named 'DEFAULT' has been created *successfully*.

The configuration and keys have been stored in the following directory:
  <<<oci_path>>>

Please ensure your *public* API key is uploaded to your OCI User Settings page.
It can be found at <<<def_public_key_path>>>
OCI Path Exists: True
Config File Exists: True
Key Path Exists: True
Public Key Path Exists: True

[DEFAULT]
fingerprint = [[*]]
key_file = <<<def_key_path>>>
pass_phrase = MySamplePwd
region = ca-toronto-1
tenancy = ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
user = ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq

#@ Second configuration attempt, profile already exists
||

#@<OUT> Second configuration attempt, custom region, same key file
This MySQL Shell Wizard will help you to configure a new OCI profile.
Configuring OCI for the profile 'second'. Press Ctrl+C to cancel the wizard at any time.

For details about how to get the user OCID, tenancy OCID and region required for the OCI configuration visit:
  https://docs.cloud.oracle.com/iaas/Content/API/Concepts/apisigningkey.htm#How3

Please enter your USER OCID: ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
Please enter your TENANCY OCID: ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
The region of the tenancy needs to be specified, available options include:
  1) ca-toronto-1
  2) eu-frankfurt-1
  3) uk-london-1
  4) us-ashburn-1
  5) us-phoenix-1

Please enter the number of a REGION listed above or type a custom region name: my-custom-region
To access Oracle Cloud Resources an API Key is required, available options include:
  1) Use the default API key [oci_api_key]
  2) Create new API key
  3) Use an existing API key

Please enter the number of an option listed above: 1
The selected API key requires a passphrase: ********
Wrong passphrase, please try again: ***********
Do you want to write your passphrase to the config file? [y/N]: y

A new OCI profile named 'second' has been created *successfully*.

Please ensure your *public* API key is uploaded to your OCI User Settings page.

[DEFAULT]
fingerprint = [[*]]
key_file = <<<def_key_path>>>
pass_phrase = MySamplePwd
region = ca-toronto-1
tenancy = ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
user = ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq


[second]
fingerprint = [[*]]
key_file = <<<def_key_path>>>
pass_phrase = MySamplePwd
region = my-custom-region
tenancy = ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
user = ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq



#@<OUT> Third configuration attempt, creates yet another key file
This MySQL Shell Wizard will help you to configure a new OCI profile.
Configuring OCI for the profile 'third'. Press Ctrl+C to cancel the wizard at any time.

For details about how to get the user OCID, tenancy OCID and region required for the OCI configuration visit:
  https://docs.cloud.oracle.com/iaas/Content/API/Concepts/apisigningkey.htm#How3

Please enter your USER OCID: ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
Please enter your TENANCY OCID: ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
The region of the tenancy needs to be specified, available options include:
  1) ca-toronto-1
  2) eu-frankfurt-1
  3) uk-london-1
  4) us-ashburn-1
  5) us-phoenix-1

Please enter the number of a REGION listed above or type a custom region name: 2
To access Oracle Cloud Resources an API Key is required, available options include:
  1) Use the default API key [oci_api_key]
  2) Create new API key
  3) Use an existing API key

Please enter the number of an option listed above: 2
Enter the name of the new API key: 
WARNING: The API key name can not be empty or blank characters.
Enter the name of the new API key: invalid key name
WARNING: The key name must be a valid identifier: only letters, numbers and underscores are allowed.
Enter the name of the new API key: oci_api_key
WARNING: A private API key with the indicated name already exists, use a different name.
Enter the name of the new API key: my_sample_key
Do you want to protect the API key with a passphrase? [Y/n]: n

A new OCI profile named 'third' has been created *successfully*.

The keys have been stored in the following directory:
  <<<oci_path>>>

Please ensure your *public* API key is uploaded to your OCI User Settings page.
It can be found at <<<public_key_path>>>
Key Path Exists: True
Public Key Path Exists: True

[DEFAULT]
fingerprint = [[*]]
key_file = <<<def_key_path>>>
pass_phrase = MySamplePwd
region = ca-toronto-1
tenancy = ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
user = ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq


[second]
fingerprint = [[*]]
key_file = <<<def_key_path>>>
pass_phrase = MySamplePwd
region = my-custom-region
tenancy = ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
user = ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq


[third]
fingerprint = [[*]]
key_file = <<<key_path>>>
region = eu-frankfurt-1
tenancy = ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
user = ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq



#@<OUT> Fourth configuration attempt, uses an existing KEY without password
This MySQL Shell Wizard will help you to configure a new OCI profile.
Configuring OCI for the profile 'fourth'. Press Ctrl+C to cancel the wizard at any time.

For details about how to get the user OCID, tenancy OCID and region required for the OCI configuration visit:
  https://docs.cloud.oracle.com/iaas/Content/API/Concepts/apisigningkey.htm#How3

Please enter your USER OCID: ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
Please enter your TENANCY OCID: ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
The region of the tenancy needs to be specified, available options include:
  1) ca-toronto-1
  2) eu-frankfurt-1
  3) uk-london-1
  4) us-ashburn-1
  5) us-phoenix-1

Please enter the number of a REGION listed above or type a custom region name: 4
To access Oracle Cloud Resources an API Key is required, available options include:
  1) Use the default API key [oci_api_key]
  2) Create new API key
  3) Use an existing API key

Please enter the number of an option listed above: 3
Enter the location of the existing API key: 
WARNING: The path can not be empty or blank characters.
Enter the location of the existing API key: <<<oci_path>>>
WARNING: The indicated path is not a file.
Enter the location of the existing API key: <<<unexisting_path>>>
WARNING: The indicated path does not exist.
Enter the location of the existing API key: <<<key_path>>>

A new OCI profile named 'fourth' has been created *successfully*.

Please ensure your *public* API key is uploaded to your OCI User Settings page.
Key Path Exists: True
Public Key Path Exists: True

[DEFAULT]
fingerprint = [[*]]
key_file = <<<def_key_path>>>
pass_phrase = MySamplePwd
region = ca-toronto-1
tenancy = ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
user = ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq


[second]
fingerprint = [[*]]
key_file = <<<def_key_path>>>
pass_phrase = MySamplePwd
region = my-custom-region
tenancy = ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
user = ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq


[third]
fingerprint = [[*]]
key_file = <<<key_path>>>
region = eu-frankfurt-1
tenancy = ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
user = ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq


[fourth]
fingerprint = [[*]]
key_file = <<<key_path>>>
region = us-ashburn-1
tenancy = ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
user = ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq



#@<OUT> Fifth configuration attempt, uses an existing KEY with password, savng password
This MySQL Shell Wizard will help you to configure a new OCI profile.
Configuring OCI for the profile 'fifth'. Press Ctrl+C to cancel the wizard at any time.

For details about how to get the user OCID, tenancy OCID and region required for the OCI configuration visit:
  https://docs.cloud.oracle.com/iaas/Content/API/Concepts/apisigningkey.htm#How3

Please enter your USER OCID: ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
Please enter your TENANCY OCID: ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
The region of the tenancy needs to be specified, available options include:
  1) ca-toronto-1
  2) eu-frankfurt-1
  3) uk-london-1
  4) us-ashburn-1
  5) us-phoenix-1

Please enter the number of a REGION listed above or type a custom region name: 4
To access Oracle Cloud Resources an API Key is required, available options include:
  1) Use the default API key [oci_api_key]
  2) Create new API key
  3) Use an existing API key

Please enter the number of an option listed above: 3
Enter the location of the existing API key: <<<def_key_path>>>
The selected API key requires a passphrase: ***********
Do you want to write your passphrase to the config file? [y/N]: y

A new OCI profile named 'fifth' has been created *successfully*.

Please ensure your *public* API key is uploaded to your OCI User Settings page.
Key Path Exists: True
Public Key Path Exists: True

#@<OUT> Sixth configuration attempt, uses an existing KEY with password, NOT saving password
Key Path Exists: True
Public Key Path Exists: True

[DEFAULT]
fingerprint = [[*]]
key_file = <<<def_key_path>>>
pass_phrase = MySamplePwd
region = ca-toronto-1
tenancy = ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
user = ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq


[second]
fingerprint = [[*]]
key_file = <<<def_key_path>>>
pass_phrase = MySamplePwd
region = my-custom-region
tenancy = ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
user = ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq


[third]
fingerprint = [[*]]
key_file = <<<key_path>>>
region = eu-frankfurt-1
tenancy = ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
user = ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq


[fourth]
fingerprint = [[*]]
key_file = <<<key_path>>>
region = us-ashburn-1
tenancy = ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
user = ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq


[fifth]
fingerprint = [[*]]
key_file = <<<def_key_path>>>
pass_phrase = MySamplePwd
region = us-ashburn-1
tenancy = ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq
user = ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq



#@ Cleanup
||


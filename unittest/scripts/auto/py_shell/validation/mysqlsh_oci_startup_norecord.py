#@<OUT> Creates initial configuration
This MySQL Shell Wizard will help you to configure a new OCI profile.
Configuring OCI for the profile 'DEFAULT'. Press Ctrl+C to cancel the wizard at any time.

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

Please enter the number of a REGION listed above or type a custom region name: 1
To access Oracle Cloud Resources an API Key is required, available options include:
  1) Create new API key
  2) Use an existing API key

Please enter the number of an option listed above: 1
Do you want to protect the API key with a passphrase? [Y/n]: 
Enter a passphrase for the API key: ***********
Enter the passphrase again for confirmation: ***********
Do you want to write your passphrase to the config file? [y/N]: y

A new OCI profile named 'DEFAULT' has been created *successfully*.

The configuration and keys have been stored in the following directory:
  <<<oci_path>>>

Please ensure your *public* API key is uploaded to your OCI User Settings page.
It can be found at <<<def_public_key_path>>>

#@<OUT> Now start the shell using --oci
MySQL Shell [[*]]

Copyright (c) 2016, [[*]], Oracle and/or its affiliates. All rights reserved.
Oracle is a registered trademark of Oracle Corporation and/or its affiliates.
Other names may be trademarks of their respective owners.

Type '\help' or '\?' for help; '\quit' to exit.

Loading the OCI configuration for the profile 'DEFAULT'...

The config, identity and compute objects have been initialized.
Type '\? oci' for help.
{
    "additional_user_agent": "",
    "fingerprint": "[[*]]",
    "key_file": "<<<repr(def_key_path)[1:-1]>>>",
    "log_requests": false,
    "pass_phrase": "MySamplePwd",
    "region": "ca-toronto-1",
    "tenancy": "ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq",
    "user": "ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq"
}
0

#@<OUT> Tweak the password to make it incorrect
MySQL Shell [[*]]

Copyright (c) 2016, [[*]], Oracle and/or its affiliates. All rights reserved.
Oracle is a registered trademark of Oracle Corporation and/or its affiliates.
Other names may be trademarks of their respective owners.

Type '\help' or '\?' for help; '\quit' to exit.

Loading the OCI configuration for the profile 'DEFAULT'...
WARNING: The API key for 'DEFAULT' requires a passphrase to be successfully used and the one in the configuration is incorrect.
Please enter the API key passphrase: 
The config, identity and compute objects have been initialized.
Type '\? oci' for help.
{
    "additional_user_agent": "",
    "fingerprint": "[[*]]",
    "key_file": "<<<repr(def_key_path)[1:-1]>>>",
    "log_requests": false,
    "pass_phrase": "MySamplePwd",
    "region": "ca-toronto-1",
    "tenancy": "ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq",
    "user": "ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq"
}
0

#@<OUT> Tweak the password to remove it
MySQL Shell [[*]]

Copyright (c) 2016, [[*]], Oracle and/or its affiliates. All rights reserved.
Oracle is a registered trademark of Oracle Corporation and/or its affiliates.
Other names may be trademarks of their respective owners.

Type '\help' or '\?' for help; '\quit' to exit.

Loading the OCI configuration for the profile 'DEFAULT'...
Please enter the API key passphrase: 
The config, identity and compute objects have been initialized.
Type '\? oci' for help.
{
    "additional_user_agent": "",
    "fingerprint": "[[*]]",
    "key_file": "<<<repr(def_key_path)[1:-1]>>>",
    "log_requests": false,
    "pass_phrase": "MySamplePwd",
    "region": "ca-toronto-1",
    "tenancy": "ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq",
    "user": "ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq"
}
0

#@<OUT> Creates second configuration with no password
This MySQL Shell Wizard will help you to configure a new OCI profile.
Configuring OCI for the profile 'passwordless'. Press Ctrl+C to cancel the wizard at any time.

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

Please enter the number of an option listed above: 1
The selected API key requires a passphrase: ***********
Do you want to write your passphrase to the config file? [y/N]: 

A new OCI profile named 'passwordless' has been created *successfully*.

Please ensure your *public* API key is uploaded to your OCI User Settings page.

#@<OUT> Now start the shell using --oci=passwordless
MySQL Shell [[*]]

Copyright (c) 2016, [[*]], Oracle and/or its affiliates. All rights reserved.
Oracle is a registered trademark of Oracle Corporation and/or its affiliates.
Other names may be trademarks of their respective owners.

Type '\help' or '\?' for help; '\quit' to exit.

Loading the OCI configuration for the profile 'passwordless'...
Please enter the API key passphrase: 
The config, identity and compute objects have been initialized.
Type '\? oci' for help.
{
    "additional_user_agent": "",
    "fingerprint": "[[*]]",
    "key_file": "<<<repr(def_key_path)[1:-1]>>>",
    "log_requests": false,
    "pass_phrase": "MySamplePwd",
    "region": "eu-frankfurt-1",
    "tenancy": "ocid1.tenancy.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq",
    "user": "ocid1.user.oc1..abcdfgetrhjzqdlgnqlrmzclepeihqrjtqmbwz6s562ywdyikr5gr7izfhlq"
}
0

#@<OUT> WL13046-FR-TS-02 Importing paramiko
2.4.2

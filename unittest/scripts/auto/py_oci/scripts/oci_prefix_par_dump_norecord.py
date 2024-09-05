#@ {has_oci_environment('OS') and __version_num >= 80000}

#@<> INCLUDE oci_utils.inc

#@<> INCLUDE dump_utils.inc

#@<> setup
import os
from random import shuffle

def compare_instances(check_events=True, check_triggers=True, check_stored_procedures=True):
    session1 = mysql.get_session(__sandbox_uri1)
    session2 = mysql.get_session(__sandbox_uri2)
    compare_servers(session1, session2, check_rows=True, check_users=True, check_events=check_events, check_triggers=check_triggers, check_stored_procedures=check_stored_procedures)
    session1.close()
    session2.close()

def anonymize_par(par, full=True):
    pos = 0
    secret_index = 4
    if not full:
        pos = par.find("/p/")
        secret_index = 2
    tokens = par[pos:].split("/")
    tokens[secret_index] = "<secret>"
    return "/".join(tokens)


RFC3339 = True
azure_sas_token="sv=2021-06-08&ss=bfqt&srt=sco&sp=rwdlacupiytfx&se=2022-11-23T05:31:36Z&st=2022-11-22T21:31:36Z&spr=https&sig=0bEAWf%2FtR%2BHhHDkHdP5khrLh3YrcN0X%2BIvirOVgXqbA%3D"

# === SETUP FOR AWS-PAR Option Conflict Test ===
aws_config_dir = os.path.join(__tmp_dir, "test_aws_config")
aws_config_file_data= """
[default]
region=us-west-2
"""

aws_credentials_file_data="""
[default]
aws_access_key_id=AKIAIOSFODNN7EXAMPLE
aws_secret_access_key=wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY
"""

testutil.mkdir(aws_config_dir)
aws_config_file = os.path.join(aws_config_dir, "config")
aws_credentials_file = os.path.join(aws_config_dir, "credentials")

testutil.create_file(aws_config_file, aws_config_file_data)
testutil.create_file(aws_credentials_file, aws_credentials_file_data)

# ==============================================

# 1 Creates a couple of sandbox instances
testutil.deploy_sandbox(__mysql_sandbox_port1, "root", {"local_infile":1})
testutil.deploy_sandbox(__mysql_sandbox_port2, "root", {"local_infile":1})

# Loads the data to be used in tests
shell.connect(__sandbox_uri1)
session.run_sql("set names utf8mb4")
session.run_sql("create schema sakila")
testutil.import_data(__sandbox_uri1, os.path.join(__data_path, "sql", "sakila-schema.sql"), "sakila")
testutil.import_data(__sandbox_uri1, os.path.join(__data_path, "sql", "sakila-data.sql"), "sakila")

sakila_tables = []
for record in session.run_sql("show tables from sakila").fetch_all():
    sakila_tables.append(record[0])

conflicting_options = {
    "osBucketName":  {"osBucketName": OS_BUCKET_NAME},
    "azureContainerName":  {"azureContainerName": "somecontainer", "azureStorageAccount": "someaccount","azureStorageSasToken": azure_sas_token},
    "s3BucketName": {"s3BucketName":"somebucket", "s3ConfigFile":aws_config_file, "s3CredentialsFile":aws_credentials_file},
}

dump_util_cb = {
    "instance": lambda par, options: util.dump_instance(par, options),
    "schemas": lambda par, options: util.dump_schemas(["sakila"], par, options),
    "tables": lambda par, options: util.dump_tables("sakila", sakila_tables, par, options)
}

prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)

good_pars = {
    "prefix": create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectReadWrite", "all-read-par", today_plus_days(1, RFC3339), f'prefix/', "ListObjects"),
    "bucket": create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectReadWrite", "all-read-par", today_plus_days(1, RFC3339), None, "ListObjects"),
}

# BUG#35548572 - PARs using dedicated endpoints
for par_type, par in list(good_pars.items()):
    good_pars[par_type + "-converted"] = convert_par(par)

#@<> Testing PAR with conflicting options
for par_type, par in good_pars.items():
    for name, callback in dump_util_cb.items():
        for  option_name, options in conflicting_options.items():
            print(f"--> Testing util.dump_{name} using {par_type} PAR with option {option_name}")
            EXPECT_THROWS(lambda: callback(par, options), f"The option '{option_name}' can not be used when using a PAR as the target output url.")


#@<> Testing PARs with bad attributions
bad_pars = {
    "bucket-read": create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), None, "Deny"),
    "bucket-write": create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectWrite", "all-read-par", today_plus_days(1, RFC3339), None, "Deny"),
    "bucket-read-write": create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectReadWrite", "all-read-par", today_plus_days(1, RFC3339), None, "Deny"),
    "bucket-read-list": create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), None, "ListObjects"),
    "prefix-read": create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), f'prefix/', "Deny"),
    "prefix-write": create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectWrite", "all-read-par", today_plus_days(1, RFC3339), f'prefix/', "Deny"),
    "prefix-read-write": create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectReadWrite", "all-read-par", today_plus_days(1, RFC3339), f'prefix/', "Deny"),
    "prefix-read-list": create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), f'prefix/', "ListObjects"),
    "mistmatched-prefix": create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectReadWrite", "all-read-par", today_plus_days(1, RFC3339), f'prefix/', "ListObjects").replace("prefix", "other"),
    "object-par": create_par(OS_NAMESPACE, OS_BUCKET_NAME, "ObjectReadWrite", "all-read-par", today_plus_days(1, RFC3339), f'target-object', "ListObjects"),
    "http-par":  create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectReadWrite", "all-read-par", today_plus_days(1, RFC3339), f'prefix/', "ListObjects").replace("https", "http"),
    "random-par": create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectReadWrite", "all-read-par", today_plus_days(1, RFC3339), f'prefix/', "ListObjects")
}

# Scrambles the secure token portion of the random-par
par_tokens = bad_pars['random-par'].split('/')
secure_token = list(par_tokens[4])
shuffle(secure_token)
par_tokens[4] = ''.join(secure_token)
bad_pars['random-par'] = '/'.join(par_tokens)


anonymous_bucket_par = anonymize_par(bad_pars["bucket-read"])
no_access_error = f"Could not access '{anonymous_bucket_par}': BucketNotFound: Either the bucket named '{OS_BUCKET_NAME}' does not exist in the namespace '{OS_NAMESPACE}' or you are not authorized to access it (404)"
no_write_bucket_error = f"Failed to put object '{anonymous_bucket_par}@.json': BucketNotFound: Either the bucket named '{OS_BUCKET_NAME}' does not exist in the namespace '{OS_NAMESPACE}' or you are not authorized to access it (404)"
no_write_prefix_error = f"Failed to put object '{anonymous_bucket_par}prefix/@.json': BucketNotFound: Either the bucket named '{OS_BUCKET_NAME}' does not exist in the namespace '{OS_NAMESPACE}' or you are not authorized to access it (404)"
prefix_mistmatch_error = f"Could not access '{anonymous_bucket_par}': NotAuthenticated: The prefix from the request and the prefix from PAR are not compatible (401)"
random_par_error = f"Could not access '{anonymous_bucket_par}': NotAuthenticated: PAR does not exist (401)"


bad_par_errors = {
    "bucket-read": no_access_error,
    "bucket-write": no_access_error,
    "bucket-read-write": no_access_error,
    "bucket-read-list": no_write_bucket_error,
    "prefix-read": no_access_error,
    "prefix-write": no_access_error,
    "prefix-read-write": no_access_error,
    "prefix-read-list": no_write_prefix_error,
    "mistmatched-prefix": prefix_mistmatch_error,
    "object-par": "The given URL is not a prefix PAR.",
    "http-par": "Directory handling for http protocol is not supported.",
    "random-par": random_par_error
}

for par_type, par in bad_pars.items():
    for name, callback in dump_util_cb.items():
        print(f"--> Testing util.dump_{name} using {par_type} PAR")
        PREPARE_PAR_IS_SECRET_TEST()
        EXPECT_THROWS(lambda: callback(par, None), bad_par_errors[par_type])
        EXPECT_PAR_IS_SECRET()

#<> Testing dump attempt, with existing data in the target bucket/prefix
for par_type, par in good_pars.items():
    # Performs an initial dump, to ensure there's data in the target location for the following tests
    print(f"--> Preparing util.dump_{name} test with existing files in target location using {par_type} PAR")
    prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE, False)
    util.dump_tables("sakila", ["category"], par)
    for name, callback in dump_util_cb.items():
        print(f"--> Testing util.dump_{name} using {par_type} PAR with existing files in target location")
        error_par = anonymize_par(par, False)
        EXPECT_THROWS(lambda: callback(par, None), f"Cannot proceed with the dump, OCI prefix PAR={error_par} already contains files with the specified prefix")


#@<> Testing successful dump utilities using PAR
for par_type, par in good_pars.items():
    for name, callback in dump_util_cb.items():
        print(f"--> Testing util.dump_{name} using {par_type} PAR")
        prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE, False)
        shell.connect(__sandbox_uri1)
        EXPECT_NO_THROWS(lambda: callback(par, None), f"Unexpected error calling util.dump_{name} with {par_type} PAR")
        session.close()
        print(f"--> Loading dump created by util.dump_{name} using {par_type} PAR")
        shell.connect(__sandbox_uri2)
        PREPARE_PAR_IS_SECRET_TEST()
        EXPECT_NO_THROWS(lambda: util.load_dump(par, {"progressFile":"progress.txt"}), f"Unexpected error loading dump using {par_type} PAR.")
        EXPECT_PAR_IS_SECRET()
        testutil.rmfile("progress.txt")
        # Used for events, triggers and stored procedures
        check_all = name != "tables"
        compare_instances(check_all, check_all, check_all)
        session.run_sql("drop schema sakila")
        session.close()

#@<> Cleanup
testutil.stop_sandbox(__mysql_sandbox_port1, {"wait":1})
testutil.stop_sandbox(__mysql_sandbox_port2, {"wait":1})
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)

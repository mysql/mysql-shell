## imports

import configparser
from contextlib import ExitStack
import datetime
import os
import os.path

## helpers

def get_options(opts = {}):
    if not hasattr(get_options, "aws_options"):
        get_options.aws_options = {}
        def set_option(name, variable):
            value = globals().get(variable)
            if value is not None:
                get_options.aws_options[name] = value
        set_option("s3BucketName", "MYSQLSH_S3_BUCKET_NAME")
        set_option("s3CredentialsFile", "MYSQLSH_AWS_SHARED_CREDENTIALS_FILE")
        set_option("s3ConfigFile", "MYSQLSH_AWS_CONFIG_FILE")
        set_option("s3Profile", "MYSQLSH_AWS_PROFILE")
        set_option("s3EndpointOverride", "MYSQLSH_S3_ENDPOINT_OVERRIDE")
    return {**get_options.aws_options, **opts}

def clean_bucket():
    return testutil.clean_s3_bucket(get_options())

def delete_object(name):
    testutil.delete_s3_object(name, get_options())

def delete_objects(names):
    testutil.delete_s3_objects(names, get_options())

def setup_tests(load_tests = False):
    clean_bucket()
    testutil.deploy_sandbox(__mysql_sandbox_port1, "root")
    setup_session(__sandbox_uri1)
    session.run_sql("CREATE SCHEMA `world`")
    testutil.import_data(__sandbox_uri1, os.path.join(__data_path, "sql", "world.sql"), "world")
    for schema, in session.run_sql("SHOW SCHEMAS").fetch_all():
        if schema in ("mysql", "sys", "performance_schema", "information_schema"):
            continue
        for table, in session.run_sql("SHOW TABLES IN !", [ schema ]).fetch_all():
            session.run_sql("ANALYZE TABLE !.!", [ schema, table ])
    if load_tests:
        testutil.deploy_sandbox(__mysql_sandbox_port2, "root", {"local_infile": 1})

def setup_session(u):
    shell.connect(u)
    session.run_sql("SET NAMES 'utf8mb4'")

def cleanup_tests(load_tests = False):
    clean_bucket()
    testutil.destroy_sandbox(__mysql_sandbox_port1)
    if load_tests:
        testutil.destroy_sandbox(__mysql_sandbox_port2)

def remove_local_progress_file():
    remove_file(local_progress_file)

def read_profile():
    config = configparser.ConfigParser()
    config.read(MYSQLSH_AWS_CONFIG_FILE)
    config.read(MYSQLSH_AWS_SHARED_CREDENTIALS_FILE)
    profile = {}
    for key, value in config.items(MYSQLSH_AWS_PROFILE) + ([] if "default" == MYSQLSH_AWS_PROFILE else config.items("profile " + MYSQLSH_AWS_PROFILE)):
        profile[key] = value
    return profile

def write_profile(path, profile, settings, extra_profiles = None):
    stack = ExitStack()
    directory = os.path.dirname(path)
    if not os.path.isdir(directory):
        os.mkdir(directory)
        stack.callback(lambda: os.rmdir(directory))
    if os.path.isfile(path):
        backup = path + ".backup"
        suffix = ""
        i = 0
        while os.path.isfile(f"{backup}{suffix}"):
            suffix = f".{i}"
            i += 1
        backup += suffix
        os.rename(path, backup)
        stack.callback(lambda: os.rename(backup, path))
    config = configparser.ConfigParser()
    config.read_dict({ profile: settings })
    if extra_profiles is not None:
        config.read_dict(extra_profiles)
    with open(path, "w") as f:
        config.write(f)
    stack.callback(lambda: os.remove(path))
    return stack

def get_scope(region):
    return f"{datetime.datetime.today().strftime('%Y%m%d')}/{region}/s3/aws4_request"

## constants

local_progress_file = "my_load_progress.txt"

default_aws_credentials_file = os.path.join(__home, ".aws", "credentials")
default_aws_config_file = os.path.join(__home, ".aws", "config")
default_aws_profile = "default"
default_s3_endpoint_override = ""
default_aws_region = "us-east-1"

local_aws_credentials_file = os.path.join(__tmp_dir, "aws_credentials")
local_aws_config_file = os.path.join(__tmp_dir, "aws_config")
local_aws_profile = random_string(10)

dump_dir = "world"

## initialization

def set_global_variable(name, default_value):
    value = globals().get(name)
    if value is None:
        globals()[name] = default_value

# call it to initialize the AWS options
get_options()
# set global variable, if they were not specified
set_global_variable("MYSQLSH_AWS_SHARED_CREDENTIALS_FILE", default_aws_credentials_file)
set_global_variable("MYSQLSH_AWS_CONFIG_FILE", default_aws_config_file)
set_global_variable("MYSQLSH_AWS_PROFILE", default_aws_profile)
set_global_variable("MYSQLSH_S3_ENDPOINT_OVERRIDE", default_s3_endpoint_override)

aws_settings = read_profile()

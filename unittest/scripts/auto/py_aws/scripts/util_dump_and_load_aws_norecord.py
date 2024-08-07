#@ {has_aws_environment()}

#@<> INCLUDE dump_utils.inc

#@<> INCLUDE aws_utils.inc

#@<> imports
from http.server import BaseHTTPRequestHandler
import datetime
import os.path
import random
import threading

#@<> setup
setup_tests(load_tests = True)

#@<> helpers
def EXPECT_SUCCESS(options = {}, excluded_options = [], all = True):
    aws_options = get_options(options, excluded_options)
    print(f"--> EXPECT_SUCCESS -> {aws_options}")
    def dump(method, *args):
        method = "dump_" + method
        print(f"--> Executing util.{method}()")
        setup_session(__sandbox_uri1)
        clean_bucket()
        EXPECT_NO_THROWS(lambda: util[method](*args, dump_dir, aws_options), f"Failed to dump using options: {aws_options}")
    def load():
        print("--> Executing util.load_dump()")
        setup_session(__sandbox_uri2)
        wipeout_server(session)
        EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, aws_options), f"Failed to load using options: {aws_options}")
    dump("instance")
    load()
    if all:
        dump("schemas", [ "world" ])
        load()
        dump("tables", "world", [ "Country" ])
        load()

def EXPECT_FAIL(error, msg, options = {}, excluded_options = [], argument_error = True, all = True):
    aws_options = get_options(options, excluded_options)
    print(f"--> EXPECT_FAIL -> {aws_options}")
    def full_msg(method, options_arg):
        is_re = is_re_instance(msg)
        m = f"{re.escape(error) if is_re else error}: {f'Argument #{options_arg}: ' if argument_error else ''}{msg.pattern if is_re else msg}"
        if is_re:
            m = re.compile("^" + m)
        return m
    def dump(method, *args):
        method = "dump_" + method
        print(f"--> Executing util.{method}()")
        setup_session(__sandbox_uri1)
        clean_bucket()
        EXPECT_THROWS(lambda: util[method](*args, dump_dir, aws_options), full_msg(method, 2 + len(args)))
    def load():
        print("--> Executing util.load_dump()")
        setup_session(__sandbox_uri2)
        wipeout_server(session)
        EXPECT_THROWS(lambda: util.load_dump(dump_dir, aws_options), full_msg("load_dump", 2))
    dump("instance")
    if all:
        dump("schemas", [ "world" ])
        dump("tables", "world", [ "Country" ])
        load()

class HandleHTTPRequest(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    content_type = 'application/xml'
    def do_GET(self):
        response = self.response() if callable(self.response) else self.response
        self.send_response(self.status_code)
        self.send_header('Content-Type', self.content_type)
        self.send_header('Content-Length', str(len(response)))
        self.end_headers()
        self.wfile.write(response.encode('ascii'))
        self.close_connection = True
    def log_message(self, format, *args):
        pass

#@<> WL14387-TSFR_1
help_text = """
      - s3BucketName: string (default: not set) - Name of the AWS S3 bucket to
        use. The bucket must already exist.
      - s3CredentialsFile: string (default: not set) - Use the specified AWS
        credentials file.
      - s3ConfigFile: string (default: not set) - Use the specified AWS config
        file.
      - s3Profile: string (default: not set) - Use the specified AWS profile.
      - s3Region: string (default: not set) - Use the specified AWS region.
      - s3EndpointOverride: string (default: not set) - Use the specified AWS
        S3 API endpoint instead of the default one.
"""

EXPECT_TRUE(help_text in util.help("dump_instance"))
EXPECT_TRUE(help_text in util.help("dump_schemas"))
EXPECT_TRUE(help_text in util.help("dump_tables"))
EXPECT_TRUE(help_text in util.help("load_dump"))

#@<> WL14387-TSFR_1_1_2
EXPECT_SUCCESS()

#@<> WL14387-TSFR_2_1_2
WIPE_SHELL_LOG()
EXPECT_FAIL("RuntimeError", "Could not select the AWS credentials provider, please see log for more details", { "s3Profile": "non-existing-profile" })
EXPECT_SHELL_LOG_CONTAINS(f"Warning: The credentials file at '{MYSQLSH_AWS_SHARED_CREDENTIALS_FILE}' does not contain a profile named: 'non-existing-profile'.")
EXPECT_SHELL_LOG_CONTAINS(f"Warning: The config file at '{MYSQLSH_AWS_CONFIG_FILE}' does not contain a profile named: 'non-existing-profile'.")
EXPECT_SHELL_LOG_CONTAINS("Info: The environment variables are not going to be used to fetch AWS credentials, because the 's3Profile' option is set.")

#@<> WL14387-TSFR_2_1_4
#    WL14387-TSFR_4_2_1
with write_profile(local_aws_config_file, "profile " + local_aws_profile, aws_settings):
    EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file })

#@<> WL14387-TSFR_2_1_5
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "region": aws_settings.get("region", default_aws_region) }):
    with write_profile(local_aws_credentials_file, local_aws_profile, aws_settings):
        EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })

#@<> WL14387-TSFR_3_1
#    WL14387-TSFR_3_1_2_1
#    WL14387-TSFR_3_2_1
with write_profile(default_aws_config_file, "profile " + local_aws_profile, { "region": aws_settings.get("region", default_aws_region) }):
    with write_profile(default_aws_credentials_file, local_aws_profile, aws_settings):
        # empty strings cause dumper/loader to use files from the default location
        EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": "", "s3CredentialsFile": "" })

#@<> WL14387-TSFR_3_1_1
with write_profile(default_aws_config_file, "profile " + local_aws_profile, { "region": aws_settings.get("region", default_aws_region) }):
    with write_profile(default_aws_credentials_file, local_aws_profile, { "aws_access_key_id": "invalid", "aws_secret_access_key": "invalid" }):
        with write_profile(local_aws_credentials_file, local_aws_profile, aws_settings):
            # empty strings cause dumper/loader to use files from the default location
            EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": "", "s3CredentialsFile": local_aws_credentials_file })

#@<> WL14387-TSFR_3_2_2
current_log_level = shell.options["logLevel"]
shell.options["logLevel"] = 8
aws_session_token = random_string(20)

with write_profile(local_aws_credentials_file, local_aws_profile, { **aws_settings, "aws_session_token": aws_session_token }):
    # token is temporary, so we cannot use a predefined one for our testing, instead we check if the token we set is sent along with the headers
    setup_session(__sandbox_uri1)
    clean_bucket()
    WIPE_SHELL_LOG()
    try:
        util.dump_instance(dump_dir, get_options({ "s3Profile": local_aws_profile, "s3CredentialsFile": local_aws_credentials_file }))
    except Exception as e:
        pass
    EXPECT_SHELL_LOG_CONTAINS(f"'x-amz-security-token' : '{aws_session_token}'")

shell.options["logLevel"] = current_log_level

#@<> WL14387-TSFR_4_1
with write_profile(local_aws_config_file, "profile " + local_aws_profile, {}):
    with write_profile(local_aws_credentials_file, local_aws_profile, {}):
        EXPECT_FAIL("RuntimeError", f"The AWS access and secret keys were not found, tried: credentials file ({local_aws_credentials_file}, profile: {local_aws_profile}), config file ({local_aws_config_file}, profile: {local_aws_profile}", { "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })

#@<> WL14387-TSFR_4_2
with write_profile(local_aws_config_file, "profile " + local_aws_profile, {}):
    with write_profile(local_aws_credentials_file, local_aws_profile, {}):
        with open(local_aws_config_file, "a") as f:
            f.write("invalid line")
        EXPECT_FAIL("RuntimeError", f"Failed to parse config file '{local_aws_config_file}': expected setting-name=value, in line 3: invalid line", { "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })

with write_profile(local_aws_config_file, "profile " + local_aws_profile, {}):
    with write_profile(local_aws_credentials_file, local_aws_profile, {}):
        with open(local_aws_credentials_file, "a") as f:
            f.write("invalid line")
        EXPECT_FAIL("RuntimeError", f"Failed to parse config file '{local_aws_credentials_file}': expected setting-name=value, in line 3: invalid line", { "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })

#@<> WL14387-TSFR_4_1_1
with write_profile(default_aws_config_file, "profile " + local_aws_profile, { "aws_access_key_id": "invalid", "aws_secret_access_key": "invalid" }):
    with write_profile(default_aws_credentials_file, local_aws_profile, {}):
        with write_profile(local_aws_config_file, local_aws_profile, aws_settings):
            # empty strings cause dumper/loader to use files from the default location
            EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": "" })

#@<> WL14387-TSFR_4_1_2_1
with write_profile(default_aws_config_file, "profile " + local_aws_profile, aws_settings):
    # empty strings cause dumper/loader to use files from the default location
    EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": "", "s3CredentialsFile": "" })

#@<> WL14387-TSFR_4_2_2
current_log_level = shell.options["logLevel"]
shell.options["logLevel"] = 8
aws_session_token = random_string(20)

with write_profile(local_aws_config_file, "profile " + local_aws_profile, { **aws_settings, "aws_session_token": aws_session_token }):
    # token is temporary, so we cannot use a predefined one for our testing, instead we check if the token we set is sent along with the headers
    setup_session(__sandbox_uri1)
    clean_bucket()
    WIPE_SHELL_LOG()
    try:
        util.dump_instance(dump_dir, get_options({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file }))
    except Exception as e:
        pass
    EXPECT_SHELL_LOG_CONTAINS(f"'x-amz-security-token' : '{aws_session_token}'")

shell.options["logLevel"] = current_log_level

#@<> WL14387-TSFR_4_2_3
current_log_level = shell.options["logLevel"]
shell.options["logLevel"] = 8
aws_region = random_string(20)

with write_profile(local_aws_config_file, "profile " + local_aws_profile, { **aws_settings, "region": aws_region }):
    # we have a fixed region set in our credentials, in order to test if the region from the config file is used, we check the headers, and don't care about the result
    setup_session(__sandbox_uri1)
    clean_bucket()
    WIPE_SHELL_LOG()
    try:
        util.dump_instance(dump_dir, get_options({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file }))
    except Exception as e:
        pass
    EXPECT_SHELL_LOG_CONTAINS(get_scope(aws_region))

shell.options["logLevel"] = current_log_level

#@<> WL14387-TSFR_4_2_1_1
current_log_level = shell.options["logLevel"]
shell.options["logLevel"] = 8
aws_region = random_string(20)

with write_profile(local_aws_config_file, "profile " + local_aws_profile, {}):
    with write_profile(local_aws_credentials_file, local_aws_profile, { **aws_settings, "region": aws_region }):
        # we have a fixed region set in our credentials, in order to test if the default region is used, we check the headers, and don't care about the result
        setup_session(__sandbox_uri1)
        clean_bucket()
        WIPE_SHELL_LOG()
        try:
            util.dump_instance(dump_dir, get_options({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file }))
        except Exception as e:
            pass
        EXPECT_SHELL_LOG_CONTAINS(get_scope("us-east-1"))

shell.options["logLevel"] = current_log_level

#@<> WL14387-TSFR_5_1
# BUG#34604763 - partial credentials are an error
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "region": aws_settings.get("region", default_aws_region), "aws_access_key_id": aws_settings["aws_access_key_id"] }):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "aws_secret_access_key": aws_settings["aws_secret_access_key"] }):
        EXPECT_FAIL("RuntimeError", f"Partial AWS credentials found in credentials file ({local_aws_credentials_file}, profile: {local_aws_profile}), missing the value of 'aws_access_key_id'", { "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })

#@<> WL14387-TSFR_5_2
current_log_level = shell.options["logLevel"]
shell.options["logLevel"] = 8
aws_session_token = random_string(20)

with write_profile(local_aws_config_file, "profile " + local_aws_profile, { **aws_settings, "aws_session_token": aws_session_token }):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "region": aws_settings.get("region", default_aws_region), "aws_session_token": "invalid" }):
        # token is temporary, so we cannot use a predefined one for our testing, instead we check if the token we set is sent along with the headers
        setup_session(__sandbox_uri1)
        clean_bucket()
        WIPE_SHELL_LOG()
        try:
            util.dump_instance(dump_dir, get_options({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file }))
        except Exception as e:
            pass
        EXPECT_SHELL_LOG_CONTAINS(f"'x-amz-security-token' : '{aws_session_token}'")

shell.options["logLevel"] = current_log_level

#@<> WL14387-TSFR_5_3
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "region": aws_settings.get("region", default_aws_region), "aws_access_key_id": "invalid", "aws_secret_access_key": "invalid" }):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "aws_access_key_id": aws_settings["aws_access_key_id"], "aws_secret_access_key": aws_settings["aws_secret_access_key"] }, { "invalid-profile": { "aws_access_key_id": "invalid", "aws_secret_access_key": "invalid" } }):
        EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })

#@<> WL14387-TSFR_5_1_1
with write_profile(default_aws_config_file, "profile " + local_aws_profile, {}):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "aws_secret_access_key": aws_settings["aws_secret_access_key"] }):
        EXPECT_FAIL("RuntimeError", f"Partial AWS credentials found in credentials file ({local_aws_credentials_file}, profile: {local_aws_profile}), missing the value of 'aws_access_key_id'", { "s3Profile": local_aws_profile, "s3ConfigFile": "", "s3CredentialsFile": local_aws_credentials_file })

#@<> WL14387-TSFR_5_2_1
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "aws_secret_access_key": "", "aws_access_key_id": "invalid" }):
    with write_profile(local_aws_credentials_file, local_aws_profile, {}):
        # aws_access_key_id is invalid, so we're expecting an 403 Forbidden error
        EXPECT_FAIL("Error: Shell Error (54403)", re.compile(".* \(403\)"), { "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file }, argument_error = False)

#@<> WL14387-TSFR_5_2_2
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "region": aws_settings.get("region", default_aws_region) }):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "aws_access_key_id": aws_settings["aws_access_key_id"] }):
        EXPECT_FAIL("RuntimeError", f"Partial AWS credentials found in credentials file ({local_aws_credentials_file}, profile: {local_aws_profile}), missing the value of 'aws_secret_access_key'", { "s3Profile": local_aws_profile, "s3ConfigFile": "", "s3CredentialsFile": local_aws_credentials_file })

#@<> WL14387-TSFR_5_3_1
current_log_level = shell.options["logLevel"]
shell.options["logLevel"] = 8

with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "region": aws_settings.get("region", default_aws_region), "aws_session_token": "" }):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "aws_access_key_id": aws_settings["aws_access_key_id"], "aws_secret_access_key": aws_settings["aws_secret_access_key"] }):
        WIPE_SHELL_LOG()
        EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })
        # with empty aws_session_token setting, the x-amz-security-token header should not be used
        EXPECT_SHELL_LOG_NOT_CONTAINS(f"'x-amz-security-token'")

shell.options["logLevel"] = current_log_level

#@<> WL14387-TSFR_6_2_1
current_log_level = shell.options["logLevel"]
shell.options["logLevel"] = 8

with write_profile(local_aws_config_file, "profile " + local_aws_profile, { **aws_settings, "region": default_aws_region }):
    # we're not checking if operation succeeds or not, we're checking if the default endpoint is used
    setup_session(__sandbox_uri1)
    clean_bucket()
    WIPE_SHELL_LOG()
    try:
        util.dump_instance(dump_dir, get_options({ "s3EndpointOverride": "", "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file }))
    except Exception as e:
        pass
    EXPECT_SHELL_LOG_CONTAINS(f"s3.{default_aws_region}.amazonaws.com GET")

shell.options["logLevel"] = current_log_level

#@<> WL14387-TSFR_6_3_1
class RejectGet(HandleHTTPRequest):
    status_code = 403
    response = "<Error><Message>Access forbidden!</Message></Error>"

http_server = Http_test_server(RejectGet)
http_server.start()

current_log_level = shell.options["logLevel"]
shell.options["logLevel"] = 8

with write_profile(local_aws_config_file, "profile " + local_aws_profile, { **aws_settings, "region": default_aws_region }):
    # operation will fail, because our test server sends 403, we're checking here if the specified endpoint is used
    setup_session(__sandbox_uri1)
    clean_bucket()
    WIPE_SHELL_LOG()
    EXPECT_THROWS(lambda: util.dump_instance(dump_dir, get_options({ "s3EndpointOverride": http_server.url(), "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file })), "Access forbidden! (403)")
    EXPECT_SHELL_LOG_CONTAINS(f"{http_server.url()} GET")

shell.options["logLevel"] = current_log_level

http_server.stop()

#@<> WL14387-TSFR_6_3_2
current_log_level = shell.options["logLevel"]
shell.options["logLevel"] = 8

with write_profile(local_aws_config_file, "profile " + local_aws_profile, { **aws_settings, "region": default_aws_region }):
    # we're not checking if operation succeeds or not, we're checking if the default endpoint is used
    setup_session(__sandbox_uri1)
    clean_bucket()
    WIPE_SHELL_LOG()
    try:
        util.dump_instance(dump_dir, get_options({ "s3EndpointOverride": "https://mynamespace.compat.objectstorage.us-phoenix-1.oraclecloud.com", "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file }))
    except Exception as e:
        pass
    EXPECT_SHELL_LOG_CONTAINS("https://mynamespace.compat.objectstorage.us-phoenix-1.oraclecloud.com GET")

shell.options["logLevel"] = current_log_level

#@<> WL14387-TSFR_ET_1
with write_profile(local_aws_config_file, "profile " + local_aws_profile, aws_settings):
    with write_profile(local_aws_credentials_file, local_aws_profile + "-invalid", {}):
        EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })

#@<> BUG#34599319 - dump&load when paths contain spaces and other characters that need to be URL-encoded
tested_schema = "test schema"
tested_table = "fish & chips"
bug_34599319_dump_dir = "shell test/dump & load"

dump_session.run_sql("DROP SCHEMA IF EXISTS !;", [ tested_schema ])
dump_session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
dump_session.run_sql("CREATE TABLE !.! (id INT PRIMARY KEY);", [ tested_schema, tested_table ])
dump_session.run_sql("INSERT INTO !.! (id) VALUES (1234);", [ tested_schema, tested_table ])
dump_session.run_sql("ANALYZE TABLE !.!;", [ tested_schema, tested_table ])

aws_options = get_options({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file })

# prepare the dump
clean_bucket()
setup_session(__sandbox_uri1)

with write_profile(local_aws_config_file, "profile " + local_aws_profile, aws_settings):
    util.dump_schemas([tested_schema], bug_34599319_dump_dir, aws_options)

#@<> BUG#34599319 - test
setup_session(__sandbox_uri2)
wipeout_server(session)

with write_profile(local_aws_config_file, "profile " + local_aws_profile, aws_settings):
    EXPECT_NO_THROWS(lambda: util.load_dump(bug_34599319_dump_dir, aws_options), "load_dump() with URL-encoded file names")

EXPECT_STDOUT_CONTAINS(f"1 tables in 1 schemas were loaded")

#@<> BUG#34599319 - cleanup
dump_session.run_sql("DROP SCHEMA IF EXISTS !;", [ tested_schema ])

#@<> BUG#34604763 - new s3Region option
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "region": "invalid" }):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "aws_access_key_id": aws_settings["aws_access_key_id"], "aws_secret_access_key": aws_settings["aws_secret_access_key"] }):
        EXPECT_SUCCESS({ "s3Region": aws_settings.get("region", default_aws_region), "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })

#@<> BUG#34840953 - support for credential_process setting
# setup
creds_script = os.path.abspath("creds.py")
creds_control = os.path.abspath("control")

with open(creds_script, 'w', encoding="utf-8") as f:
    f.write(f"""# returns hardcoded credentials with optional expiration time
import datetime
import json
import sys

creds = {{
    'Version': 1,
    'AccessKeyId': '{aws_settings["aws_access_key_id"]}',
    'SecretAccessKey': '{aws_settings["aws_secret_access_key"]}'
}}

expiration = '<permanent>'

if len(sys.argv) > 1:
    expiration = (datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(milliseconds = int(sys.argv[1]))).isoformat()
    creds['Expiration'] = expiration

with open('{creds_control}', 'a', encoding="utf-8") as f:
    f.write(expiration)
    f.write('\\n')

print(json.dumps(creds))
""")

def write_expiration(ms):
    expiration = (datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(milliseconds = ms)).isoformat()
    with open(creds_control, 'a', encoding="utf-8") as f:
        f.write(expiration)
        f.write('\n')
    return expiration

def read_control_file():
    with open(creds_control, 'r', encoding="utf-8") as f:
        control = f.read()
        f.close()
        os.remove(creds_control)
    return control

#@<> BUG#34840953 - no expiration
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "credential_process": f"{__mysqlsh} --py --file {creds_script}" }):
    EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file })

# we're dumping and loading 3 times, process should be executed 6 times total
EXPECT_EQ(6, read_control_file().count('\n'))

#@<> BUG#34840953 - very short expiration time
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "credential_process": f"{__mysqlsh} --py --file {creds_script} 100" }):
    EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file })

# expiration time is 100ms, process should be executed multiple times
EXPECT_LT(12, read_control_file().count('\n'))

#@<> BUG#34840953 - very long expiration time
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "credential_process": f"{__mysqlsh} --py --file {creds_script} 600000" }):
    EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file })

# credentials do not expire during the test, process should be executed six times
EXPECT_EQ(6, read_control_file().count('\n'))

#@<> BUG#34840953 - cleanup
if os.path.exists(creds_script):
    os.remove(creds_script)

#@<> BUG#35027093 - retry in case of AWS errors returned for an expired token
# setup
creds_script = os.path.abspath("creds.py")

with open(creds_script, 'w', encoding="utf-8") as f:
    f.write(f"""# returns hardcoded credentials with expiration time in the past
import datetime
import json

creds = {{
    'Version': 1,
    'AccessKeyId': 'ACCESS-KEY',
    'SecretAccessKey': 'SECRET-ACCESS-KEY',
    'SessionToken': 'SESSION-TOKEN',
    'Expiration': (datetime.datetime.now(datetime.timezone.utc) - datetime.timedelta(milliseconds = 100)).isoformat()
}}

print(json.dumps(creds))
""")

class ExpiredToken(HandleHTTPRequest):
    status_code = 400
    response = """<?xml version="1.0" encoding="UTF-8"?>
<Error>
  <Code>ExpiredToken</Code>
  <Message>The provided token has expired.</Message>
  <Token-0>SESSION-TOKEN</Token-0>
  <RequestId>REQUEST-ID</RequestId>
  <HostId>HOST-ID</HostId>
</Error>
"""

class TokenRefreshRequired(HandleHTTPRequest):
    status_code = 400
    response = """<?xml version="1.0" encoding="UTF-8"?>
<Error>
  <Code>TokenRefreshRequired</Code>
  <Message>The provided token must be refreshed.</Message>
  <Token-0>SESSION-TOKEN</Token-0>
  <RequestId>REQUEST-ID</RequestId>
  <HostId>HOST-ID</HostId>
</Error>
"""

def TEST_BUG35027093(http_server, expected_error):
    WIPE_SHELL_LOG()
    EXPECT_THROWS(lambda: util.dump_instance(dump_dir, get_options({ "s3EndpointOverride": http_server.url(), "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file })), expected_error)
    EXPECT_SHELL_LOG_CONTAINS_COUNT("Refreshing authentication data", 2)
    EXPECT_SHELL_LOG_CONTAINS_COUNT("Retrying a request which failed due to an authorization error", 2)

# BUG#35468541 - expired HEAD request
class FailHeadRequest(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    def do_HEAD(self):
        self.send_response(400)
        self.send_header('Content-Type', 'application/xml')
        self.send_header('Content-Length', 123)
        self.end_headers()
        self.close_connection = True
    def log_message(self, format, *args):
        pass

def TEST_BUG35468541(http_server, expected_error):
    WIPE_SHELL_LOG()
    EXPECT_THROWS(lambda: util.load_dump(dump_dir, get_options({ "s3EndpointOverride": http_server.url(), "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file })), expected_error)
    EXPECT_SHELL_LOG_CONTAINS_COUNT("Refreshing authentication data", 2)
    EXPECT_SHELL_LOG_CONTAINS_COUNT("Retrying a request which failed due to an authorization error", 2)

expired_token_server = Http_test_server(ExpiredToken)
expired_token_server.start()

token_refresh_server = Http_test_server(TokenRefreshRequired)
token_refresh_server.start()

fail_head_server = Http_test_server(FailHeadRequest)
fail_head_server.start()

#@<> BUG#35027093 + BUG#35468541 - test
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "credential_process": f"{__mysqlsh} --py --file {creds_script}" }):
    TEST_BUG35027093(expired_token_server, "The provided token has expired. (400)")
    TEST_BUG35027093(token_refresh_server, "The provided token must be refreshed. (400)")
    TEST_BUG35468541(fail_head_server, "Bad Request (400)")

#@<> BUG#35027093 + BUG#35468541 - cleanup
expired_token_server.stop()
token_refresh_server.stop()
fail_head_server.stop()

if os.path.exists(creds_script):
    os.remove(creds_script)

#@<> WL#15885 - ECS credentials
# setup
class EcsCredentials(HandleHTTPRequest):
    status_code = 200
    content_type = 'application/json'
    def response(self):
        expiration = write_expiration(100)
        return f"""{{
    "AccessKeyId": "{aws_settings["aws_access_key_id"]}",
    "SecretAccessKey": "{aws_settings["aws_secret_access_key"]}",
    "Expiration": "{expiration}"
}}
"""

ecs_credentials_server = Http_test_server(EcsCredentials)
ecs_credentials_server.start()

#@<> WL#15885 - ECS credentials - test
with set_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI", ecs_credentials_server.url()):
    EXPECT_SUCCESS({ "s3Profile": "non-existing-profile" }, all = False)

# expiration time is 100ms, credentials should be fetched multiple times
EXPECT_LT(4, read_control_file().count('\n'))

#@<> WL#15885 - ECS credentials - cleanup
ecs_credentials_server.stop()

#@<> WL#15885 - IMDS credentials
# setup
class ImdsCredentials(HandleHTTPRequest):
    token_path = '/latest/api/token'
    credentials_path = '/latest/meta-data/iam/security-credentials/'
    security_token = '1234567890'
    imds_role = 'some-role'
    def reply(self, response):
        self.send_response(200)
        self.send_header('Content-Type', 'text/plain')
        self.send_header('Content-Length', str(len(response)))
        self.end_headers()
        self.wfile.write(response.encode('ascii'))
    def error(self):
        self.reply('{"Code": "UnexpectedError", "Message": "This was unexpected"}')
    def credentials(self):
        expiration = write_expiration(100)
        return f"""{{
    "AccessKeyId": "{aws_settings["aws_access_key_id"]}",
    "SecretAccessKey": "{aws_settings["aws_secret_access_key"]}",
    "Token": "",
    "Expiration": "{expiration}"
}}
"""
    def do_PUT(self):
        if self.path == self.token_path and self.headers.get('x-aws-ec2-metadata-token-ttl-seconds', None):
            self.reply(self.security_token)
        else:
            self.error()
    def do_GET(self):
        if self.headers.get('x-aws-ec2-metadata-token', None) == self.security_token:
            if self.path == self.credentials_path:
                self.reply(self.imds_role)
            elif self.path == self.credentials_path + self.imds_role:
                self.reply(self.credentials())
            else:
                self.error()
        else:
            self.error()

imds_credentials_server = Http_test_server(ImdsCredentials)
imds_credentials_server.start()

#@<> WL#15885 - IMDS credentials - test
with set_env_var("AWS_EC2_METADATA_DISABLED", "false"):
    with set_env_var("AWS_EC2_METADATA_SERVICE_ENDPOINT", imds_credentials_server.url()):
        EXPECT_SUCCESS({ "s3Profile": "non-existing-profile" }, all = False)

# expiration time is 100ms, credentials should be fetched multiple times
EXPECT_LT(4, read_control_file().count('\n'))

#@<> WL#15885 - IMDS credentials - cleanup
imds_credentials_server.stop()

#@<> WL15885-TSFR_1_2_1_1 - assume role - environment {MYSQLSH_AWS_ROLE is not None}
# WL15885-TSFR_4_1
with set_env_var("AWS_ACCESS_KEY_ID", aws_settings["aws_access_key_id"]):
    with set_env_var("AWS_SECRET_ACCESS_KEY", aws_settings["aws_secret_access_key"]):
        with write_profile(local_aws_config_file, "profile " + local_aws_role_profile, { "role_arn": MYSQLSH_AWS_ROLE, "credential_source" : "Environment", "region": aws_settings.get("region", default_aws_region) }):
            # WL15885-TSFR_1_1_2_2 - AWS_WEB_IDENTITY_TOKEN_FILE is only applicable when assuming a role with web identity provider - regular assume role is used
            with set_env_var("AWS_WEB_IDENTITY_TOKEN_FILE", "token-file"):
                EXPECT_SUCCESS({ "s3Profile": local_aws_role_profile, "s3ConfigFile": local_aws_config_file }, all = False)

#@<> WL15885-TSFR_1_2_3_1- assume role - IMDS {MYSQLSH_AWS_ROLE is not None}
# WL15885-TSFR_6_1
imds_credentials_server = Http_test_server(ImdsCredentials)
imds_credentials_server.start()

current_log_level = shell.options["logLevel"]
shell.options["logLevel"] = 8

with set_env_var("AWS_EC2_METADATA_DISABLED", "false"):
    with set_env_var("AWS_EC2_METADATA_SERVICE_ENDPOINT", imds_credentials_server.url()):
        with write_profile(local_aws_config_file, "profile " + local_aws_role_profile, { "role_arn": MYSQLSH_AWS_ROLE, "credential_source" : "Ec2InstanceMetadata", "duration_seconds": 1000, "region": aws_settings.get("region", default_aws_region) }):
            WIPE_SHELL_LOG()
            EXPECT_SUCCESS({ "s3Profile": local_aws_role_profile, "s3ConfigFile": local_aws_config_file }, all = False)
            # WL15885-TSFR_1_7_1
            EXPECT_SHELL_LOG_CONTAINS("AWS assume role - custom duration: 1000s")
            # WL15885-TSFR_1_8_1
            EXPECT_SHELL_LOG_NOT_CONTAINS("AWS assume role - external ID:")

shell.options["logLevel"] = current_log_level

imds_credentials_server.stop()

#@<> WL15885-TSFR_1_2_2_1 - assume role - ECS {MYSQLSH_AWS_ROLE is not None}
# WL15885-TSFR_5_1
ecs_credentials_server = Http_test_server(EcsCredentials)
ecs_credentials_server.start()

current_log_level = shell.options["logLevel"]
shell.options["logLevel"] = 8

with set_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI", ecs_credentials_server.url()):
    with write_profile(local_aws_config_file, "profile " + local_aws_role_profile, { "role_arn": MYSQLSH_AWS_ROLE, "credential_source" : "EcsContainer", "external_id": "external-id", "region": aws_settings.get("region", default_aws_region) }):
        WIPE_SHELL_LOG()
        EXPECT_SUCCESS({ "s3Profile": local_aws_role_profile, "s3ConfigFile": local_aws_config_file }, all = False)
        # WL15885-TSFR_1_8_2
        EXPECT_SHELL_LOG_CONTAINS("AWS assume role - external ID: external-id")
        # WL15885-TSFR_1_7_2
        EXPECT_SHELL_LOG_NOT_CONTAINS("AWS assume role - custom duration:")

shell.options["logLevel"] = current_log_level

ecs_credentials_server.stop()

#@<> WL15885-TSFR_1_3_1_1 - assume role - source profile {MYSQLSH_AWS_ROLE is not None}
current_log_level = shell.options["logLevel"]
shell.options["logLevel"] = 8

with write_profile(local_aws_config_file, "profile " + local_aws_role_profile, { "role_arn": MYSQLSH_AWS_ROLE, "source_profile" : local_aws_profile, "role_session_name" : "test-session-name", "region": aws_settings.get("region", default_aws_region) }):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "aws_access_key_id": aws_settings["aws_access_key_id"], "aws_secret_access_key" : aws_settings["aws_secret_access_key"] }):
        WIPE_SHELL_LOG()
        EXPECT_SUCCESS({ "s3Profile": local_aws_role_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file }, all = False)
        # WL15885-TSFR_1_6_1
        EXPECT_SHELL_LOG_CONTAINS("AWS assume role - session name: test-session-name")

shell.options["logLevel"] = current_log_level

#@<> BUG#36916928 - using invalid role ARN fails without any more details
with write_profile(local_aws_config_file, "profile " + local_aws_role_profile, { "role_arn": "arn:aws:iam::1234567890:role/unknown", "source_profile" : local_aws_profile, "region": aws_settings.get("region", default_aws_region) }):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "aws_access_key_id": aws_settings["aws_access_key_id"], "aws_secret_access_key" : aws_settings["aws_secret_access_key"] }):
        EXPECT_FAIL("RuntimeError", re.compile(f"Could not assume role using profile '{local_aws_role_profile}': User: .* is not authorized to perform: sts:AssumeRole on resource: arn:aws:iam::1234567890:role/unknown"), { "s3Profile": local_aws_role_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file }, all = False)

#@<> BUG#36917083 - using invalid value of duration_seconds fails without any more details {MYSQLSH_AWS_ROLE is not None}
with write_profile(local_aws_config_file, "profile " + local_aws_role_profile, { "duration_seconds": 123, "role_arn": MYSQLSH_AWS_ROLE, "source_profile" : local_aws_profile, "region": aws_settings.get("region", default_aws_region) }):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "aws_access_key_id": aws_settings["aws_access_key_id"], "aws_secret_access_key" : aws_settings["aws_secret_access_key"] }):
        EXPECT_FAIL("RuntimeError", f"Could not assume role using profile '{local_aws_role_profile}': 1 validation error detected: Value '123' at 'durationSeconds' failed to satisfy constraint: Member must have value greater than or equal to 900", { "s3Profile": local_aws_role_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file }, all = False)

#@<> cleanup
cleanup_tests(load_tests = True)

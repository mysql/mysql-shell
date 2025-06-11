#@ {has_aws_environment()}

#@<> INCLUDE dump_utils.inc

#@<> INCLUDE aws_utils.inc

#@<> setup
setup_tests(load_tests = True)

#@<> helpers
dump_file = "dump.csv"

country_sql = """CREATE TABLE `world`.`Country` (
  `Code` char(3) NOT NULL default '',
  `Name` char(52) NOT NULL default '',
  `Continent` enum('Asia','Europe','North America','Africa','Oceania','Antarctica','South America') NOT NULL default 'Asia',
  `Region` char(26) NOT NULL default '',
  `SurfaceArea` float(10,2) NOT NULL default '0.00',
  `IndepYear` smallint(6) default NULL,
  `Population` int(11) NOT NULL default '0',
  `LifeExpectancy` float(3,1) default NULL,
  `GNP` float(10,2) default NULL,
  `GNPOld` float(10,2) default NULL,
  `LocalName` char(45) NOT NULL default '',
  `GovernmentForm` char(45) NOT NULL default '',
  `HeadOfState` char(60) default NULL,
  `Capital` int(11) default NULL,
  `Code2` char(2) NOT NULL default '',
  PRIMARY KEY  (`Code`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1"""

def EXPECT_SUCCESS(options = {}, excluded_options = []):
    aws_options = get_options(options)
    print(f"--> EXPECT_SUCCESS -> {aws_options}")
    def dump():
        print(f"--> Executing util.export_table()")
        setup_session(__sandbox_uri1)
        clean_bucket()
        EXPECT_NO_THROWS(lambda: util.export_table("world.Country", dump_file, aws_options), f"Failed to export using options: {aws_options}")
    def load():
        print("--> Executing util.import_table()")
        setup_session(__sandbox_uri2)
        wipeout_server(session)
        session.run_sql("CREATE SCHEMA `world`")
        session.run_sql(country_sql)
        opts = aws_options.copy()
        opts["schema"] = "world"
        opts["table"] = "Country"
        EXPECT_NO_THROWS(lambda: util.import_table(dump_file, opts), f"Failed to import using options: {opts}")
    dump()
    load()

def EXPECT_FAIL(error, msg, options = {}, excluded_options = [], argument_error = True):
    aws_options = get_options(options, excluded_options)
    print(f"--> EXPECT_FAIL -> {aws_options}")
    def full_msg(method, options_arg):
        is_re = is_re_instance(msg)
        m = f"{re.escape(error) if is_re else error}: {f'Argument #{options_arg}: ' if argument_error else ''}{msg.pattern if is_re else msg}"
        if is_re:
            m = re.compile("^" + m)
        return m
    def dump():
        print(f"--> Executing util.export_table()")
        setup_session(__sandbox_uri1)
        clean_bucket()
        EXPECT_THROWS(lambda: util.export_table("world.Country", dump_file, aws_options), full_msg("export_table", 3))
    def load():
        print("--> Executing util.import_table()")
        setup_session(__sandbox_uri2)
        wipeout_server(session)
        session.run_sql("CREATE SCHEMA `world`")
        session.run_sql(country_sql)
        opts = aws_options.copy()
        opts["schema"] = "world"
        opts["table"] = "Country"
        EXPECT_THROWS(lambda: util.import_table(dump_file, opts), full_msg("import_table", 2))
    dump()
    load()

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

EXPECT_TRUE(help_text in util.help("export_table"))
EXPECT_TRUE(help_text in util.help("import_table"))

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
        with write_profile(local_aws_config_file, "profile " + local_aws_profile, aws_settings):
            # empty strings cause dumper/loader to use files from the default location
            EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": "" })

#@<> WL14387-TSFR_4_1_2_1
with write_profile(default_aws_config_file, "profile " + local_aws_profile, aws_settings):
    # empty strings cause dumper/loader to use files from the default location
    EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": "", "s3CredentialsFile": "" })

#@<> WL14387-TSFR_5_1
# BUG#34604763 - partial credentials are an error
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "region": aws_settings.get("region", default_aws_region), "aws_access_key_id": aws_settings["aws_access_key_id"] }):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "aws_secret_access_key": aws_settings["aws_secret_access_key"] }):
        EXPECT_FAIL("RuntimeError", f"Partial AWS credentials found in credentials file ({local_aws_credentials_file}, profile: {local_aws_profile}), missing the value of 'aws_access_key_id'", { "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })

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
        EXPECT_FAIL("Error: Shell Error (54403)", re.compile(".* \\(403\\)"), { "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file }, argument_error = False)

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

#@<> WL14387-TSFR_ET_1
with write_profile(local_aws_config_file, "profile " + local_aws_profile, aws_settings):
    with write_profile(local_aws_credentials_file, local_aws_profile + "-invalid", {}):
        EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })

#@<> BUG#34604763 - new s3Region option
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "region": "invalid" }):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "aws_access_key_id": aws_settings["aws_access_key_id"], "aws_secret_access_key": aws_settings["aws_secret_access_key"] }):
        EXPECT_SUCCESS({ "s3Region": aws_settings.get("region", default_aws_region), "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })

#@<> BUG#34657730 - util.export_table() should output remote options needed to import this dump
# setup
tested_schema = "tested_schema"
tested_table = "tested_table"
dump_dir = "bug_34657730"

dump_session.run_sql("DROP SCHEMA IF EXISTS !;", [ tested_schema ])
dump_session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
dump_session.run_sql("CREATE TABLE !.! (id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, something BINARY)", [ tested_schema, tested_table ])
dump_session.run_sql("INSERT INTO !.! (id) VALUES (302)", [ tested_schema, tested_table ])
dump_session.run_sql("INSERT INTO !.! (something) VALUES (char(0))", [ tested_schema, tested_table ])

# prepare the dump
setup_session(__sandbox_uri1)
clean_bucket()
WIPE_OUTPUT()
util.export_table(quote_identifier(tested_schema, tested_table), dump_dir, get_options())

# capture the import command
util_import_table_code = extract_import_table_code()

#@<> BUG#34657730 - test
wipeout_server(load_session)
load_session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
load_session.run_sql("CREATE TABLE !.! (id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, something BINARY)", [ tested_schema, tested_table ])

setup_session(__sandbox_uri2)
EXPECT_NO_THROWS(lambda: exec(util_import_table_code), "importing data")
EXPECT_EQ(md5_table(dump_session, tested_schema, tested_table), md5_table(load_session, tested_schema, tested_table))

#@<> BUG#34657730 - cleanup
dump_session.run_sql("DROP SCHEMA IF EXISTS !;", [ tested_schema ])

#@<> BUG#35018278 skipRows=X should be applied even if a compressed file or multiple files are loaded
# setup
test_schema = "bug_35018278"
test_table = "t"
test_table_qualified = quote_identifier(test_schema, test_table)
test_rows = 10

def create_test_table(s):
    s.run_sql("DROP SCHEMA IF EXISTS !", [ test_schema ])
    s.run_sql("CREATE SCHEMA !", [ test_schema ])
    s.run_sql(f"CREATE TABLE {test_table_qualified} (k INT PRIMARY KEY, v TEXT)")

create_test_table(dump_session)

for i in range(test_rows):
    dump_session.run_sql(f"INSERT INTO {test_table_qualified} VALUES ({i}, REPEAT('a', 10000))")

clean_bucket()
setup_session(__sandbox_uri1)

for compression, extension in { "none": "", "zstd": ".zst" }.items():
    util.export_table(test_table_qualified, f"1.tsv{extension}", { "fieldsEnclosedBy": "'", "linesTerminatedBy": "a", "compression": compression, "where": f"k < {test_rows / 2}", "showProgress": False, **get_options() })
    util.export_table(test_table_qualified, f"2.tsv{extension}", { "fieldsEnclosedBy": "'", "linesTerminatedBy": "a", "compression": compression, "where": f"k >= {test_rows / 2}", "showProgress": False, **get_options() })

#@<> BUG#35018278 - tests
wipeout_server(load_session)
create_test_table(load_session)
setup_session(__sandbox_uri2)

for extension in [ "", ".zst" ]:
    for files in [ [ "1.tsv", "2.tsv" ], [ "*.tsv" ] ]:
        for skip in range(int(test_rows / 2) + 2):
            context = f"skip: {skip}"
            session.run_sql(f"TRUNCATE TABLE {test_table_qualified}")
            for f in files:
                EXPECT_NO_THROWS(lambda: util.import_table(f"{f}{extension}", { "skipRows": skip, "schema": test_schema, "table": test_table, "fieldsEnclosedBy": "'", "linesTerminatedBy": "a", "showProgress": False, **get_options() }), f"file: {f}{extension}, {context}")
            EXPECT_EQ(max(test_rows - 2 * skip, 0), session.run_sql(f"SELECT COUNT(*) FROM {test_table_qualified}").fetch_one()[0], f"files: {files}, extension: {extension}, {context}")

#@<> BUG#35018278 - cleanup
dump_session.run_sql("DROP SCHEMA IF EXISTS !", [ test_schema ])

#@<> cleanup
cleanup_tests(load_tests = True)

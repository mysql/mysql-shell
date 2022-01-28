#@ {has_aws_environment()}

#@<> INCLUDE aws_utils.inc

#@<> INCLUDE dump_utils.inc

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

def EXPECT_SUCCESS(options = {}):
    aws_options = get_options(options)
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

def EXPECT_FAIL(error, msg, options = {}, argument_error = True):
    aws_options = get_options(options)
    def full_msg(method, options_arg):
        is_re = is_re_instance(msg)
        m = f"{re.escape(error) if is_re else error}: Util.{method}: {f'Argument #{options_arg}: ' if argument_error else ''}{msg.pattern if is_re else msg}"
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
        credentials file instead of the one at the default location.
      - s3ConfigFile: string (default: not set) - Use the specified AWS config
        file instead of the one at the default location.
      - s3Profile: string (default: not set) - Use the specified AWS profile
        instead of the default one.
      - s3EndpointOverride: string (default: not set) - Use the specified AWS
        S3 API endpoint instead of the default one.
"""

EXPECT_TRUE(help_text in util.help("export_table"))
EXPECT_TRUE(help_text in util.help("import_table"))

#@<> WL14387-TSFR_1_1_2
EXPECT_SUCCESS()

#@<> WL14387-TSFR_2_1_2
EXPECT_FAIL("RuntimeError", f"The 'aws_access_key_id' setting for the profile 'non-existing-profile' was not found in neither '{MYSQLSH_AWS_CONFIG_FILE}' nor '{MYSQLSH_AWS_SHARED_CREDENTIALS_FILE}' files.", { "s3Profile": "non-existing-profile" })

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
        EXPECT_FAIL("RuntimeError", f"The 'aws_access_key_id' setting for the profile '{local_aws_profile}' was not found in neither '{local_aws_config_file}' nor '{local_aws_credentials_file}' files.", { "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })

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

#@<> WL14387-TSFR_5_1
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "region": aws_settings.get("region", default_aws_region), "aws_access_key_id": aws_settings["aws_access_key_id"] }):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "aws_secret_access_key": aws_settings["aws_secret_access_key"] }):
        EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })

#@<> WL14387-TSFR_5_3
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "region": aws_settings.get("region", default_aws_region), "aws_access_key_id": "invalid", "aws_secret_access_key": "invalid" }):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "aws_access_key_id": aws_settings["aws_access_key_id"], "aws_secret_access_key": aws_settings["aws_secret_access_key"] }, { "invalid-profile": { "aws_access_key_id": "invalid", "aws_secret_access_key": "invalid" } }):
        EXPECT_SUCCESS({ "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })

#@<> WL14387-TSFR_5_1_1
with write_profile(default_aws_config_file, "profile " + local_aws_profile, {}):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "aws_secret_access_key": aws_settings["aws_secret_access_key"] }):
        EXPECT_FAIL("RuntimeError", f"The 'aws_access_key_id' setting for the profile '{local_aws_profile}' was not found in neither '{default_aws_config_file}' nor '{local_aws_credentials_file}' files.", { "s3Profile": local_aws_profile, "s3ConfigFile": "", "s3CredentialsFile": local_aws_credentials_file })

#@<> WL14387-TSFR_5_2_1
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "aws_secret_access_key": "", "aws_access_key_id": "invalid" }):
    with write_profile(local_aws_credentials_file, local_aws_profile, {}):
        # aws_access_key_id is invalid, so we're expecting an 403 Forbidden error
        EXPECT_FAIL("Error: Shell Error (54403)", re.compile(".* \(403\)"), { "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file }, False)

#@<> WL14387-TSFR_5_2_2
with write_profile(local_aws_config_file, "profile " + local_aws_profile, { "region": aws_settings.get("region", default_aws_region) }):
    with write_profile(local_aws_credentials_file, local_aws_profile, { "aws_access_key_id": aws_settings["aws_access_key_id"] }):
        EXPECT_FAIL("RuntimeError", f"The 'aws_secret_access_key' setting for the profile '{local_aws_profile}' was not found in neither '{local_aws_config_file}' nor '{local_aws_credentials_file}' files.", { "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_config_file, "s3CredentialsFile": local_aws_credentials_file })

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

#@<> cleanup
cleanup_tests(load_tests = True)

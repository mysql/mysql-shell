/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/util/mod_util.h"

#include <memory>
#include <set>
#include <utility>
#include <vector>
#include "modules/mod_utils.h"
#include "modules/mysqlxtest_utils.h"
#include "modules/util/binlog/binlog_dumper.h"
#include "modules/util/binlog/binlog_loader.h"
#include "modules/util/copy/copy_operation.h"
#include "modules/util/dump/dump_instance.h"
#include "modules/util/dump/dump_instance_options.h"
#include "modules/util/dump/dump_schemas.h"
#include "modules/util/dump/dump_schemas_options.h"
#include "modules/util/dump/dump_tables.h"
#include "modules/util/dump/dump_tables_options.h"
#include "modules/util/dump/export_table.h"
#include "modules/util/dump/export_table_options.h"
#include "modules/util/import_table/import_table.h"
#include "modules/util/import_table/import_table_options.h"
#include "modules/util/json_importer.h"
#include "modules/util/load/dump_loader.h"
#include "modules/util/load/load_dump_options.h"
#include "mysqlshdk/include/scripting/shexcept.h"
#include "mysqlshdk/include/shellcore/base_session.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/atomic_flag.h"
#include "mysqlshdk/libs/utils/document_parser.h"
#include "mysqlshdk/libs/utils/log_sql.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
REGISTER_HELP_GLOBAL_OBJECT(util, shellapi);
REGISTER_HELP(UTIL_GLOBAL_BRIEF,
              "Global object that groups miscellaneous tools like upgrade "
              "checker and JSON import.");
REGISTER_HELP(UTIL_BRIEF,
              "Global object that groups miscellaneous tools like upgrade "
              "checker and JSON import.");

Util::Util(shcore::IShell_core *owner)
    : Extensible_object("util", "util", true), _shell_core(*owner) {
  expose("checkForServerUpgrade", &Util::check_for_server_upgrade,
         "?connectionData", "?options")
      ->cli();
  expose("importJson", &Util::import_json, "path", "?options")->cli();
  expose("importTable", &Util::import_table_file, "path", "?options")
      ->cli(false);
  expose("importTable", &Util::import_table_files, "files", "?options")->cli();
  expose("dumpSchemas", &Util::dump_schemas, "schemas", "outputUrl", "?options")
      ->cli();
  expose("dumpTables", &Util::dump_tables, "schema", "tables", "outputUrl",
         "?options")
      ->cli();
  expose("dumpInstance", &Util::dump_instance, "outputUrl", "?options")->cli();
  expose("exportTable", &Util::export_table, "table", "outputUrl", "?options")
      ->cli();
  expose("loadDump", &Util::load_dump, "url", "?options")->cli();
  expose("copyInstance", &Util::copy_instance, "connectionData", "?options")
      ->cli();
  expose("copySchemas", &Util::copy_schemas, "schemas", "connectionData",
         "?options")
      ->cli();
  expose("copyTables", &Util::copy_tables, "schema", "tables", "connectionData",
         "?options")
      ->cli();

  expose("dumpBinlogs", &Util::dump_binlogs, "outputUrl", "?options")->cli();
  expose("loadBinlogs", &Util::load_binlogs, "urls", "?options")->cli();
}

REGISTER_HELP_FUNCTION(checkForServerUpgrade, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_CHECKFORSERVERUPGRADE, R"*(
Performs series of tests on specified MySQL server to check if the upgrade
process will succeed.

@param connectionData Optional the connection data to server to be checked
@param options Optional dictionary of options to modify tool behaviour.

If no connectionData is specified tool will try to establish connection using
data from current session.

${TOPIC_CONNECTION_DATA}

Tool behaviour can be modified with following options:

@li <b>configPath</b> - full path to MySQL server configuration file.
@li <b>outputFormat</b> - value can be either TEXT (default) or JSON.
@li <b>targetVersion</b> - version to which upgrade will be checked.
@li <b>password</b> - password for connection.
@li <b>include</b> - comma separated list containing the check identifiers to be
included in the operation.
@li <b>exclude</b> - comma separated list containing the check identifiers to be
 excluded from the operation.
@li <b>list</b> - bool value to indicate the operation should only list the
checks.

If <b>targetVersion</b> is not specified, the current %Shell version will be
used as target version.

<b>Limitations</b>

When running this tool with a server older than 8.0, some checks have additional
requirements:

@li The checks related to system variables require the full path to the
configuration file to be provided through the <b>configPath</b> option.
@li The <b>checkTableCommand</b> check requires the user executing the tool has
the RELOAD grant.
@li The <b>schemaInconsistency</b> check ignores schemas/tables that contain
unicode characters outside ASCII range.
)*");

/**
 * \ingroup util
 * $(UTIL_CHECKFORSERVERUPGRADE_BRIEF)
 *
 * $(UTIL_CHECKFORSERVERUPGRADE)
 */
#if DOXYGEN_JS
Undefined Util::checkForServerUpgrade(ConnectionData connectionData,
                                      Dictionary options);
Undefined Util::checkForServerUpgrade(Dictionary options);
#elif DOXYGEN_PY
None Util::check_for_server_upgrade(ConnectionData connectionData,
                                    dict options);
None Util::check_for_server_upgrade(dict options);
#endif

void Util::check_for_server_upgrade(
    const std::optional<mysqlshdk::db::Connection_options> &connection_options,
    const shcore::Option_pack_ref<upgrade_checker::Upgrade_check_options>
        &options) {
  mysqlshdk::db::Connection_options connection;

  if (connection_options.has_value()) {
    connection = *connection_options;
  }

  if (connection.has_data()) {
    if (options->password.has_value()) {
      if (connection.has_password()) connection.clear_password();
      connection.set_password(*options->password);
    }
  } else {
    if (!_shell_core.get_dev_session()) {
      if (!options->list_checks) {
        throw shcore::Exception::argument_error(
            "Please connect the shell to the MySQL server to be checked or "
            "specify the server URI as a parameter.");
      }
    } else {
      connection = _shell_core.get_dev_session()->get_connection_options();
    }
  }

  upgrade_checker::Upgrade_check_config config{*options};
  std::unique_ptr<mysqlshdk::mysql::User_privileges> privileges;

  if (connection.has_data()) {
    if (connection.is_interactive()) {
      connection.set_interactive(false);
    }
    const auto session =
        establish_session(connection, current_shell_options()->get().wizards);
    mysqlshdk::mysql::Instance instance(session);

    try {
      privileges = instance.get_current_user_privileges(true);
    } catch (const std::runtime_error &e) {
      log_error("Unable to check permissions: %s", e.what());
      throw std::runtime_error(
          "The upgrade check needs to be performed by user with PROCESS, and "
          "SELECT privileges.");
    } catch (const std::logic_error &) {
      throw std::runtime_error("Unable to get information about a user");
    }

    config.set_session(session);
    config.set_user_privileges(privileges.get());
  } else {
    config.set_default_server_data();
  }

  check_for_upgrade(config);
}

REGISTER_HELP_FUNCTION(importJson, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_IMPORTJSON, R"*(
Import JSON documents from file to collection or table in MySQL Server using X
Protocol session.

@param file %Path to JSON documents file
@param options Optional dictionary with import options

This function reads standard JSON documents from a file, it also supports
converting BSON Data Types represented using the MongoDB Extended Json (strict
mode) into MySQL values.

The <b>options</b> dictionary supports the following options:
@li <b>schema</b>: string - Name of target schema.
@li <b>collection</b>: string - Name of collection where the data will be
imported.
@li <b>table</b>: string - Name of table where the data will be imported.
@li <b>tableColumn</b>: string (default: "doc") - Name of column in target
table where the imported JSON documents will be stored.
@li <b>convertBsonTypes</b>: bool (default: false) - Enables the BSON data type
conversion.
@li <b>convertBsonOid</b>: bool (default: the value of <b>convertBsonTypes</b>)
- Enables conversion of the BSON ObjectId values.
@li <b>extractOidTime</b>: string (default: empty) - Creates a new field based
on the ObjectID timestamp. Only valid if <b>convertBsonOid</b> is enabled.

The following options are valid only when <b>convertBsonTypes</b> is enabled.
These are all boolean flags. The <b>ignoreRegexOptions</b> option is enabled by
default, the rest is disabled by default.
@li <b>ignoreDate</b>: Disables conversion of BSON Date values.
@li <b>ignoreTimestamp</b>: Disables conversion of BSON Timestamp values.
@li <b>ignoreRegex</b>: Disables conversion of BSON Regex values.
@li <b>ignoreBinary</b>: Disables conversion of BSON BinData values.
@li <b>decimalAsDouble</b>: Causes BSON Decimal values to be imported as double
values.
@li <b>ignoreRegexOptions</b>: Causes regex options to be ignored when
processing a Regex BSON value. This option is only valid if <b>ignoreRegex</b>
is disabled.

If the schema is not provided, an active schema on the global session, if set,
will be used.

The <b>collection</b> and the <b>table</b> options cannot be combined. If they
are not provided, the basename of the file without extension will be used as
target collection name.

If the target collection or table does not exist, they are created, otherwise
the data is inserted into the existing collection or table.

The <b>tableColumn</b> implies the use of the <b>table</b> option and cannot be
combined with the <b>collection</b> option.

<b>BSON Data Type Processing.</b>

If only <b>convertBsonOid</b> is enabled, no conversion will be done on the rest
of the BSON Data Types.

To use <b>extractOidTime</b>, it should be set to a name which will be used to
insert an additional field into the main document. The value of the new field
will be the timestamp obtained from the ObjectID value. Note that this will be
done only for an ObjectID value associated to the '_id' field of the main
document.

NumberLong and NumberInt values will be converted to integer values.

NumberDecimal values are imported as strings, unless <b>decimalAsDouble</b> is
enabled.

Regex values will be converted to strings containing the regular expression. The
regular expression options are ignored unless <b>ignoreRegexOptions</b> is
disabled. When <b>ignoreRegexOptions</b> is disabled, the regular expression
will be converted to the form: /@<regex@>/@<options@>.
)*");

const shcore::Option_pack_def<Import_json_options>
    &Import_json_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Import_json_options>()
          .optional("schema", &Import_json_options::schema)
          .optional("collection", &Import_json_options::collection)
          .optional("table", &Import_json_options::table)
          .optional("tableColumn", &Import_json_options::table_column)
          .include(&Import_json_options::doc_reader);

  return opts;
}

/**
 * \ingroup util
 *
 * $(UTIL_IMPORTJSON_BRIEF)
 *
 * $(UTIL_IMPORTJSON)
 */
#if DOXYGEN_JS
Undefined Util::importJson(String file, Dictionary options);
#elif DOXYGEN_PY
None Util::import_json(str file, dict options);
#endif

void Util::import_json(
    const std::string &file,
    const shcore::Option_pack_ref<Import_json_options> &options) {
  auto shell_session = _shell_core.get_dev_session();

  if (!shell_session) {
    throw shcore::Exception::runtime_error(
        "Please connect the shell to the MySQL server.");
  }

  if (shell_session->session_type() != SessionType::X) {
    throw shcore::Exception::runtime_error(
        "An X Protocol session is required for JSON import.");
  }

  Connection_options connection_options =
      shell_session->get_connection_options();

  std::shared_ptr<mysqlshdk::db::mysqlx::Session> xsession =
      mysqlshdk::db::mysqlx::Session::create();

  if (current_shell_options()->get().trace_protocol) {
    xsession->enable_protocol_trace(true);
  }
  xsession->connect(connection_options);

  Prepare_json_import prepare{xsession};

  if (!options->schema.empty()) {
    prepare.schema(options->schema);
  } else if (!shell_session->get_current_schema().empty()) {
    prepare.schema(shell_session->get_current_schema());
  } else {
    throw std::runtime_error(
        "There is no active schema on the current session, the target schema "
        "for the import operation must be provided in the options.");
  }

  prepare.path(file);

  if (!options->table.empty()) {
    prepare.table(options->table);
  }

  if (!options->table_column.empty()) {
    prepare.column(options->table_column);
  }

  if (!options->collection.empty()) {
    if (!options->table_column.empty()) {
      throw std::invalid_argument(
          "tableColumn cannot be used with collection.");
    }
    prepare.collection(options->collection);
  }

  // Validate provided parameters and build Json_importer object.
  auto importer = prepare.build();

  auto console = mysqlsh::current_console();
  console->print_info(
      prepare.to_string() + " in MySQL Server at " +
      connection_options.as_uri(mysqlshdk::db::uri::formats::only_transport()) +
      "\n");

  importer.set_print_callback([](const std::string &msg) -> void {
    mysqlsh::current_console()->print(msg);
  });

  try {
    importer.load_from(options->doc_reader);
  } catch (...) {
    importer.print_stats();
    throw;
  }
  importer.print_stats();
}

REGISTER_HELP_TOPIC(Remote Storage, TOPIC, TOPIC_REMOTE_STORAGE, Contents,
                    SCRIPTING);

REGISTER_HELP_TOPIC_TEXT(TOPIC_REMOTE_STORAGE, R"*(
<b>Oracle Cloud Infrastructure (OCI) Object Storage Options</b>

${TOPIC_OCI_STORAGE_OPTIONS}

<b>Description</b>

${TOPIC_OCI_STORAGE_OPTIONS_DETAILS}

<b>OCI Object Storage Pre-Authenticated Requests (PARs)</b>

${TOPIC_OCI_STORAGE_PAR_DETAILS}

<b>AWS S3 Object Storage Options</b>

${TOPIC_AWS_STORAGE_OPTIONS}

<b>Description</b>

${TOPIC_AWS_STORAGE_OPTIONS_DETAILS}

<b>Azure Blob Storage Options</b>

${TOPIC_AZURE_STORAGE_OPTIONS}

<b>Description</b>

${TOPIC_AZURE_STORAGE_OPTIONS_DETAILS}
)*");

#ifdef DOXYGEN
REGISTER_HELP(TOPIC_REMOTE_STORAGE_MORE_INFO,
              "For additional information on remote storage support, see @ref "
              "remote_storage.");
#else
REGISTER_HELP(TOPIC_REMOTE_STORAGE_MORE_INFO,
              "For additional information on remote storage support use \\? "
              "remote storage.");
#endif

REGISTER_HELP_DETAIL_TEXT(TOPIC_AZURE_STORAGE_OPTIONS, R"*(
@li <b>azureContainerName</b>: string (default: not set) - Name of the Azure
container to use. The container must already exist.
@li <b>azureConfigFile</b>: string (default: not set) - Use the specified Azure
configuration file instead of the one at the default location.
@li <b>azureStorageAccount</b>: string (default: not set) - The account to be
used for the operation.
@li <b>azureStorageSasToken</b>: string (default: not set) - Azure Shared Access
Signature (SAS) token, to be used for the authentication of the operation,
instead of a key.)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_AZURE_STORAGE_OPTIONS_DETAILS, R"*(
If the <b>azureContainerName</b> option is used, the specified Azure container
is used as the file storage. Connection is established using the configuration
at the local Azure configuration file. The directory structure is simulated
within the blob name.

The <b>azureConfigFile</b>, <b>azureStorageAccount</b> and
<b>azureStorageSasToken</b> options cannot be used if the
<b>azureContainerName</b> option is not set or set to an empty string.

<b>Handling of the Azure settings</b>

The following settings are read from the <b>storage</b> section in the
<b>config</b> file:
@li <b>connection_string</b>
@li <b>account</b>
@li <b>key</b>
@li <b>sas_token</b>

Additionally, the connection options may be defined using the standard Azure
environment variables:
@li <b>AZURE_STORAGE_CONNECTION_STRING</b>
@li <b>AZURE_STORAGE_ACCOUNT</b>
@li <b>AZURE_STORAGE_KEY</b>
@li <b>AZURE_STORAGE_SAS_TOKEN</b>

The Azure configuration values are evaluated in the following precedence:

@li options parameter
@li environment variables
@li configuration file

If a connection string is defined either case in the environment variable or the
configuration option, the individual configuration values for account and key
will be ignored.

If a SAS Token is defined, it will be used for the authorization (ignoring any
defined account key).

The default Azure Blob Endpoint to be used in the operations is defined by:
<br>
@code
  https://<account>.blob.core.windows.net
@endcode

unless a different endpoint is defined in the connection string.)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_AWS_STORAGE_OPTIONS, R"*(
@li <b>s3BucketName</b>: string (default: not set) - Name of the AWS S3 bucket
to use. The bucket must already exist.
@li <b>s3CredentialsFile</b>: string (default: not set) - Use the specified AWS
<b>credentials</b> file.
@li <b>s3ConfigFile</b>: string (default: not set) - Use the specified AWS
<b>config</b> file.
@li <b>s3Profile</b>: string (default: not set) - Use the specified AWS profile.
@li <b>s3Region</b>: string (default: not set) - Use the specified AWS region.
@li <b>s3EndpointOverride</b>: string (default: not set) - Use the specified AWS
S3 API endpoint instead of the default one.)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_AWS_STORAGE_OPTIONS_DETAILS, R"*(
If the <b>s3BucketName</b> option is used, the specified AWS S3 bucket is used
as the file storage. Connection is established using default local AWS
configuration paths and profiles, unless overridden. The directory structure is
simulated within the object name.

The <b>s3CredentialsFile</b>, <b>s3ConfigFile</b>, <b>s3Profile</b>,
<b>s3Region</b> and <b>s3EndpointOverride</b> options cannot be used if the
<b>s3BucketName</b> option is not set or set to an empty string.

All failed connections to AWS S3 are retried three times, with a 1 second delay
between retries. If a failure occurs 10 minutes after the connection was
created, the delay is changed to an exponential back-off strategy:
@li first delay: 3-6 seconds
@li second delay: 18-36 seconds
@li third delay: 40-80 seconds

<b>Handling of the AWS settings</b>

The AWS options are evaluated in the order of precedence, the first available
value is used.

-# Name of the AWS profile:
@li the <b>s3Profile</b> option
@li the <b>AWS_PROFILE</b> environment variable
@li the <b>AWS_DEFAULT_PROFILE</b> environment variable
@li the default value of <b>default</b>
-# Location of the <b>credentials</b> file:
@li the <b>s3CredentialsFile</b> option
@li the <b>AWS_SHARED_CREDENTIALS_FILE</b> environment variable
@li the default value of <b>~/.aws/credentials</b>
-# Location of the <b>config</b> file:
@li the <b>s3ConfigFile</b> option
@li the <b>AWS_CONFIG_FILE</b> environment variable
@li the default value of <b>~/.aws/config</b>
-# Name of the AWS region:
@li the <b>s3Region</b> option
@li the <b>AWS_REGION</b> environment variable
@li the <b>AWS_DEFAULT_REGION</b> environment variable
@li the <b>region</b> setting from the <b>config</b> file for the specified profile
@li the default value of <b>us-east-1</b>
-# URI of AWS S3 API endpoint
@li the <b>s3EndpointOverride</b> option
@li the default value of <b>%https://@<s3BucketName@>.s3.@<region@>.amazonaws.com</b>

.
The AWS credentials are fetched from the following providers, in the order of
precedence:

-# Environment variables:
@li <b>AWS_ACCESS_KEY_ID</b>
@li <b>AWS_SECRET_ACCESS_KEY</b>
@li <b>AWS_SESSION_TOKEN</b>
-# Assuming a role
-# Settings from the <b>credentials</b> file for the specified profile:
@li <b>aws_access_key_id</b>
@li <b>aws_secret_access_key</b>
@li <b>aws_session_token</b>
-# Process specified by the <b>credential_process</b> setting from the
<b>config</b> file for the specified profile
-# Settings from the <b>config</b> file for the specified profile:
@li <b>aws_access_key_id</b>
@li <b>aws_secret_access_key</b>
@li <b>aws_session_token</b>
-# Amazon Elastic Container Service (Amazon ECS) credentials
-# Amazon Instance Metadata Service (Amazon IMDS) credentials

The items specified above correspond to the following credentials:
@li the AWS access key
@li the secret key associated with the AWS access key
@li the AWS session token for the temporary security credentials

Role is assumed using the following settings from the AWS <b>config</b> file:

@li <b>credential_source</b>
@li <b>duration_seconds</b>
@li <b>external_id</b>
@li <b>role_arn</b>
@li <b>role_session_name</b>
@li <b>source_profile</b>

The multi-factor authentication is not supported.
For more information on assuming a role, please consult the AWS documentation.

The process/command line specified by the <b>credential_process</b> setting must
write a JSON object to the standard output in the following form:

@code
{
  "Version": 1,
  "AccessKeyId": "AWS access key",
  "SecretAccessKey": "secret key associated with the AWS access key",
  "SessionToken": "temporary AWS session token, optional",
  "Expiration": "RFC3339 timestamp, optional"
}
@endcode

The Amazon ECS credentials are fetched from a URI specified by an environment
variable <b>AWS_CONTAINER_CREDENTIALS_RELATIVE_URI</b> (its value is appended to
'%http://169.254.170.2'). If this environment variable is not set, the value of
<b>AWS_CONTAINER_CREDENTIALS_FULL_URI</b> environment variable is used instead.
If neither of these environment variables are set, ECS credentials are not used.

The request may optionally be sent with an 'Authorization' header. If the
<b>AWS_CONTAINER_AUTHORIZATION_TOKEN_FILE</b> environment variable is set, its
value should specify an absolute file path to a file that contains the
authorization token. Alternatively, the <b>AWS_CONTAINER_AUTHORIZATION_TOKEN</b>
environment variable should be used to explicilty specify that authorization
token. If neither of these environment variables are set, the 'Authorization'
header is not sent with the request.

The reply is expected to be a JSON object in the following form:

@code
{
  "AccessKeyId": "AWS access key",
  "SecretAccessKey": "secret key associated with the AWS access key",
  "Token": "temporary AWS session token",
  "Expiration": "RFC3339 timestamp"
}
@endcode

The Amazon IMDS credential provider is configured using the following
environment variables:
@li <b>AWS_EC2_METADATA_DISABLED</b>
@li <b>AWS_EC2_METADATA_SERVICE_ENDPOINT</b>
@li <b>AWS_EC2_METADATA_SERVICE_ENDPOINT_MODE</b>
@li <b>AWS_EC2_METADATA_V1_DISABLED</b>
@li <b>AWS_METADATA_SERVICE_TIMEOUT</b>
@li <b>AWS_METADATA_SERVICE_NUM_ATTEMPTS</b>

and the following settings from the AWS <b>config</b> file:

@li <b>ec2_metadata_service_endpoint</b>
@li <b>ec2_metadata_service_endpoint_mode</b>
@li <b>ec2_metadata_v1_disabled</b>
@li <b>metadata_service_timeout</b>
@li <b>metadata_service_num_attempts</b>

For more information on IMDS, please consult the AWS documentation.

The <b>Expiration</b> value, if given, specifies when the credentials are going
to expire, they will be automatically refreshed before this happens.

The following credential handling rules apply:
@li If the <b>s3Profile</b> option is set to a non-empty string, the environment
variables are not used as a potential credential provider.
@li If either an access key or a secret key is available in a potential
credential provider, it is selected as the credential provider.
@li If either the access key or the secret key is missing in the selected
credential provider, an exception is thrown.
@li If the session token is missing in the selected credential provider, or if
it is set to an empty string, it is not used to authenticate the user.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_OCI_STORAGE_OPTIONS, R"*(
@li <b>osBucketName</b>: string (default: not set) - Name of the OCI Object
Storage bucket to use. The bucket must already exist.
@li <b>osNamespace</b>: string (default: not set) - Specifies the namespace
where the bucket is located, if not given it will be obtained using the tenancy
ID on the OCI configuration.
@li <b>ociConfigFile</b>: string (default: not set) - Use the specified OCI
configuration file instead of the one at the default location.
@li <b>ociProfile</b>: string (default: not set) - Use the specified OCI profile
instead of the default one.
@li <b>ociAuth</b>: string (default: not set) - Use the specified authentication
method when connecting to the OCI. Allowed values: <b>api_key</b> (used when not
explicitly set), <b>instance_principal</b>, <b>resource_principal</b>,
<b>security_token</b>.)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_OCI_STORAGE_OPTIONS_DETAILS, R"*(
If the <b>osBucketName</b> option is used, the specified OCI bucket is used as
the file storage. Connection is established using the local OCI configuration
file. The directory structure is simulated within the object name.

The <b>osNamespace</b>, <b>ociConfigFile</b>, <b>ociProfile</b> and
<b>ociAuth</b> options cannot be used if the <b>osBucketName</b> option is not
set or set to  an empty string.

The <b>osNamespace</b> option overrides the OCI namespace obtained based on the
tenancy ID from the local OCI profile.

The <b>ociAuth</b> option allows to specify the authentication method used when
connecting to the OCI:

@li <b>api_key</b> - API Key-Based Authentication
@li <b>instance_principal</b> - Instance Principal Authentication
@li <b>resource_principal</b> - Resource Principal Authentication
@li <b>security_token</b> - Session Token-Based Authentication

For more information please see: https://docs.oracle.com/en-us/iaas/Content/API/Concepts/sdk_authentication_methods.htm
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_OCI_STORAGE_PAR_DETAILS, R"*(
When using a PAR to perform an operation, the OCI configuration is not needed to
execute it (the <b>osBucketName</b> option should not be set).

When using PAR to a specific file, the generated PAR URL should be used as
argument for an operation. The following is a file PAR to export a table to
'tab.tsv' file at the 'dump' directory of the 'test' bucket:
<br>
@code
  https://*.objectstorage.*.oci.customer-oci.com/p/*/n/*/b/test/o/dump/tab.tsv
@endcode

When using a bucket PAR, the generated PAR URL should be used as argument for an
operation. The following is a bucket PAR to create a dump at the root directory
of the 'test' bucket:
<br>
@code
  https://*.objectstorage.*.oci.customer-oci.com/p/*/n/*/b/test/o/
@endcode

When using a prefix PAR, argument should contain the PAR URL itself and the
prefix used to generate it. The following is a prefix PAR to create a dump at
the 'dump' directory of the 'test' bucket. The PAR was created using 'dump' as
prefix:
<br>
@code
  https://*.objectstorage.*.oci.customer-oci.com/p/*/n/*/b/test/o/dump/
@endcode

Note that the prefix PAR URL must end with a slash, because otherwise it will be
treated as a PAR to specific file.

<b>Reading from a PAR to specific file</b>

The following access type is required to read from such PAR:

@li Permit object reads

<b>Reading from a bucket or a prefix PAR</b>

The following access types are required to read from such PAR:

@li Permit object reads
@li Enable object listing

<b>Writing to a PAR to specific file</b>

The following access type is required to write to such PAR:

@li Permit object writes

<b>Writing to a bucket or a prefix PAR</b>

The following access types are required to write to such PAR:

@li Permit object reads and writes
@li Enable object listing

Note that read access is required in this case, because otherwise it is not
possible to list the objects.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_REMOTE_STORAGE_OPTIONS, R"*(
${TOPIC_OCI_STORAGE_OPTIONS}

${TOPIC_AWS_STORAGE_OPTIONS}

${TOPIC_AZURE_STORAGE_OPTIONS}
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_REMOTE_STORAGE_COMMON_PATHS, R"*(
@li <b>[%file://]/local/path</b> - local file storage (default)
@li <b>oci/bucket/path</b> - OCI Object Storage, when the <b>osBucketName</b>
option is given
@li <b>aws/bucket/path</b> - AWS S3 Object Storage, when the <b>s3BucketName</b>
option is given
@li <b>azure/container/path</b> - Azure Blob Storage, when the
<b>azureContainerName</b> option is given)*");

REGISTER_HELP_FUNCTION(importTable, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_IMPORTTABLE, R"*(
Import table dump stored in files to target table using LOAD DATA LOCAL
INFILE calls in parallel connections.

@param urls URL or list of URLs to files with user data.
URL can contain a glob pattern with wildcard '*' and/or '?'.
All selected files must be chunks of the same target table.
@param options Optional dictionary with import options

The <b>urls</b> parameter is a string or list of strings which specifies the
files to be imported. Allowed values:

${TOPIC_REMOTE_STORAGE_COMMON_PATHS}
@li <b>OCI Pre-Authenticated Request to an object</b> - imports a specific file
@li <b>OCI Pre-Authenticated Request to a bucket or a prefix</b> - imports files
matching the glob pattern

${TOPIC_REMOTE_STORAGE_MORE_INFO}

%Options dictionary:
@li <b>schema</b>: string (default: current %Shell active schema) - Name of
target schema.
@li <b>table</b>: string (default: filename without extension) - Name of target
table.
@li <b>columns</b>: array of strings and/or integers (default: empty array) -
This option takes an array of column names as its value. The order of the column
names indicates how to match data file columns with table columns.
Use non-negative integer `i` to capture column value into user variable @@i.
With user variables, the decodeColumns option enables you to perform preprocessing
transformations on their values before assigning the result to columns.
@li <b>fieldsTerminatedBy</b>: string (default: "\t") - This option has the same
meaning as the corresponding clause for LOAD DATA INFILE.
@li <b>fieldsEnclosedBy</b>: char (default: '') - This option has the same meaning
as the corresponding clause for LOAD DATA INFILE.
@li <b>fieldsEscapedBy</b>: char (default: '\\') - This option has the same meaning
as the corresponding clause for LOAD DATA INFILE.
@li <b>fieldsOptionallyEnclosed</b>: bool (default: false) - Set to true if the
input values are not necessarily enclosed within quotation marks specified by
<b>fieldsEnclosedBy</b> option. Set to false if all fields are quoted by
character specified by <b>fieldsEnclosedBy</b> option.
@li <b>linesTerminatedBy</b>: string (default: "\n") - This option has the same
meaning as the corresponding clause for LOAD DATA INFILE. For example, to import
Windows files that have lines terminated with carriage return/linefeed pairs,
use --lines-terminated-by="\r\n". (You might have to double the backslashes,
depending on the escaping conventions of your command interpreter.)
See Section 13.2.7, "LOAD DATA INFILE Syntax".
@li <b>replaceDuplicates</b>: bool (default: false) - If true, input rows that
have the same value for a primary key or unique index as an existing row will be
replaced, otherwise input rows will be skipped.
@li <b>threads</b>: int (default: 8) - Use N threads to sent file chunks to the
server.
@li <b>bytesPerChunk</b>: string (minimum: "131072", default: "50M") - Send
bytesPerChunk (+ bytes to end of the row) in single LOAD DATA call. Unit
suffixes, k - for Kilobytes (n * 1'000 bytes), M - for Megabytes (n * 1'000'000
bytes), G - for Gigabytes (n * 1'000'000'000 bytes), bytesPerChunk="2k" - ~2
kilobyte data chunk will send to the MySQL Server. Not available for multiple
files import.
@li <b>maxBytesPerTransaction</b>: string (default: empty) - Specifies the
maximum number of bytes that can be loaded from a dump data file per single
LOAD DATA statement. If a content size of data file is bigger than this option
value, then multiple LOAD DATA statements will be executed per single file.
If this option is not specified explicitly, dump data file sub-chunking will be
disabled. Use this option with value less or equal to global variable
'max_binlog_cache_size' to mitigate "MySQL Error 1197 (HY000): Multi-statement
transaction required more than 'max_binlog_cache_size' bytes of storage".
Unit suffixes: k (Kilobytes), M (Megabytes), G (Gigabytes).
Minimum value: 4096.
@li <b>maxRate</b>: string (default: "0") - Limit data send throughput to
maxRate in bytes per second per thread.
maxRate="0" - no limit. Unit suffixes, k - for Kilobytes (n * 1'000 bytes),
M - for Megabytes (n * 1'000'000 bytes), G - for Gigabytes (n * 1'000'000'000
bytes), maxRate="2k" - limit to 2 kilobytes per second.
@li <b>showProgress</b>: bool (default: true if stdout is a tty, false
otherwise) - Enable or disable import progress information.
@li <b>skipRows</b>: int (default: 0) - Skip first N physical lines from each of
the imported files. You can use this option to skip an initial header line
containing column names.
@li <b>dialect</b>: enum (default: "default") - Setup fields and lines options
that matches specific data file format. Can be used as base dialect and
customized with fieldsTerminatedBy, fieldsEnclosedBy, fieldsOptionallyEnclosed,
fieldsEscapedBy and linesTerminatedBy options. Must be one of the following
values: default, csv, tsv, json or csv-unix.
@li <b>decodeColumns</b>: map (default: not set) - A map between columns names
and SQL expressions to be applied on the loaded data. Column value captured in
'columns' by integer is available as user variable '@@i', where `i` is that
integer. Requires 'columns' to be set.
@li <b>characterSet</b>: string (default: not set) - Interpret the information
in the input file using this character set encoding. characterSet set to
"binary" specifies "no conversion". If not set, the server will use the
character set indicated by the character_set_database system variable to
interpret the information in the file.
@li <b>sessionInitSql</b>: list of strings (default: []) - Execute the given
list of SQL statements in each session about to load data.

${TOPIC_REMOTE_STORAGE_OPTIONS}

<b>dialect</b> predefines following set of options fieldsTerminatedBy (FT),
fieldsEnclosedBy (FE), fieldsOptionallyEnclosed (FOE), fieldsEscapedBy (FESC)
and linesTerminatedBy (LT) in following manner:
@li default: no quoting, tab-separated, lf line endings.
(LT=@<LF@>, FESC='\', FT=@<TAB@>, FE=@<empty@>, FOE=false)
@li csv: optionally quoted, comma-separated, crlf line endings.
(LT=@<CR@>@<LF@>, FESC='\', FT=",", FE='&quot;', FOE=true)
@li tsv: optionally quoted, tab-separated, crlf line endings.
(LT=@<CR@>@<LF@>, FESC='\', FT=@<TAB@>, FE='&quot;', FOE=true)
@li json: one JSON document per line.
(LT=@<LF@>, FESC=@<empty@>, FT=@<LF@>, FE=@<empty@>, FOE=false)
@li csv-unix: fully quoted, comma-separated, lf line endings.
(LT=@<LF@>, FESC='\', FT=",", FE='&quot;', FOE=false)

If the <b>schema</b> is not provided, an active schema on the global session, if
set, will be used.

If the input values are not necessarily enclosed within <b>fieldsEnclosedBy</b>,
set <b>fieldsOptionallyEnclosed</b> to true.

If you specify one separator that is the same as or a prefix of another, LOAD
DATA INFILE cannot interpret the input properly.

Connection options set in the global session, such as compression, ssl-mode, etc.
are used in parallel connections.

Each parallel connection sets the following session variables:
@li SET SQL_MODE = ''; -- Clear SQL Mode
@li SET NAMES ?; -- Set to characterSet option if provided by user.
@li SET unique_checks = 0
@li SET foreign_key_checks = 0
@li SET SESSION TRANSACTION ISOLATION LEVEL READ UNCOMMITTED

Note: because of storage engine limitations, table locks held by MyISAM will
cause imports of such tables to be sequential, regardless of the number of
threads used.
)*");
/**
 * \ingroup util
 *
 * $(UTIL_IMPORTTABLE_BRIEF)
 *
 * $(UTIL_IMPORTTABLE)
 *
 * Example input data for dialects:
 * @li default:
 * @code{.unparsed}
 * 1<TAB>20.1000<TAB>foo said: "Where is my bar?"<LF>
 * 2<TAB>-12.5000<TAB>baz said: "Where is my \<TAB> char?"<LF>
 * @endcode
 * @li csv:
 * @code{.unparsed}
 * 1,20.1000,"foo said: \"Where is my bar?\""<CR><LF>
 * 2,-12.5000,"baz said: \"Where is my <TAB> char?\""<CR><LF>
 * @endcode
 * @li tsv:
 * @code{.unparsed}
 * 1<TAB>20.1000<TAB>"foo said: \"Where is my bar?\""<CR><LF>
 * 2<TAB>-12.5000<TAB>"baz said: \"Where is my <TAB> char?\""<CR><LF>
 * @endcode
 * @li json:
 * @code{.unparsed}
 * {"id_int": 1, "value_float": 20.1000, "text_text": "foo said: \"Where is my
 * bar?\""}<LF>
 * {"id_int": 2, "value_float": -12.5000, "text_text": "baz said: \"Where is my
 * \u000b char?\""}<LF>
 * @endcode
 * @li csv-unix:
 * @code{.unparsed}
 * "1","20.1000","foo said: \"Where is my bar?\""<LF>
 * "2","-12.5000","baz said: \"Where is my <TAB> char?\""<LF>
 * @endcode
 *
 * Examples of <b>decodeColumns</b> usage:
 * @li Preprocess column2:
 * @code
 *     util.importTable('file.txt', {
 *       table: 't1',
 *       columns: ['column1', 1],
 *       decodeColumns: {'column2': '@1 / 100'}
 *     });
 * @endcode
 * is equivalent to:
 * @code
 *     LOAD DATA LOCAL INFILE 'file.txt'
 *     INTO TABLE `t1` (column1, @var1)
 *     SET `column2` = @var/100;
 * @endcode
 *
 * @li Skip columns:
 * @code
 *     util.importTable('file.txt', {
 *       table: 't1',
 *       columns: ['column1', 1, 'column2', 2, 'column3']
 *     });
 * @endcode
 * is equivalent to:
 * @code
 *     LOAD DATA LOCAL INFILE 'file.txt'
 *     INTO TABLE `t1` (column1, @1, column2, @2, column3);
 * @endcode
 *
 * @li Generate values for columns:
 * @code
 *     util.importTable('file.txt', {
 *       table: 't1',
 *       columns: [1, 2],
 *       decodeColumns: {
 *         'a': '@1',
 *         'b': '@2',
 *         'sum': '@1 + @2',
 *         'mul': '@1 * @2',
 *         'pow': 'POW(@1, @2)'
 *       }
 *     });
 * @endcode
 * is equivalent to:
 * @code
 *     LOAD DATA LOCAL INFILE 'file.txt'
 *     INTO TABLE `t1` (@1, @2)
 *     SET
 *       `a` = @1,
 *       `b` = @2,
 *       `sum` = @1 + @2,
 *       `mul` = @1 * @2,
 *       `pow` = POW(@1, @2);
 * @endcode
 */
#if DOXYGEN_JS
Undefined Util::importTable(List urls, Dictionary options);
#elif DOXYGEN_PY
None Util::import_table(list urls, dict options);
#endif
void Util::import_table_file(
    const std::string &filename,
    const shcore::Option_pack_ref<import_table::Import_table_option_pack>
        &options) {
  import_table_files({filename}, options);
}

void Util::import_table_files(
    const std::vector<std::string> &files,
    const shcore::Option_pack_ref<import_table::Import_table_option_pack>
        &options) {
  using import_table::Import_table;
  using mysqlshdk::utils::format_bytes;

  import_table::Import_table_options opt(*options);

  opt.set_filenames(files);
  opt.set_session(global_session());

  opt.validate_and_configure();

  shcore::atomic_flag interrupt;
  shcore::Interrupt_handler intr_handler(
      [&interrupt]() {
        interrupt.test_and_set();
        return false;
      },
      []() {
        mysqlsh::current_console()->print_note(
            "Interrupted by user. Cancelling...");
      });

  Import_table importer(opt);
  importer.setup_interrupt(&interrupt);

  const auto console = mysqlsh::current_console();
  console->print_info(opt.target_import_info());

  importer.import();

  if (!importer.interrupted()) {
    console->print_info(importer.import_summary());
  }

  console->print_info(importer.rows_affected_info());

  if (interrupt.test()) {
    throw shcore::cancelled("Interrupted by user");
  }

  importer.rethrow_exceptions();
}

namespace {

std::shared_ptr<shcore::Log_sql> log_sql_for_dump_and_load() {
  // copy the storage
  auto storage = current_shell_options()->get();

  // log all errors
  if (shcore::Log_sql::parse_log_level(storage.log_sql) <
      shcore::Log_sql::Log_level::ERROR) {
    storage.log_sql = "error";
  }

  return std::make_shared<shcore::Log_sql>(storage);
}

}  // namespace

REGISTER_HELP_FUNCTION(loadDump, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_LOADDUMP, R"*(
Loads database dumps created by MySQL %Shell.

@param url defines the location of the dump to be loaded
@param options Optional dictionary with load options

The <b>url</b> parameter identifies the location of the dump to be loaded.
Allowed values:

${TOPIC_REMOTE_STORAGE_COMMON_PATHS}
@li <b>OCI Pre-Authenticated Request to a bucket or a prefix</b> - loads a dump
from OCI Object Storage using a single PAR

${TOPIC_REMOTE_STORAGE_MORE_INFO}

<<<loadDump>>>() will load a dump from the specified path. It transparently
handles compressed files and directly streams data when loading from remote
storage. If the 'waitDumpTimeout' option is set, it will load a dump on-the-fly,
loading table data chunks as the dumper produces them.

Table data will be loaded in parallel using the configured number of threads
(4 by default). Multiple threads per table can be used if the dump was created
with table chunking enabled. Data loads are scheduled across threads in a way
that tries to maximize parallelism, while also minimizing lock contention from
concurrent loads to the same table. If there are more tables than threads,
different tables will be loaded per thread, larger tables first. If there are
more threads than tables, then chunks from larger tables will be proportionally
assigned more threads.

LOAD DATA LOCAL INFILE is used to load table data and thus, the 'local_infile'
MySQL global setting must be enabled.

If target MySQL server supports BULK LOAD, the load operation of compatible
tables can be offloaded to the target server, which parallelizes and loads data
directly from the Cloud storage.

<b>Resuming</b>

The load command will store progress information into a file for each step of
the loading process, including successfully completed and interrupted/failed
ones. If that file already exists, its contents will be used to skip steps that
have already been completed and retry those that failed or didn't start yet.

When resuming, table chunks that have started being loaded but didn't finish are
loaded again. Duplicate rows are discarded by the server. Tables that do not
have unique keys are truncated before the load is resumed.

IMPORTANT: Resuming assumes that no changes have been made to the partially
loaded data between the failure and the retry. Resuming after external changes
has undefined behavior and may lead to data loss.

The progress state file has a default name of load-progress.@<server_uuid@>.json
and is written to the same location as the dump. If 'progressFile' is specified,
progress will be written to either a local file at the given path, or, if the
HTTP(S) scheme is used, to a remote file using HTTP PUT requests. Setting it to
an empty string will disable progress tracking and resuming.

If the 'resetProgress' option is enabled, progress information from previous
load attempts of the dump to the destination server is discarded and the load
is restarted. You may use this option to retry loading the whole dump from the
beginning. However, changes made to the database are not reverted, so previously
loaded objects should be manually dropped first.

%Options dictionary:

@li <b>analyzeTables</b>: "off", "on", "histogram" (default: off) - If 'on',
executes ANALYZE TABLE for all tables, once loaded. If set to 'histogram', only
tables that have histogram information stored in the dump will be analyzed. This
option can be used even if all 'load' options are disabled.
@li <b>backgroundThreads</b>: int (default not set) - Number of additional
threads to use to fetch contents of metadata and DDL files. If not set, loader
will use the value of the <b>threads</b> option in case of a local dump, or four
times that value in case on a non-local dump.
@li <b>characterSet</b>: string (default taken from dump) - Overrides
the character set to be used for loading dump data. By default, the same
character set used for dumping will be used (utf8mb4 if not set on dump).
@li <b>checksum</b>: bool (default: false) - Verify tables against checksums
that were computed during dump.
@li <b>createInvisiblePKs</b>: bool (default taken from dump) - Automatically
create an invisible Primary Key for each table which does not have one. By
default, set to true if dump was created with <b>create_invisible_pks</b>
compatibility option, false otherwise. Requires server 8.0.24 or newer.
@li <b>deferTableIndexes</b>: "off", "fulltext", "all" (default: fulltext) -
If "all", creation of "all" indexes except PRIMARY is deferred until after
table data is loaded, which in many cases can reduce load times. If "fulltext",
only full-text indexes will be deferred.
@li <b>disableBulkLoad</b>: bool (default: false) - Do not use BULK LOAD
feature to load the data, even when available.
@li <b>dropExistingObjects</b>: bool (default false) - Load the dump even if
it contains user accounts or DDL objects that already exist in the target
database. If this option is set to false, any existing object results in an
error. Setting it to true drops existing user accounts and objects before
creating them. Schemas are not dropped. Mutually exclusive with
<b>ignoreExistingObjects</b>.
@li <b>dryRun</b>: bool (default: false) - Scans the dump and prints everything
that would be performed, without actually doing so.
@li <b>excludeEvents</b>: array of strings (default not set) - Skip loading
specified events from the dump. Strings are in format <b>schema</b>.<b>event</b>,
quoted using backtick characters when required.
@li <b>excludeRoutines</b>: array of strings (default not set) - Skip loading
specified routines from the dump. Strings are in format <b>schema</b>.<b>routine</b>,
quoted using backtick characters when required.
@li <b>excludeSchemas</b>: array of strings (default not set) - Skip loading
specified schemas from the dump.
@li <b>excludeTables</b>: array of strings (default not set) - Skip loading
specified tables from the dump. Strings are in format <b>schema</b>.<b>table</b>,
quoted using backtick characters when required.
@li <b>excludeTriggers</b>: array of strings (default not set) - Skip loading
specified triggers from the dump. Strings are in format <b>schema</b>.<b>table</b>
(all triggers from the specified table) or <b>schema</b>.<b>table</b>.<b>trigger</b>
(the individual trigger), quoted using backtick characters when required.
@li <b>excludeUsers</b>: array of strings (default not set) - Skip loading
specified users from the dump. Each user is in the format of
'user_name'[@'host']. If the host is not specified, all the accounts with the
given user name are excluded.
@li <b>handleGrantErrors</b>: "abort", "drop_account", "ignore" (default: abort)
- Specifies action to be performed in case of errors related to the GRANT/REVOKE
statements, "abort": throws an error and aborts the load, "drop_account":
deletes the problematic account and continues, "ignore": ignores the error and
continues loading the account.
@li <b>ignoreExistingObjects</b>: bool (default false) - Load the dump even if
it contains user accounts or DDL objects that already exist in the target
database. If this option is set to false, any existing object results in an
error. Setting it to true ignores existing objects, but the CREATE statements
are still going to be executed, except for the tables and views. Mutually
exclusive with <b>dropExistingObjects</b>.
@li <b>ignoreVersion</b>: bool (default false) - Load the dump even if the
major version number of the server where it was created is different from where
it will be loaded.
@li <b>includeEvents</b>: array of strings (default not set) - Loads only the
specified events from the dump. Strings are in format <b>schema</b>.<b>event</b>,
quoted using backtick characters when required. By default, all events are
included.
@li <b>includeRoutines</b>: array of strings (default not set) - Loads only the
specified routines from the dump. Strings are in format <b>schema</b>.<b>routine</b>,
quoted using backtick characters when required. By default, all routines are
included.
@li <b>includeSchemas</b>: array of strings (default not set) - Loads only the
specified schemas from the dump. By default, all schemas are included.
@li <b>includeTables</b>: array of strings (default not set) - Loads only the
specified tables from the dump. Strings are in format <b>schema</b>.<b>table</b>,
quoted using backtick characters when required. By default, all tables from all
schemas are included.
@li <b>includeTriggers</b>: array of strings (default not set) - Loads only the
specified triggers from the dump. Strings are in format <b>schema</b>.<b>table</b>
(all triggers from the specified table) or <b>schema</b>.<b>table</b>.<b>trigger</b>
(the individual trigger), quoted using backtick characters when required. By
default, all triggers are included.
@li <b>includeUsers</b>: array of strings (default not set) - Load only the
specified users from the dump. Each user is in the format of
'user_name'[@'host']. If the host is not specified, all the accounts with the
given user name are included. By default, all users are included.
@li <b>loadData</b>: bool (default: true) - Loads table data from the dump.
@li <b>loadDdl</b>: bool (default: true) - Executes DDL/SQL scripts in the
dump.
@li <b>loadIndexes</b>: bool (default: true) - use together with
<b>deferTableIndexes</b> to control whether secondary indexes should be
recreated at the end of the load. Useful when loading DDL and data separately.
@li <b>loadUsers</b>: bool (default: false) - Executes SQL scripts for user
accounts, roles and grants contained in the dump. Note: statements for the
current user will be skipped.
@li <b>maxBytesPerTransaction</b>: string (default taken from dump) - Specifies
the maximum number of bytes that can be loaded from a dump data file per single
LOAD DATA statement. Supports unit suffixes: k (kilobytes), M (Megabytes), G
(Gigabytes). Minimum value: 4096. If this option is not specified explicitly,
the value of the <b>bytesPerChunk</b> dump option is used, but only in case of
the files with data size greater than <b>1.5 * bytesPerChunk</b>. Not used if
table is BULK LOADED.
@li <b>progressFile</b>: path (default: load-progress.@<server_uuid@>.progress)
- Stores load progress information in the given local file path.
@li <b>resetProgress</b>: bool (default: false) - Discards progress information
of previous load attempts to the destination server and loads the whole dump
again.
@li <b>schema</b>: string (default not set) - Load the dump into the given
schema. This option can only be used when loading just one schema, (either only
one schema was dumped, or schema filters result in only one schema).
@li <b>sessionInitSql</b>: list of strings (default: []) - execute the given
list of SQL statements in each session about to load data.
@li <b>showMetadata</b>: bool (default: false) - Displays the metadata
information stored in the dump files, i.e. binary log file name and position.
@li <b>showProgress</b>: bool (default: true if stdout is a tty, false
otherwise) - Enable or disable import progress information.
@li <b>skipBinlog</b>: bool (default: false) - Disables the binary log
for the MySQL sessions used by the loader (set sql_log_bin=0).
@li <b>threads</b>: int (default: 4) - Number of threads to use to import table
data.
@li <b>updateGtidSet</b>: "off", "replace", "append" (default: off) - if set to
a value other than 'off' updates GTID_PURGED by either replacing its contents
or appending to it the gtid set present in the dump.
@li <b>waitDumpTimeout</b>: float (default: 0) - Loads a dump while it's still
being created. Once all uploaded tables are processed the command will either
wait for more data, the dump is marked as completed or the given timeout (in
seconds) passes.
<= 0 disables waiting.

${TOPIC_REMOTE_STORAGE_OPTIONS}

Connection options set in the global session, such as compression, ssl-mode, etc.
are inherited by load sessions.

Examples:
<br>
@code
util.<<<loadDump>>>('sakila_dump')

util.<<<loadDump>>>('mysql/sales', {
    'osBucketName': 'mybucket',    // OCI Object Storage bucket
    'waitDumpTimeout': 1800        // wait for new data for up to 30mins
})
@endcode


<b>Loading a dump using Pre-authenticated Requests (PAR)</b>

When a dump is created in OCI Object Storage, it is possible to load it using a
single pre-authenticated request which gives access to the location of the dump.
The requirements for this PAR include:

@li Permits object reads
@li Enables object listing

Given a dump located at a bucket root and a PAR created for the bucket, the
dump can be loaded by providing the PAR as the URL parameter:

Example:
<br>
@code
Dump Location: root of 'test' bucket

uri = 'https://*.objectstorage.*.oci.customer-oci.com/p/*/n/*/b/test/o/'

util.<<<loadDump>>>(uri, { 'progressFile': 'load_progress.txt' })
@endcode

Given a dump located at some directory within a bucket and a PAR created for the
given directory, the dump can be loaded by providing the PAR and the prefix as
the URL parameter:

Example:
<br>
@code
Dump Location: directory 'dump' at the 'test' bucket
PAR created using the 'dump/' prefix.

uri = 'https://*.objectstorage.*.oci.customer-oci.com/p/*/n/*/b/test/o/dump/'

util.<<<loadDump>>>(uri, { 'progressFile': 'load_progress.txt' })
@endcode

In both of the above cases the load is done using pure HTTP GET requests and the
<b>progressFile</b> option is mandatory.
)*");
/**
 * \ingroup util
 *
 * $(UTIL_LOADDUMP_BRIEF)
 *
 * $(UTIL_LOADDUMP)
 */
#if DOXYGEN_JS
Undefined Util::loadDump(String url, Dictionary options) {}
#elif DOXYGEN_PY
None Util::load_dump(str url, dict options) {}
#endif
void Util::load_dump(
    const std::string &url,
    const shcore::Option_pack_ref<Load_dump_options> &options) {
  const auto session = global_session();
  Scoped_log_sql log_sql{log_sql_for_dump_and_load()};
  shcore::Log_sql_guard log_sql_context{"util.loadDump()"};

  Load_dump_options opt = *options;
  opt.set_url(url);
  opt.set_session(session);
  opt.validate_and_configure();

  Dump_loader loader(opt);

  shcore::Interrupt_handler intr_handler(
      [&loader]() {
        loader.interrupt();
        return false;
      },
      [&loader]() { loader.interruption_notification(); });

  auto console = mysqlsh::current_console();
  console->print_info(opt.target_import_info());

  loader.run();
}

REGISTER_HELP_TOPIC_TEXT(TOPIC_UTIL_DUMP_COMPATIBILITY_OPTION, R"*(
<b>MySQL HeatWave Service Compatibility</b>

The MySQL HeatWave Service has a few security related restrictions that
are not present in a regular, on-premise instance of MySQL. In order to make it
easier to load existing databases into the Service, the dump commands in the
MySQL %Shell has options to detect potential issues and in some cases, to
automatically adjust your schema definition to be compliant. For best results,
always use the latest available version of MySQL %Shell.

The <b>ocimds</b> option, when set to true, will perform schema checks for
most of these issues and abort the dump if any are found. The <<<loadDump>>>()
command will also only allow loading dumps that have been created with the
"ocimds" option enabled.

Some issues found by the <b>ocimds</b> option may require you to manually
make changes to your database schema before it can be loaded into the MySQL
HeatWave Service. However, the <b>compatibility</b> option can be used to
automatically modify the dumped schema SQL scripts, resolving some of these
compatibility issues. You may pass one or more of the following values to
the "compatibility" option.

<b>create_invisible_pks</b> - Each table which does not have a Primary Key will
have one created when the dump is loaded. The following Primary Key is added
to the table:
@code
`my_row_id` BIGINT UNSIGNED AUTO_INCREMENT INVISIBLE PRIMARY KEY
@endcode
Dumps created with this value can be used with Inbound Replication into an MySQL
HeatWave Service DB System instance with High Availability, as long as target
instance has version 8.0.32 or newer. Mutually exclusive with the
<b>ignore_missing_pks</b> value.

<b>force_innodb</b> - The MySQL HeatWave Service requires use of the InnoDB
storage engine. This option will modify the ENGINE= clause of CREATE TABLE
statements that use incompatible storage engines and replace them with InnoDB.
It will also remove the ROW_FORMAT=FIXED option, as it is not supported by the
InnoDB storage engine.

<b>force_non_standard_fks</b> - In MySQL 8.4.0, a new system variable
<b>restrict_fk_on_non_standard_key</b> was added, which prohibits creation of
non-standard foreign keys (that reference non-unique keys or partial fields of
composite keys), when enabled. The MySQL HeatWave Service instances have this
variable enabled by default, which causes dumps with such tables to fail to
load. This option will disable checks for non-standard foreign keys, and cause
the loader to set the session value of <b>restrict_fk_on_non_standard_key</b>
variable to <b>OFF</b>. Creation of foreign keys with non-standard keys may
break the replication.

<b>ignore_missing_pks</b> - Ignore errors caused by tables which do not have
Primary Keys. Dumps created with this value cannot be used in MySQL HeatWave
Service DB System instance with High Availability. Mutually exclusive with the
<b>create_invisible_pks</b> value.

<b>ignore_wildcard_grants</b> - Ignore errors from grants on schemas with
wildcards, which are interpreted differently in systems where
<b>partial_revokes</b> system variable is enabled. When this variable is enabled,
the <b>_</b> and <b>%</b> characters are treated as literals, which could lead
to unexpected results. Before using this compatibility option, each such grant
should be carefully reviewed.

<b>skip_invalid_accounts</b> - Skips accounts which do not have a password or
use authentication methods (plugins) not supported by the MySQL HeatWave Service.

<b>strip_definers</b> - This option should not be used if the destination MySQL
HeatWave Service DB System instance has version 8.2.0 or newer. In such case,
the <b>administrator</b> role is granted the SET_ANY_DEFINER privilege. Users
which have this privilege are able to specify any valid authentication ID in the
DEFINER clause.

Strips the "DEFINER=account" clause from views, routines,
events and triggers. The MySQL HeatWave Service requires special privileges to
create these objects with a definer other than the user loading the schema.
By stripping the DEFINER clause, these objects will be created with that default
definer. Views and routines will additionally have their SQL SECURITY clause
changed from DEFINER to INVOKER. If this characteristic is missing, SQL SECURITY
INVOKER clause will be added. This ensures that the access permissions of the
account querying or calling these are applied, instead of the user that created
them. This should be sufficient for most users, but if your database security
model requires that views and routines have more privileges than their invoker,
you will need to manually modify the schema before loading it.

Please refer to the MySQL manual for details about DEFINER and SQL SECURITY.

<b>strip_invalid_grants</b> - Strips grant statements which would fail when
users are loaded, i.e. grants referring to a specific routine which does not
exist.

<b>strip_restricted_grants</b> - Certain privileges are restricted in the MySQL
HeatWave Service. Attempting to create users granting these privileges would
fail, so this option allows dumped GRANT statements to be stripped of these
privileges. If the destination MySQL version supports the SET_ANY_DEFINER
privilege, the SET_USER_ID privilege is replaced with SET_ANY_DEFINER instead of
being stripped.

<b>strip_tablespaces</b> - Tablespaces have some restrictions in the MySQL
HeatWave Service. If you'd like to have tables created in their default
tablespaces, this option will strip the TABLESPACE= option from CREATE TABLE
statements.

<b>unescape_wildcard_grants</b> - Fixes grants on schemas with wildcards,
replacing escaped <b>\\_</b> and <b>\\%</b> wildcards in schema names with
<b>_</b> and <b>%</b> wildcard characters. When the <b>partial_revokes</b>
system variable is enabled, the <b>\\</b> character is treated as a literal,
which could lead to unexpected results. Before using this compatibility option,
each such grant should be carefully reviewed.

Additionally, the following changes will always be made to DDL scripts
when the <b>ocimds</b> option is enabled:

@li <b>DATA DIRECTORY</b>, <b>INDEX DIRECTORY</b> and <b>ENCRYPTION</b> options
in <b>CREATE TABLE</b> statements will be commented out.

In order to use Inbound Replication into an MySQL HeatWave Service DB System
instance with High Availability where instance has version older than 8.0.32,
all tables at the source server need to have Primary Keys. This needs to be
fixed manually before running the dump. Starting with MySQL 8.0.23 invisible
columns may be used to add Primary Keys without changing the schema
compatibility, for more information see:
https://dev.mysql.com/doc/refman/en/invisible-columns.html.

In order to use Inbound Replication into an MySQL HeatWave Service DB System
instance with High Availability, please see
https://docs.oracle.com/en-us/iaas/mysql-database/doc/creating-replication-channel.html.

In order to use MySQL HeatWave Service DB Service instance with High
Availability, all tables must have a Primary Key. This can be fixed
automatically using the <b>create_invisible_pks</b> compatibility value.

Please refer to the MySQL HeatWave Service documentation for more information
about restrictions and compatibility.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_DDL_COMMON_PARAMETERS, R"*(
The <b>outputUrl</b> specifies URL to a directory where the dump is going to be
stored. If the output directory does not exist but its parent does, it is
created. If the output directory exists, it must be empty. All directories are
created with the following access rights (on operating systems which support
them): <b>rwxr-x---</b>. All files are created with the following access rights
(on operating systems which support them): <b>rw-r-----</b>. Allowed values:

${TOPIC_REMOTE_STORAGE_COMMON_PATHS}
@li <b>OCI Pre-Authenticated Request to a bucket or a prefix</b> - dumps to the
OCI Object Storage using a PAR

${TOPIC_REMOTE_STORAGE_MORE_INFO}
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_EXPORT_COMMON_OPTIONS, R"*(
@li <b>fieldsTerminatedBy</b>: string (default: "\t") - This option has the same
meaning as the corresponding clause for SELECT ... INTO OUTFILE.
@li <b>fieldsEnclosedBy</b>: char (default: '') - This option has the same
meaning as the corresponding clause for SELECT ... INTO OUTFILE.
@li <b>fieldsEscapedBy</b>: char (default: '\\') - This option has the same
meaning as the corresponding clause for SELECT ... INTO OUTFILE.
@li <b>fieldsOptionallyEnclosed</b>: bool (default: false) - Set to true if the
input values are not necessarily enclosed within quotation marks specified by
<b>fieldsEnclosedBy</b> option. Set to false if all fields are quoted by
character specified by <b>fieldsEnclosedBy</b> option.
@li <b>linesTerminatedBy</b>: string (default: "\n") - This option has the same
meaning as the corresponding clause for SELECT ... INTO OUTFILE. See Section
13.2.10.1, "SELECT ... INTO Statement".
@li <b>dialect</b>: enum (default: "default") - Setup fields and lines options
that matches specific data file format. Can be used as base dialect and
customized with <b>fieldsTerminatedBy</b>, <b>fieldsEnclosedBy</b>,
<b>fieldsEscapedBy</b>, <b>fieldsOptionallyEnclosed</b> and
<b>linesTerminatedBy</b> options. Must be one of the following values: default,
csv, tsv or csv-unix.

@li <b>maxRate</b>: string (default: "0") - Limit data read throughput to
maximum rate, measured in bytes per second per thread. Use maxRate="0" to set no
limit.
@li <b>showProgress</b>: bool (default: true if stdout is a TTY device, false
otherwise) - Enable or disable dump progress information.
@li <b>defaultCharacterSet</b>: string (default: "utf8mb4") - Character set used
for the dump.)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_DDL_COMMON_OPTIONS, R"*(
@li <b>triggers</b>: bool (default: true) - Include triggers for each dumped
table.
@li <b>excludeTriggers</b>: list of strings (default: empty) - List of triggers
to be excluded from the dump in the format of <b>schema</b>.<b>table</b>
(all triggers from the specified table) or
<b>schema</b>.<b>table</b>.<b>trigger</b> (the individual trigger).
@li <b>includeTriggers</b>: list of strings (default: empty) - List of triggers
to be included in the dump in the format of <b>schema</b>.<b>table</b>
(all triggers from the specified table) or
<b>schema</b>.<b>table</b>.<b>trigger</b> (the individual trigger).

@li <b>where</b>: dictionary (default: not set) - A key-value pair of a table
name in the format of <b>schema.table</b> and a valid SQL condition expression
used to filter the data being exported.
@li <b>partitions</b>: dictionary (default: not set) - A key-value pair of a
table name in the format of <b>schema.table</b> and a list of valid partition
names used to limit the data export to just the specified partitions.

@li <b>tzUtc</b>: bool (default: true) - Convert TIMESTAMP data to UTC.

@li <b>consistent</b>: bool (default: true) - Enable or disable consistent data
dumps. When enabled, produces a transactionally consistent dump at a specific
point in time.
@li <b>skipConsistencyChecks</b>: bool (default: false) - Skips additional
consistency checks which are executed when running consistent dumps and i.e.
backup lock cannot not be acquired.
@li <b>ddlOnly</b>: bool (default: false) - Only dump Data Definition Language
(DDL) from the database.
@li <b>dataOnly</b>: bool (default: false) - Only dump data from the database.
@li <b>checksum</b>: bool (default: false) - Compute and include checksum of the
dumped data.
@li <b>dryRun</b>: bool (default: false) - Print information about what would be
dumped, but do not dump anything. If <b>ocimds</b> is enabled, also checks for
compatibility issues with MySQL HeatWave Service.

@li <b>chunking</b>: bool (default: true) - Enable chunking of the tables.
@li <b>bytesPerChunk</b>: string (default: "64M") - Sets average estimated
number of bytes to be written to each chunk file, enables <b>chunking</b>.
@li <b>threads</b>: int (default: 4) - Use N threads to dump data chunks from
the server.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_DDL_COMPRESSION, R"*(
@li <b>compression</b>: string (default: "zstd;level=1") - Compression used when writing
the data dump files, one of: "none", "gzip", "zstd". Compression level may be
specified as "gzip;level=8" or "zstd;level=8".
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_MDS_COMMON_OPTIONS, R"*(
@li <b>ocimds</b>: bool (default: false) - Enable checks for compatibility with
MySQL HeatWave Service.
@li <b>compatibility</b>: list of strings (default: empty) - Apply MySQL
HeatWave Service compatibility modifications when writing dump files. Supported
values: "create_invisible_pks", "force_innodb", "force_non_standard_fks",
"ignore_missing_pks", "ignore_wildcard_grants", "skip_invalid_accounts",
"strip_definers", "strip_invalid_grants", "strip_restricted_grants",
"strip_tablespaces", "unescape_wildcard_grants".
@li <b>targetVersion</b>: string (default: current version of %Shell) -
Specifies version of the destination MySQL server.
@li <b>skipUpgradeChecks</b>: bool (default: false) - Do not execute the
upgrade check utility. Compatibility issues related to MySQL version upgrades
will not be checked. Use this option only when executing the Upgrade Checker
separately.)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_SCHEMAS_COMMON_OPTIONS, R"*(
@li <b>excludeTables</b>: list of strings (default: empty) - List of tables or
views to be excluded from the dump in the format of <b>schema</b>.<b>table</b>.
@li <b>includeTables</b>: list of strings (default: empty) - List of tables or
views to be included in the dump in the format of <b>schema</b>.<b>table</b>.

${TOPIC_UTIL_DUMP_MDS_COMMON_OPTIONS}

@li <b>events</b>: bool (default: true) - Include events from each dumped
schema.
@li <b>excludeEvents</b>: list of strings (default: empty) - List of events
to be excluded from the dump in the format of <b>schema</b>.<b>event</b>.
@li <b>includeEvents</b>: list of strings (default: empty) - List of events
to be included in the dump in the format of <b>schema</b>.<b>event</b>.

@li <b>routines</b>: bool (default: true) - Include functions and stored
procedures for each dumped schema.
@li <b>excludeRoutines</b>: list of strings (default: empty) - List of routines
to be excluded from the dump in the format of <b>schema</b>.<b>routine</b>.
@li <b>includeRoutines</b>: list of strings (default: empty) - List of routines
to be included in the dump in the format of <b>schema</b>.<b>routine</b>.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_SESSION_DETAILS, R"*(
Requires an open, global %Shell session, and uses its connection options, such
as compression, ssl-mode, etc., to establish additional connections.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_EXPORT_COMMON_REQUIREMENTS, R"*(
<b>Requirements</b>
@li MySQL Server 5.7 or newer is required.
@li Size limit for individual files uploaded to the cloud storage is 1.2 TiB.
@li Columns with data types which are not safe to be stored in text form (i.e.
BLOB) are converted to Base64, hence the size of such columns cannot exceed
approximately 0.74 * <b>max_allowed_packet</b> bytes, as configured through that
system variable at the target server.)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_DDL_COMMON_REQUIREMENTS, R"*(
${TOPIC_UTIL_DUMP_EXPORT_COMMON_REQUIREMENTS}
@li Schema object names must use latin1 or utf8 character set.
@li Only tables which use the InnoDB storage engine are guaranteed to be dumped
with consistent data.)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_SCHEMAS_COMMON_DETAILS, R"*(
${TOPIC_UTIL_DUMP_DDL_COMMON_REQUIREMENTS}

<b>Details</b>

This operation writes SQL files per each schema, table and view dumped, along
with some global SQL files.

Table data dumps are written to text files using the specified file format,
optionally splitting them into multiple chunk files.

${TOPIC_UTIL_DUMP_SESSION_DETAILS}

Data dumps cannot be created for the following tables:
@li mysql.apply_status
@li mysql.general_log
@li mysql.schema
@li mysql.slow_log
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_EXPORT_DIALECT_OPTION_DETAILS, R"*(
The <b>dialect</b> option predefines the set of options fieldsTerminatedBy (FT),
fieldsEnclosedBy (FE), fieldsOptionallyEnclosed (FOE), fieldsEscapedBy (FESC)
and linesTerminatedBy (LT) in the following manner:
@li default: no quoting, tab-separated, LF line endings.
(LT=@<LF@>, FESC='\', FT=@<TAB@>, FE=@<empty@>, FOE=false)
@li csv: optionally quoted, comma-separated, CRLF line endings.
(LT=@<CR@>@<LF@>, FESC='\', FT=",", FE='&quot;', FOE=true)
@li tsv: optionally quoted, tab-separated, CRLF line endings.
(LT=@<CR@>@<LF@>, FESC='\', FT=@<TAB@>, FE='&quot;', FOE=true)
@li csv-unix: fully quoted, comma-separated, LF line endings.
(LT=@<LF@>, FESC='\', FT=",", FE='&quot;', FOE=false)
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_DDL_COMMON_OPTION_DETAILS, R"*(
The names given in the <b>exclude{object}</b>, <b>include{object}</b>,
<b>where</b> or <b>partitions</b> options should be valid MySQL identifiers,
quoted using backtick characters when required.

If the <b>exclude{object}</b>, <b>include{object}</b>, <b>where</b> or
<b>partitions</b> options contain an object which does not exist, or an object
which belongs to a schema which does not exist, it is ignored.

The <b>tzUtc</b> option allows dumping TIMESTAMP data when a server has data in
different time zones or data is being moved between servers with different time
zones.

If the <b>consistent</b> option is set to true, a global read lock is set using
the <b>FLUSH TABLES WITH READ LOCK</b> statement, all threads establish
connections with the server and start transactions using:
@li SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ
@li START TRANSACTION WITH CONSISTENT SNAPSHOT

Once all the threads start transactions, the instance is locked for backup and
the global read lock is released.

If the account used for the dump does not have enough privileges to execute
FLUSH TABLES, LOCK TABLES will be used as a fallback instead. All tables being
dumped, in addition to DDL and GRANT related tables in the mysql schema will
be temporarily locked.

The <b>ddlOnly</b> and <b>dataOnly</b> options cannot both be set to true at
the same time.

The <b>chunking</b> option causes the the data from each table to be split and
written to multiple chunk files. If this option is set to false, table data is
written to a single file.

If the <b>chunking</b> option is set to <b>true</b>, but a table to be dumped
cannot be chunked (for example if it does not contain a primary key or a unique
index), data is dumped to multiple files using a single thread.

The value of the <b>threads</b> option must be a positive number.

${TOPIC_UTIL_DUMP_EXPORT_DIALECT_OPTION_DETAILS}

Both the <b>bytesPerChunk</b> and <b>maxRate</b> options support unit suffixes:
@li k - for kilobytes,
@li M - for Megabytes,
@li G - for Gigabytes,

i.e. maxRate="2k" - limit throughput to 2000 bytes per second.

The value of the <b>bytesPerChunk</b> option cannot be smaller than "128k".
)*");

REGISTER_HELP_FUNCTION(exportTable, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_EXPORTTABLE, R"*(
Exports the specified table to the data dump file.

@param table Name of the table to be exported.
@param outputUrl Target file to store the data.
@param options Optional dictionary with the export options.

The value of <b>table</b> parameter should be in form of <b>table</b> or
<b>schema</b>.<b>table</b>, quoted using backtick characters when required. If
schema is omitted, an active schema on the global %Shell session is used. If
there is none, an exception is raised.

The <b>outputUrl</b> specifies URL to a file where the exported data is going to
be stored. The parent directory of the output file must exist. If the output
file exists, it is going to be overwritten. The output file is created with the
following access rights (on operating systems which support them):
<b>rw-r-----</b>. Allowed values:

${TOPIC_REMOTE_STORAGE_COMMON_PATHS}
@li <b>OCI Pre-Authenticated Request to an object</b> - exports to a specific
file

${TOPIC_REMOTE_STORAGE_MORE_INFO}

<b>The following options are supported:</b>
@li <b>where</b>: string (default: not set) - A valid SQL condition expression
used to filter the data being exported.
@li <b>partitions</b>: list of strings (default: not set) - A list of valid
partition names used to limit the data export to just the specified partitions.

${TOPIC_UTIL_DUMP_EXPORT_COMMON_OPTIONS}
@li <b>compression</b>: string (default: "none") - Compression used when writing
the data dump files, one of: "none", "gzip", "zstd". Compression level may be
specified as "gzip;level=8" or "zstd;level=8".

${TOPIC_REMOTE_STORAGE_OPTIONS}

${TOPIC_UTIL_DUMP_EXPORT_COMMON_REQUIREMENTS}

<b>Details</b>

This operation writes table data dump to the specified by the user files.

${TOPIC_UTIL_DUMP_SESSION_DETAILS}

<b>%Options</b>

${TOPIC_UTIL_DUMP_EXPORT_DIALECT_OPTION_DETAILS}

The <b>maxRate</b> option supports unit suffixes:
@li k - for kilobytes,
@li M - for Megabytes,
@li G - for Gigabytes,

i.e. maxRate="2k" - limit throughput to 2000 bytes per second.
)*");

/**
 * \ingroup util
 *
 * $(UTIL_EXPORTTABLE_BRIEF)
 *
 * $(UTIL_EXPORTTABLE)
 */
#if DOXYGEN_JS
Undefined Util::exportTable(String table, String outputUrl, Dictionary options);
#elif DOXYGEN_PY
None Util::export_table(str table, str outputUrl, dict options);
#endif
void Util::export_table(
    const std::string &table, const std::string &file,
    const shcore::Option_pack_ref<dump::Export_table_options> &options) {
  const auto session = global_session();
  Scoped_log_sql log_sql{log_sql_for_dump_and_load()};
  shcore::Log_sql_guard log_sql_context{"util.exportTable()"};

  using mysqlsh::dump::Export_table;

  mysqlsh::dump::Export_table_options opts = *options;
  opts.set_table(table);
  opts.set_output_url(file);
  opts.set_session(session);
  opts.validate_and_configure();

  Export_table dumper{opts};

  shcore::Interrupt_handler intr_handler(
      [&dumper]() {
        dumper.interrupt();
        return false;
      },
      [&dumper]() {
        dumper.interruption_notification();
        dumper.abort();
      });

  dumper.run();
}

REGISTER_HELP_FUNCTION(dumpTables, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_DUMPTABLES, R"*(
Dumps the specified tables or views from the given schema to the files in the
target directory.

@param schema Name of the schema that contains tables/views to be dumped.
@param tables List of tables/views to be dumped.
@param outputUrl Target directory to store the dump files.
@param options Optional dictionary with the dump options.

The <b>tables</b> parameter cannot be an empty list.

${TOPIC_UTIL_DUMP_DDL_COMMON_PARAMETERS}

<b>The following options are supported:</b>
@li <b>all</b>: bool (default: false) - Dump all views and tables from the
specified schema.

${TOPIC_UTIL_DUMP_MDS_COMMON_OPTIONS}

${TOPIC_UTIL_DUMP_DDL_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_EXPORT_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_DDL_COMPRESSION}

${TOPIC_REMOTE_STORAGE_OPTIONS}

${TOPIC_UTIL_DUMP_DDL_COMMON_REQUIREMENTS}
@li Views and triggers to be dumped must not use qualified names to reference
other views or tables.
@li Since util.<<<dumpTables>>>() function does not dump routines, any routines
referenced by the dumped objects are expected to already exist when the dump is
loaded.

<b>Details</b>

This operation writes SQL files per each table and view dumped, along with some
global SQL files. The information about the source schema is also saved, meaning
that when using the util.<<<loadDump>>>() function to load the dump, it is
automatically recreated. Alternatively, dump can be loaded into another existing
schema using the <b>schema</b> option.

Table data dumps are written to text files using the specified file format,
optionally splitting them into multiple chunk files.

${TOPIC_UTIL_DUMP_SESSION_DETAILS}

<b>%Options</b>

If the <b>all</b> option is set to true and the <b>tables</b> parameter is set
to an empty array, all views and tables from the specified schema are going to
be dumped. If the <b>tables</b> parameter is not set to an empty array, an
exception is thrown.

${TOPIC_UTIL_DUMP_DDL_COMMON_OPTION_DETAILS}
${TOPIC_UTIL_DUMP_COMPATIBILITY_OPTION}
)*");

/**
 * \ingroup util
 *
 * $(UTIL_DUMPTABLES_BRIEF)
 *
 * $(UTIL_DUMPTABLES)
 */
#if DOXYGEN_JS
Undefined Util::dumpTables(String schema, List tables, String outputUrl,
                           Dictionary options);
#elif DOXYGEN_PY
None Util::dump_tables(str schema, list tables, str outputUrl, dict options);
#endif
void Util::dump_tables(
    const std::string &schema, const std::vector<std::string> &tables,
    const std::string &directory,
    const shcore::Option_pack_ref<dump::Dump_tables_options> &options) {
  const auto session = global_session();
  Scoped_log_sql log_sql{log_sql_for_dump_and_load()};
  shcore::Log_sql_guard log_sql_context{"util.dumpTables()"};

  using mysqlsh::dump::Dump_tables;

  mysqlsh::dump::Dump_tables_options opts = *options;
  opts.set_schema(schema);
  opts.set_tables(tables);
  opts.set_output_url(directory);
  opts.set_session(session);
  opts.validate_and_configure();

  Dump_tables dumper{opts};

  shcore::Interrupt_handler intr_handler(
      [&dumper]() {
        dumper.interrupt();
        return false;
      },
      [&dumper]() {
        dumper.interruption_notification();
        dumper.abort();
      });

  dumper.run();
}

REGISTER_HELP_FUNCTION(dumpSchemas, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_DUMPSCHEMAS, R"*(
Dumps the specified schemas to the files in the output directory.

@param schemas List of schemas to be dumped.
@param outputUrl Target directory to store the dump files.
@param options Optional dictionary with the dump options.

The <b>schemas</b> parameter cannot be an empty list.

${TOPIC_UTIL_DUMP_DDL_COMMON_PARAMETERS}

<b>The following options are supported:</b>
${TOPIC_UTIL_DUMP_SCHEMAS_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_DDL_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_EXPORT_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_DDL_COMPRESSION}

${TOPIC_REMOTE_STORAGE_OPTIONS}

${TOPIC_UTIL_DUMP_SCHEMAS_COMMON_DETAILS}

<b>%Options</b>

${TOPIC_UTIL_DUMP_DDL_COMMON_OPTION_DETAILS}
${TOPIC_UTIL_DUMP_COMPATIBILITY_OPTION}
)*");

/**
 * \ingroup util
 *
 * $(UTIL_DUMPSCHEMAS_BRIEF)
 *
 * $(UTIL_DUMPSCHEMAS)
 */
#if DOXYGEN_JS
Undefined Util::dumpSchemas(List schemas, String outputUrl, Dictionary options);
#elif DOXYGEN_PY
None Util::dump_schemas(list schemas, str outputUrl, dict options);
#endif
void Util::dump_schemas(
    const std::vector<std::string> &schemas, const std::string &directory,
    const shcore::Option_pack_ref<dump::Dump_schemas_options> &options) {
  const auto session = global_session();
  Scoped_log_sql log_sql{log_sql_for_dump_and_load()};
  shcore::Log_sql_guard log_sql_context{"util.dumpSchemas()"};

  using mysqlsh::dump::Dump_schemas;

  mysqlsh::dump::Dump_schemas_options opts = *options;
  opts.set_schemas(schemas);
  opts.set_output_url(directory);
  opts.set_session(session);
  opts.validate_and_configure();

  Dump_schemas dumper{opts};

  shcore::Interrupt_handler intr_handler(
      [&dumper]() {
        dumper.interrupt();
        return false;
      },
      [&dumper]() {
        dumper.interruption_notification();
        dumper.abort();
      });

  dumper.run();
}

REGISTER_HELP_FUNCTION(dumpInstance, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_DUMPINSTANCE, R"*(
Dumps the whole database to files in the output directory.

@param outputUrl Target directory to store the dump files.
@param options Optional dictionary with the dump options.

${TOPIC_UTIL_DUMP_DDL_COMMON_PARAMETERS}

<b>The following options are supported:</b>
@li <b>excludeSchemas</b>: list of strings (default: empty) - List of schemas to
be excluded from the dump.
@li <b>includeSchemas</b>: list of strings (default: empty) - List of schemas to
be included in the dump.
${TOPIC_UTIL_DUMP_SCHEMAS_COMMON_OPTIONS}
@li <b>users</b>: bool (default: true) - Include users, roles and grants in the
dump file.
@li <b>excludeUsers</b>: array of strings (default not set) - Skip dumping the
specified users. Each user is in the format of 'user_name'[@'host']. If the host
is not specified, all the accounts with the given user name are excluded.
@li <b>includeUsers</b>: array of strings (default not set) - Dump only the
specified users. Each user is in the format of 'user_name'[@'host']. If the host
is not specified, all the accounts with the given user name are included. By
default, all users are included.

${TOPIC_UTIL_DUMP_DDL_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_EXPORT_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_DDL_COMPRESSION}

${TOPIC_REMOTE_STORAGE_OPTIONS}

${TOPIC_UTIL_DUMP_SCHEMAS_COMMON_DETAILS}

Dumps cannot be created for the following schemas:
@li information_schema,
@li mysql,
@li ndbinfo,
@li performance_schema,
@li sys.

<b>%Options</b>

If the <b>excludeSchemas</b> or <b>includeSchemas</b> options contain a schema
which is not included in the dump or does not exist, it is ignored.

${TOPIC_UTIL_DUMP_DDL_COMMON_OPTION_DETAILS}
${TOPIC_UTIL_DUMP_COMPATIBILITY_OPTION}
)*");

/**
 * \ingroup util
 *
 * $(UTIL_DUMPINSTANCE_BRIEF)
 *
 * $(UTIL_DUMPINSTANCE)
 */
#if DOXYGEN_JS
Undefined Util::dumpInstance(String outputUrl, Dictionary options);
#elif DOXYGEN_PY
None Util::dump_instance(str outputUrl, dict options);
#endif
void Util::dump_instance(
    const std::string &directory,
    const shcore::Option_pack_ref<dump::Dump_instance_options> &options) {
  const auto session = global_session();
  Scoped_log_sql log_sql{log_sql_for_dump_and_load()};
  shcore::Log_sql_guard log_sql_context{"util.dumpInstance()"};

  using mysqlsh::dump::Dump_instance;

  mysqlsh::dump::Dump_instance_options opts = *options;
  opts.set_output_url(directory);
  opts.set_session(session);
  opts.validate_and_configure();

  Dump_instance dumper{opts};

  shcore::Interrupt_handler intr_handler(
      [&dumper]() {
        dumper.interrupt();
        return false;
      },
      [&dumper]() {
        dumper.interruption_notification();
        dumper.abort();
      });

  dumper.run();
}

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_COPY_COMMON_DESCRIPTION, R"*(
Runs simultaneous dump and load operations, while storing the dump artifacts in
memory.

If target is a MySQL HeatWave Service DB System instance, automatically checks
for compatibility with it.

${TOPIC_CONNECTION_DATA}
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_COPY_SCHEMAS_COMMON_OPTIONS, R"*(
@li <b>excludeTables</b>: list of strings (default: empty) - List of tables or
views to be excluded from the copy in the format of <b>schema</b>.<b>table</b>.
@li <b>includeTables</b>: list of strings (default: empty) - List of tables or
views to be included in the copy in the format of <b>schema</b>.<b>table</b>.

@li <b>events</b>: bool (default: true) - Include events from each copied
schema.
@li <b>excludeEvents</b>: list of strings (default: empty) - List of events
to be excluded from the copy in the format of <b>schema</b>.<b>event</b>.
@li <b>includeEvents</b>: list of strings (default: empty) - List of events
to be included in the copy in the format of <b>schema</b>.<b>event</b>.

@li <b>routines</b>: bool (default: true) - Include functions and stored
procedures for each copied schema.
@li <b>excludeRoutines</b>: list of strings (default: empty) - List of routines
to be excluded from the copy in the format of <b>schema</b>.<b>routine</b>.
@li <b>includeRoutines</b>: list of strings (default: empty) - List of routines
to be included in the copy in the format of <b>schema</b>.<b>routine</b>.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_COPY_COMMON_OPTIONS, R"*(
@li <b>triggers</b>: bool (default: true) - Include triggers for each copied
table.
@li <b>excludeTriggers</b>: list of strings (default: empty) - List of triggers
to be excluded from the copy in the format of <b>schema</b>.<b>table</b>
(all triggers from the specified table) or
<b>schema</b>.<b>table</b>.<b>trigger</b> (the individual trigger).
@li <b>includeTriggers</b>: list of strings (default: empty) - List of triggers
to be included in the copy in the format of <b>schema</b>.<b>table</b>
(all triggers from the specified table) or
<b>schema</b>.<b>table</b>.<b>trigger</b> (the individual trigger).

@li <b>where</b>: dictionary (default: not set) - A key-value pair of a table
name in the format of <b>schema.table</b> and a valid SQL condition expression
used to filter the data being copied.
@li <b>partitions</b>: dictionary (default: not set) - A key-value pair of a
table name in the format of <b>schema.table</b> and a list of valid partition
names used to limit the data copy to just the specified partitions.

@li <b>compatibility</b>: list of strings (default: empty) - Apply MySQL
HeatWave Service compatibility modifications when copying the DDL. Supported
values: "create_invisible_pks", "force_innodb", "force_non_standard_fks",
"ignore_missing_pks", "ignore_wildcard_grants", "skip_invalid_accounts",
"strip_definers", "strip_invalid_grants", "strip_restricted_grants",
"strip_tablespaces", "unescape_wildcard_grants".

@li <b>tzUtc</b>: bool (default: true) - Convert TIMESTAMP data to UTC.

@li <b>consistent</b>: bool (default: true) - Enable or disable consistent data
copies. When enabled, produces a transactionally consistent copy at a specific
point in time.
@li <b>skipConsistencyChecks</b>: bool (default: false) - Skips additional
consistency checks which are executed when running consistent copies and i.e.
backup lock cannot not be acquired.
@li <b>ddlOnly</b>: bool (default: false) - Only copy Data Definition Language
(DDL) from the database.
@li <b>dataOnly</b>: bool (default: false) - Only copy data from the database.
@li <b>checksum</b>: bool (default: false) - Compute checksums of the data and
verify tables in the target instance against these checksums.
@li <b>dryRun</b>: bool (default: false) - Simulates a copy and prints
everything that would be performed, without actually doing so. If target is
MySQL HeatWave Service, also checks for compatibility issues.

@li <b>chunking</b>: bool (default: true) - Enable chunking of the tables.
@li <b>bytesPerChunk</b>: string (default: "64M") - Sets average estimated
number of bytes to be copied in each chunk, enables <b>chunking</b>.

@li <b>threads</b>: int (default: 4) - Use N threads to read the data from
the source server and additional N threads to write the data to the target
server.

@li <b>maxRate</b>: string (default: "0") - Limit data read throughput to
maximum rate, measured in bytes per second per thread. Use maxRate="0" to set no
limit.
@li <b>showProgress</b>: bool (default: true if stdout is a TTY device, false
otherwise) - Enable or disable copy progress information.
@li <b>defaultCharacterSet</b>: string (default: "utf8mb4") - Character set used
for the copy.

@li <b>analyzeTables</b>: "off", "on", "histogram" (default: off) - If 'on',
executes ANALYZE TABLE for all tables, once copied. If set to 'histogram', only
tables that have histogram information stored in the copy will be analyzed.
@li <b>deferTableIndexes</b>: "off", "fulltext", "all" (default: fulltext) -
If "all", creation of "all" indexes except PRIMARY is deferred until after
table data is copied, which in many cases can reduce load times. If "fulltext",
only full-text indexes will be deferred.
@li <b>dropExistingObjects</b>: bool (default false) - Perform the copy even if
it contains objects that already exist in the target database. Drops existing
user accounts and objects (excluding schemas) before copying them. Mutually
exclusive with <b>ignoreExistingObjects</b>.
@li <b>handleGrantErrors</b>: "abort", "drop_account", "ignore" (default: abort)
- Specifies action to be performed in case of errors related to the GRANT/REVOKE
statements, "abort": throws an error and aborts the copy, "drop_account":
deletes the problematic account and continues, "ignore": ignores the error and
continues copying the account.
@li <b>ignoreExistingObjects</b>: bool (default false) - Perform the copy even
if it contains objects that already exist in the target database. Ignores
existing user accounts and objects. Mutually exclusive with
<b>dropExistingObjects</b>.
@li <b>ignoreVersion</b>: bool (default false) - Perform the copy even if the
major version number of the server where it was created is different from where
it will be loaded.
@li <b>loadIndexes</b>: bool (default: true) - use together with
<b>deferTableIndexes</b> to control whether secondary indexes should be
recreated at the end of the copy.
@li <b>maxBytesPerTransaction</b>: string (default: the value of
<b>bytesPerChunk</b>) - Specifies the maximum number of bytes that can be copied
per single LOAD DATA statement. Supports unit suffixes: k (kilobytes), M
(Megabytes), G (Gigabytes). Minimum value: 4096.
@li <b>schema</b>: string (default not set) - Copy the data into the given
schema. This option can only be used when copying just one schema.
@li <b>sessionInitSql</b>: list of strings (default: []) - execute the given
list of SQL statements in each session about to copy data.
@li <b>skipBinlog</b>: bool (default: false) - Disables the binary log
for the MySQL sessions used by the loader (set sql_log_bin=0).
@li <b>updateGtidSet</b>: "off", "replace", "append" (default: off) - if set to
a value other than 'off' updates GTID_PURGED by either replacing its contents
or appending to it the gtid set present in the copy.
)*");

REGISTER_HELP_FUNCTION(copyInstance, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_COPYINSTANCE, R"*(
Copies a source instance to the target instance. Requires an open global %Shell
session to the source instance, if there is none, an exception is raised.

@param connectionData Specifies the connection information required to establish
a connection to the target instance.
@param options Optional dictionary with the copy options.

${TOPIC_UTIL_COPY_COMMON_DESCRIPTION}

<b>The following options are supported:</b>
@li <b>excludeSchemas</b>: list of strings (default: empty) - List of schemas to
be excluded from the copy.
@li <b>includeSchemas</b>: list of strings (default: empty) - List of schemas to
be included in the copy.

${TOPIC_UTIL_COPY_SCHEMAS_COMMON_OPTIONS}
@li <b>users</b>: bool (default: true) - Include users, roles and grants in the
copy.
@li <b>excludeUsers</b>: list of strings (default not set) - Skip copying the
specified users. Each user is in the format of 'user_name'[@'host']. If the host
is not specified, all the accounts with the given user name are excluded.
@li <b>includeUsers</b>: list of strings (default not set) - Copy only the
specified users. Each user is in the format of 'user_name'[@'host']. If the host
is not specified, all the accounts with the given user name are included. By
default, all users are included.

${TOPIC_UTIL_COPY_COMMON_OPTIONS}

For discussion of all options see: <<<dumpInstance>>>() and <<<loadDump>>>().
)*");

/**
 * \ingroup util
 *
 * $(UTIL_COPYINSTANCE_BRIEF)
 *
 * $(UTIL_COPYINSTANCE)
 */
#if DOXYGEN_JS
Undefined Util::copyInstance(ConnectionData connectionData, Dictionary options);
#elif DOXYGEN_PY
None Util::copy_instance(ConnectionData connectionData, dict options);
#endif
void Util::copy_instance(
    const mysqlshdk::db::Connection_options &connection_options,
    const shcore::Option_pack_ref<copy::Copy_instance_options> &options) {
  const auto session = global_session();
  Scoped_log_sql log_sql{log_sql_for_dump_and_load()};
  shcore::Log_sql_guard log_sql_context{"util.copyInstance()"};

  auto copy_options = *options;
  copy_options.dump_options()->set_session(session);

  copy::copy<mysqlsh::dump::Dump_instance>(connection_options, &copy_options);
}

REGISTER_HELP_FUNCTION(copySchemas, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_COPYSCHEMAS, R"*(
Copies schemas from the source instance to the target instance. Requires an open
global %Shell session to the source instance, if there is none, an exception is
raised.

@param schemas List of strings with names of schemas to be copied.
@param connectionData Specifies the connection information required to establish
a connection to the target instance.
@param options Optional dictionary with the copy options.

${TOPIC_UTIL_COPY_COMMON_DESCRIPTION}

<b>The following options are supported:</b>
${TOPIC_UTIL_COPY_SCHEMAS_COMMON_OPTIONS}

${TOPIC_UTIL_COPY_COMMON_OPTIONS}

For discussion of all options see: <<<dumpSchemas>>>() and <<<loadDump>>>().
)*");

/**
 * \ingroup util
 *
 * $(UTIL_COPYSCHEMAS_BRIEF)
 *
 * $(UTIL_COPYSCHEMAS)
 */
#if DOXYGEN_JS
Undefined Util::copySchemas(List schemas, ConnectionData connectionData,
                            Dictionary options);
#elif DOXYGEN_PY
None Util::copy_schemas(list schemas, ConnectionData connectionData,
                        dict options);
#endif
void Util::copy_schemas(
    const std::vector<std::string> &schemas,
    const mysqlshdk::db::Connection_options &connection_options,
    const shcore::Option_pack_ref<copy::Copy_schemas_options> &options) {
  const auto session = global_session();
  Scoped_log_sql log_sql{log_sql_for_dump_and_load()};
  shcore::Log_sql_guard log_sql_context{"util.copySchemas()"};

  auto copy_options = *options;
  copy_options.dump_options()->set_schemas(schemas);
  copy_options.dump_options()->set_session(session);

  copy::copy<mysqlsh::dump::Dump_schemas>(connection_options, &copy_options);
}

REGISTER_HELP_FUNCTION(copyTables, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_COPYTABLES, R"*(
Copies tables and views from schema in the source instance to the target
instance. Requires an open global %Shell session to the source instance, if
there is none, an exception is raised.

@param schema Name of the schema that contains tables and views to be copied.
@param tables List of strings with names of tables and views to be copied.
@param connectionData Specifies the connection information required to establish
a connection to the target instance.
@param options Optional dictionary with the copy options.

${TOPIC_UTIL_COPY_COMMON_DESCRIPTION}

<b>The following options are supported:</b>
@li <b>all</b>: bool (default: false) - Copy all views and tables from the
specified schema, requires the <b>tables</b> argument to be an empty list.

${TOPIC_UTIL_COPY_COMMON_OPTIONS}

For discussion of all options see: <<<dumpTables>>>() and <<<loadDump>>>().
)*");

/**
 * \ingroup util
 *
 * $(UTIL_COPYTABLES_BRIEF)
 *
 * $(UTIL_COPYTABLES)
 */
#if DOXYGEN_JS
Undefined Util::copyTables(String schema, List tables,
                           ConnectionData connectionData, Dictionary options);
#elif DOXYGEN_PY
None Util::copy_tables(str schema, list tables, ConnectionData connectionData,
                       dict options);
#endif
void Util::copy_tables(
    const std::string &schema, const std::vector<std::string> &tables,
    const mysqlshdk::db::Connection_options &connection_options,
    const shcore::Option_pack_ref<copy::Copy_tables_options> &options) {
  const auto session = global_session();
  Scoped_log_sql log_sql{log_sql_for_dump_and_load()};
  shcore::Log_sql_guard log_sql_context{"util.copyTables()"};

  auto copy_options = *options;
  copy_options.dump_options()->set_schema(schema);
  copy_options.dump_options()->set_tables(tables);
  copy_options.dump_options()->set_session(session);

  copy::copy<mysqlsh::dump::Dump_tables>(connection_options, &copy_options);
}

std::shared_ptr<mysqlshdk::db::ISession> Util::global_session() const {
  const auto session = _shell_core.get_dev_session();

  if (!session || !session->is_open()) {
    throw std::runtime_error(
        "An open session is required to perform this operation.");
  }

  return session->get_core_session();
}

REGISTER_HELP_FUNCTION(dumpBinlogs, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_DUMPBINLOGS, R"*(
Dumps binary logs generated since a specific point in time to the given local or
remote directory.

@param outputUrl Target directory to store the dump files.
@param options Optional dictionary with the dump options.

The <b>outputUrl</b> specifies URL to a directory where the dump is going to be
stored. Allowed values:

${TOPIC_REMOTE_STORAGE_COMMON_PATHS}
@li <b>OCI Pre-Authenticated Request to a bucket or a prefix</b> - dumps to the
OCI Object Storage using a PAR

${TOPIC_REMOTE_STORAGE_MORE_INFO}

The <b>options</b> dictionary supports the following options:
@li <b>since</b>: string (default: not set) - URL to a local or remote directory
containing dump created either by <b>util.<<<dumpInstance>>>()</b> or
<b>util.<<<dumpBinlogs>>>()</b>. Binary logs since the specified dump are going
to be dumped.
@li <b>startFrom</b>: string (default: not set) - Specifies the name of the
binary log file and its position to start the dump from. Accepted format:
<b>@<binary-log-file@>[:@<binary-log-file-position@>]</b>.
@li <b>ignoreDdlChanges</b>: bool (default: false) - Proceeds with the dump even
if the <b>since</b> option points to a dump created by
<b>util.<<<dumpInstance>>>()</b> which contains modified DDL.

@li <b>threads</b>: unsigned int (default: 4) - Use N threads to dump the data
from the instance.
@li <b>dryRun</b>: bool (default: false) - Print information about what would be
dumped, but do not dump anything.
@li <b>compression</b>: string (default: "zstd;level=1") - Compression used when
writing the binary log dump files, one of: "none", "gzip", "zstd". Compression
level may be specified as "gzip;level=8" or "zstd;level=8".
@li <b>showProgress</b>: bool (default: true if stdout is a TTY device, false
otherwise) - Enable or disable dump progress information.

${TOPIC_REMOTE_STORAGE_OPTIONS}

<b>Details</b>

The starting point can be automatically determined from a previous dump (either
by reusing the same output directory created previously by
<b>util.<<<dumpBinlogs>>>()</b>, or by setting the <b>since</b> option to a
directory containing dump created either by <b>util.<<<dumpInstance>>>()</b> or
<b>util.<<<dumpBinlogs>>>()</b>) or as a explicitly specified binlog file and
position in the <b>startFrom</b> option.

Use <b>util.<<<loadBinlogs>>>()</b> to apply binary log dumps created with this
command.

The %Shell must be connected to the server from where binlogs will be dumped
from.

If directory specified by the <b>outputUrl</b> parameter does not exist, either
<b>since</b> or <b>startFrom</b> option must also be given.

If directory specified by the <b>outputUrl</b> parameter exists, neither
<b>since</b> nor <b>startFrom</b> options may be given, as they will be
automatically determined.

By default, the <b>since</b> option assumes the same storage location as
<b>outputUrl</b>:

@li if <b>outputUrl</b> is set to a prefix PAR, <b>since</b> option denotes a
path relative to this PAR
@li if any of the remote storages is used (i.e. <b>osBucketName</b> is set),
<b>since</b> option denotes a path in that remote storage

In order to use a different storage, prefix the option value with
<b>%file://</b> for a local storage, or use a PAR URL.
)*");

/**
 * \ingroup util
 *
 * $(UTIL_DUMPBINLOGS_BRIEF)
 *
 * $(UTIL_DUMPBINLOGS)
 */
#if DOXYGEN_JS
Undefined Util::dumpBinlogs(String outputUrl, Dictionary options);
#elif DOXYGEN_PY
None Util::dump_binlogs(str outputUrl, dict options);
#endif
void Util::dump_binlogs(
    const std::string &url,
    const shcore::Option_pack_ref<binlog::Dump_binlogs_options> &options) {
  Scoped_log_sql log_sql{log_sql_for_dump_and_load()};
  shcore::Log_sql_guard log_sql_context{"util.dumpBinlogs()"};

  const auto session = global_session();

  auto opts = *options;
  opts.set_url(url);
  opts.set_session(session);
  opts.validate_and_configure();

  binlog::Binlog_dumper dumper{opts};

  shcore::Interrupt_handler intr_handler(
      [&dumper]() {
        dumper.interrupt();
        return false;
      },
      [&dumper]() { dumper.async_interrupt(); });

  dumper.run();
}

REGISTER_HELP_FUNCTION(loadBinlogs, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_LOADBINLOGS, R"*(
Loads binary log dumps created by MySQL %Shell from a local or remote directory.

@param url URL to a local or remote directory containing dump created by
<b>util.<<<dumpBinlogs>>>()</b>.
@param options Optional dictionary with the load options.

The <b>url</b> parameter specifies the location of the dump to be loaded.
Allowed values:

${TOPIC_REMOTE_STORAGE_COMMON_PATHS}
@li <b>OCI Pre-Authenticated Request to a bucket or a prefix</b> - loads a dump
from OCI Object Storage using a PAR URL

${TOPIC_REMOTE_STORAGE_MORE_INFO}

The <b>options</b> dictionary supports the following options:
@li <b>ignoreVersion</b>: bool (default: false) - Load the dumps even if version
of the target instance is incompatible with version of the source instance where
the binary logs were dumped from.
@li <b>ignoreGtidGap</b>: bool (default: false) - Load the dumps even if the
current value of <b>gtid_executed</b> in the target instance does not fully
contain the starting value of <b>gtid_executed</b> of the first binary log file
to be loaded.
@li <b>stopBefore</b>: string (default: not set) - Stops the load right before
the specified binary log event is applied. Accepts a GTID:
<b>@<UUID@>[:tag]:@<transaction-id@></b>.
@li <b>stopAfter</b>: string (default: not set) - Stops the load right after the
specified binary log event is applied. Accepts a GTID:
<b>@<UUID@>[:tag]:@<transaction-id@></b>.

@li <b>dryRun</b>: bool (default: false) - Print information about what would be
loaded, but do not load anything.
@li <b>showProgress</b>: bool (default: true if stdout is a TTY device, false
otherwise) - Enable or disable dump progress information.

${TOPIC_REMOTE_STORAGE_OPTIONS}

<b>Details</b>

The %Shell must be connected to the server where binlogs will be loaded to.

The binary logs to be loaded are automatically selected based on the contents of
the dump and the current state of the target instance.
)*");

/**
 * \ingroup util
 *
 * $(UTIL_LOADBINLOGS_BRIEF)
 *
 * $(UTIL_LOADBINLOGS)
 */
#if DOXYGEN_JS
Undefined Util::loadBinlogs(String url, Dictionary options);
#elif DOXYGEN_PY
None Util::load_binlogs(str url, dict options);
#endif
void Util::load_binlogs(
    const std::string &url,
    const shcore::Option_pack_ref<binlog::Load_binlogs_options> &options) {
  Scoped_log_sql log_sql{log_sql_for_dump_and_load()};
  shcore::Log_sql_guard log_sql_context{"util.loadBinlogs()"};

  const auto session = global_session();

  auto opts = *options;
  opts.set_url(url);
  opts.set_session(session);
  opts.validate_and_configure();

  binlog::Binlog_loader loader{opts};

  shcore::Interrupt_handler intr_handler(
      [&loader]() {
        loader.interrupt();
        return false;
      },
      [&loader]() { loader.async_interrupt(); });

  loader.run();
}

}  // namespace mysqlsh

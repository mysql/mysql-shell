from contextlib import ExitStack
from enum import Enum
from http.server import HTTPServer
import json
import os
import os.path
import random
import re
import threading
import time
import urllib.parse

def md5_table(session, schema, table, where = "", partitions = []):
    columns = []
    has_pri = 0 != session.run_sql("SELECT COUNT(*) FROM information_schema.statistics WHERE index_name = 'PRIMARY' AND table_schema = ? AND table_name = ?", [schema, table]).fetch_one()[0]
    for c in session.run_sql("desc !.!", [schema, table]).fetch_all():
        quoted = quote_identifier(c[0])
        columns.append(f"VECTOR_TO_STRING({quoted})" if c[1].upper().startswith('VECTOR') else quoted)
    query = f"SELECT @crc := sha1(concat(@crc, sha1(concat_ws('#', {', '.join([f'convert({c} using binary)' for c in columns])})))),@cnt := @cnt + 1 as discard from !.!"
    params = [schema, table]
    if partitions:
        query += f" PARTITION ({', '.join(['!' for p in partitions])})"
        params += partitions
    if has_pri:
        query += " use index(PRIMARY)"
    if where:
        query += f" WHERE {where}"
    if not has_pri:
        query += f" ORDER BY {', '.join(columns)}"
    session.run_sql("SET @crc = ''")
    session.run_sql("SET @cnt = 0")
    session.run_sql(query, params)
    row = session.run_sql("select @crc, @cnt").fetch_one()
    return {"sha1": row[0], "count": row[1]}

def snapshot_account(session, auser, ahost):
    user = {}
    user["create"] = session.run_sql("SHOW CREATE USER ?@?", [auser, ahost]).fetch_one()[0]
    user["grants"] = []
    for row in session.run_sql("SHOW GRANTS FOR ?@?", [auser, ahost]).fetch_all():
        user["grants"].append(row[0])
    return user

def snapshot_accounts(session):
    accounts = {}
    for row in session.run_sql("SELECT user, host FROM mysql.user").fetch_all():
        user = snapshot_account(session, row[0], row[1])
        name = row[0] + "@" + row[1]
        accounts[name]=user
    return accounts

def snapshot_table_data(session, schema, table):
    # CHECKSUM TABLE returns different values for floating point values when they're loaded with LOAD DATA - BUG#31071891
    # CHECKSUM TABLE doesn't work well with JSON columns (5.7)
    # cksum = session.run_sql("CHECKSUM TABLE !.!", [schema, table]).fetch_one()[1]
    # count = session.run_sql("SELECT count(*) FROM !.!", [schema, table])).fetch_one()[0]
    # return {"checksum":cksum, "rowcount":count}
    return md5_table(session, schema, table)

def snapshot_triggers(session, schema, table):
    triggers = {}
    for row in session.run_sql("SELECT trigger_name FROM information_schema.triggers WHERE trigger_schema = ? and event_object_table = ?", [schema, table]).fetch_all():
        trigger = row[0]
        triggers[trigger] = session.run_sql("SHOW CREATE TRIGGER !.!", [schema, trigger]).fetch_one()[2]
    return triggers

def snapshot_tables_and_triggers(session, schema):
    tables = {}
    for row in session.run_sql("SELECT TABLE_NAME FROM information_schema.tables WHERE table_schema = ? and table_type = 'BASE TABLE'", [schema]).fetch_all():
        name = row[0]
        tables[name] = snapshot_table_data(session, schema, name)
        tables[name]["ddl"] = session.run_sql("SHOW CREATE TABLE !.!", [schema, name]).fetch_one()[1]
        # on Windows "SHOW CREATE" keeps adding slashes at the end of a DATA|INDEX directory, mitigate that
        tables[name]["ddl"] = re.compile(r"\/+").sub("/", tables[name]["ddl"])
        tables[name]["triggers"] = snapshot_triggers(session, schema, name)
    return tables

def snapshot_views(session, schema):
    tables = {}
    for row in session.run_sql("SELECT TABLE_NAME FROM information_schema.tables WHERE table_schema = ? and table_type = 'VIEW'", [schema]).fetch_all():
        name = row[0]
        tables[name] = {}
        tables[name]["ddl"] = session.run_sql("SHOW CREATE VIEW !.!", [schema, name]).fetch_one()[1]
    return tables

def snapshot_routines(session, schema, type):
    routines = {}
    for row in session.run_sql(f"SELECT ROUTINE_NAME FROM information_schema.routines WHERE routine_schema = ? and routine_type = '{type}'", [schema]).fetch_all():
        name = row[0]
        routines[name] = {}
        routines[name]["ddl"] = session.run_sql(f"SHOW CREATE {type} !.!", [schema, name]).fetch_one()[2]
    return routines

def snapshot_procedures(session, schema):
    return snapshot_routines(session, schema, "PROCEDURE")

def snapshot_functions(session, schema):
    return snapshot_routines(session, schema, "FUNCTION")

def snapshot_libraries(session, schema):
    libraries = {}
    if instance_supports_libraries:
        for row in session.run_sql("SELECT LIBRARY_NAME FROM information_schema.libraries WHERE LIBRARY_SCHEMA = ?", [schema]).fetch_all():
            name = row[0]
            libraries[name] = {}
            libraries[name]["ddl"] = session.run_sql("SHOW CREATE LIBRARY !.!", [schema, name]).fetch_one()[2]
    return libraries

def snapshot_events(session, schema):
    events = {}
    for row in session.run_sql("SHOW EVENTS IN !", [schema]).fetch_all():
        name = row.Name
        events[name] = {}
        events[name]["ddl"] = session.run_sql("SHOW CREATE EVENT !.!", [schema, name]).fetch_one()[3]
    return events

def snapshot_schema(session, schema):
    obj = {}
    obj["tables"] = snapshot_tables_and_triggers(session, schema)
    obj["views"] = snapshot_views(session, schema)
    obj["procedures"] = snapshot_procedures(session, schema)
    obj["functions"] = snapshot_functions(session, schema)
    obj["libraries"] = snapshot_libraries(session, schema)
    obj["events"] = snapshot_events(session, schema)
    return obj

def snapshot_schemas(session):
    schemas = {}
    for row in session.run_sql("SELECT SCHEMA_NAME FROM information_schema.schemata WHERE schema_name not in ('sys', 'information_schema', 'mysql', 'performance_schema')").fetch_all():
        name = row[0]
        schemas[name] = snapshot_schema(session, name)
        schemas[name]["ddl"] = session.run_sql("SHOW CREATE SCHEMA !", [name]).fetch_one()[1]
    return schemas

def snapshot_tablespaces(session):
    # not supported atm
    return {}

def snapshot_instance(session):
    snapshot = {}
    snapshot["accounts"] = snapshot_accounts(session)
    snapshot["tablespaces"] = snapshot_tablespaces(session)
    snapshot["schemas"] = snapshot_schemas(session)
    # normalize the JSON encoding
    return json.loads(json.dumps(snapshot))

def wipeout_server(session):
    session.run_sql("set foreign_key_checks=0")
    schemas = session.run_sql("show schemas").fetch_all()
    for schema, in schemas:
        if schema in ("mysql", "sys", "performance_schema", "information_schema"):
            continue
        session.run_sql("drop schema `"+schema+"`")
    for user,host in session.run_sql("select user,host from mysql.user").fetch_all():
        if user not in ["root", "mysql.infoschema", "mysql.sys", "mysql.session"]:
            session.run_sql("drop user ?@?", [user,host])
    session.run_sql("reset " + get_reset_binary_logs_keyword())
    session.run_sql("set global foreign_key_checks=1")

def truncate_all_tables(session):
    session.run_sql("set foreign_key_checks=0")
    schemas = session.run_sql("show schemas").fetch_all()
    for schema, in schemas:
        if schema in ("mysql", "sys", "performance_schema", "information_schema"):
            continue
        tables = session.run_sql("show full tables in `%s`" % schema).fetch_all()
        for table, t in tables:
            if t == "BASE TABLE":
                session.run_sql("truncate table `%s`.`%s`" % (schema, table))
    session.run_sql("set foreign_key_checks=1")

def compare_query_results(session1, session2, query, args=[], ignore_columns=[]):
    r1=list([list([str(f) for i,f in enumerate(r) if i not in ignore_columns]) for r in session1.run_sql(query, args).fetch_all()])
    r1.sort()
    r2=list([list([str(f) for i,f in enumerate(r) if i not in ignore_columns]) for r in session2.run_sql(query, args).fetch_all()])
    r2.sort()
    EXPECT_EQ(len(r1), len(r2), query + ", args: " + str(args))
    for i in range(len(r1)):
        EXPECT_EQ(str(r1[i]), str(r2[i]) if i < len(r2) else "<missing>", query + ", args: " + str(args) + ", row: " + str(i))
    return r1

def compare_schema(session1, session2, schema, check_rows=True, check_events=True, check_triggers=True, check_stored_procedures=True):
    tables = compare_query_results(session1, session2, "select table_name from information_schema.tables where table_schema=? and table_type='BASE TABLE' order by table_name", [schema])
    for table, in tables:
        compare_query_results(session1, session2, "show create table `"+schema+"`.`"+table+"`")
        if check_rows:
            EXPECT_EQ(md5_table(session1, schema, table), md5_table(session2, schema, table), quote_identifier(schema, table))
    views = compare_query_results(session1, session2, "select table_name from information_schema.tables where table_schema=? and table_type='VIEW' order by table_name", [schema])
    for view, in views:
        compare_query_results(session1, session2, "show create view `"+schema+"`.`"+view+"`")
    # ignore Originator in the show events output
    if check_events:
        compare_query_results(session1, session2, "show events in `"+schema+"`", ignore_columns=[11])
    if check_triggers:
        compare_query_results(session1, session2, "select TRIGGER_NAME,EVENT_MANIPULATION,EVENT_OBJECT_SCHEMA,EVENT_OBJECT_TABLE,ACTION_ORDER,ACTION_CONDITION,ACTION_STATEMENT,ACTION_ORIENTATION,ACTION_TIMING,ACTION_REFERENCE_OLD_TABLE,ACTION_REFERENCE_NEW_TABLE,ACTION_REFERENCE_OLD_ROW,ACTION_REFERENCE_NEW_ROW,SQL_MODE,DEFINER,CHARACTER_SET_CLIENT,COLLATION_CONNECTION,DATABASE_COLLATION from information_schema.triggers where trigger_schema=? order by trigger_name",[schema])
    if check_stored_procedures:
        compare_query_results(session1, session2, "select SPECIFIC_NAME,ROUTINE_SCHEMA,ROUTINE_NAME,ROUTINE_TYPE,DATA_TYPE,CHARACTER_MAXIMUM_LENGTH,CHARACTER_OCTET_LENGTH,NUMERIC_PRECISION,NUMERIC_SCALE,DATETIME_PRECISION,CHARACTER_SET_NAME,COLLATION_NAME,DTD_IDENTIFIER,ROUTINE_BODY,ROUTINE_DEFINITION,EXTERNAL_NAME,EXTERNAL_LANGUAGE,PARAMETER_STYLE,IS_DETERMINISTIC,SQL_DATA_ACCESS,SQL_PATH,SECURITY_TYPE,SQL_MODE,ROUTINE_COMMENT,DEFINER,CHARACTER_SET_CLIENT,COLLATION_CONNECTION,DATABASE_COLLATION from information_schema.routines where routine_schema=? order by specific_name", [schema])


def compare_schemas(session1, session2, check_rows=True, check_events=True, check_triggers=True, check_stored_procedures=True):
    schemas = compare_query_results(session1, session2, "show schemas")
    for schema, in schemas:
        if schema in ("mysql", "sys", "performance_schema", "information_schema"):
            continue
        compare_schema(session1, session2, schema, check_rows, check_events, check_triggers, check_stored_procedures)

def compare_user_grants(session1, session2, user):
    grants1 = session1.run_sql(f"show grants for {user}").fetch_all()
    grants2 = session2.run_sql(f"show grants for {user}").fetch_all()
    EXPECT_EQ(str(grants1), str(grants2), f"grants for {user}")


def compare_users(session1, session2):
    users = set()
    for s in [session1, session2]:
        for user, in s.run_sql("select concat(quote(user), '@', quote(host)) from mysql.user").fetch_all():
            users.add(user)
    for user in users:
        compare_user_grants(session1, session2, user)


def compare_servers(session1, session2, *, check_rows=True, check_users=True, check_events=True, check_triggers=True, check_stored_procedures=True):
    compare_schemas(session1, session2, check_rows, check_events, check_triggers, check_stored_procedures)
    if check_users:
        compare_users(session1, session2)

def CHECK_OUTPUT_SANITY(outdir, min_chunk_size, min_chunks, allow_min_chunk_size_errors=0):
    chunks_per_table = {}
    files = []
    print()
    print(outdir)
    for f in os.listdir(outdir):
        if not f.endswith(".tsv"):
            continue
        fsize = os.stat(os.path.join(outdir, f)).st_size
        print("%-30s\t%-10s"%(f, fsize))
        files.append((f, fsize))
        if f.count("@") >= 2:
            table = "@".join(f.split("@")[:2])
            if table not in chunks_per_table:
                chunks_per_table[table] = 1
            else:
                chunks_per_table[table] += 1
    min_chunk_size_errors = 0
    for f, fsize in files:
        # don't check the last file if there are many chunks
        if "@@" not in f or len(files) == 1:
            if fsize < min_chunk_size:
                min_chunk_size_errors += 1
            if min_chunk_size_errors > allow_min_chunk_size_errors:
                EXPECT_LE(min_chunk_size, fsize, "Size of "+f+" too small")
    for t, c in chunks_per_table.items():
        EXPECT_LE(min_chunks, c, "Too few chunks for "+t)

def get_test_user_name(name):
    return f"test_{name}"

def get_test_user_account(name):
    return f"'{get_test_user_name(name)}'@'{__host}'"

def get_user_account_for_output(name):
    return name if __version_num < 80000 else name.replace("'", "`")

def test_user_uri(port):
    return f"mysql://{get_test_user_name(test_user)}:{test_user_pwd}@{__host}:{port}"

if __os_type != "windows":
    def filename_for_file(filename):
        return filename
else:
    def filename_for_file(filename):
        return filename.replace("\\", "/")

if __os_type != "windows":
    def absolute_path_for_output(path):
        return os.path.abspath(path)
else:
    def absolute_path_for_output(path):
        long_path_prefix = r"\\?" "\\"
        return long_path_prefix + os.path.abspath(path)

class Compatibility_issue:
    def __init__(self, error, fixed, warning=None, note=None):
        self.__error_msg = "ERROR: SRC: " + error if error else ""
        self.__warning_msg = "WARNING: SRC: " + warning if warning else ""
        self.__fixed_msg = "NOTE: SRC: " + fixed if fixed else ""
        self.__note_msg = "NOTE: SRC: " + note if note else ""
    def error(self, copy_op=False):
        if self.__error_msg:
            return self.__remove_src(self.__error_msg, copy_op)
        else:
            raise Exception("No error message")
    def error_no_prefix(self):
        return self.error()[7:]
    def warning(self, copy_op=False):
        if self.__warning_msg:
            return self.__remove_src(self.__warning_msg, copy_op)
        else:
            raise Exception("No warning message")
    def warning_no_prefix(self):
        return self.warning()[9:]
    def fixed(self, copy_op=False):
        if self.__fixed_msg:
            return self.__remove_src(self.__fixed_msg, copy_op)
        else:
            raise Exception("No fixed message")
    def fixed_no_prefix(self):
        return self.fixed()[6:]
    def note(self, copy_op=False):
        if self.__note_msg:
            return self.__remove_src(self.__note_msg, copy_op)
        else:
            raise Exception("No note message")
    def note_no_prefix(self):
        return self.note()[6:]
    def __remove_src(self, msg, copy_op):
        return msg if copy_op else msg.replace("SRC: ", "")

def strip_restricted_grants(user, privileges):
    all_privileges = ", ".join(sorted(privileges))
    plural = "s" if len(privileges) > 1 else ""
    error = f"User {user} is granted restricted privilege{plural}: {all_privileges} (fix this with 'strip_restricted_grants' compatibility option)"
    fixed = f"User {user} had restricted privilege{plural} ({all_privileges}) removed"
    return Compatibility_issue(error, fixed)

def strip_restricted_grants_set_user_id_replaced(user):
    fixed = f"User {user} had restricted privilege SET_USER_ID replaced with SET_ANY_DEFINER"
    return Compatibility_issue(None, fixed)

def strip_invalid_grants(user, grant, object_type):
    error = f"User {user} has grant statement on a non-existent {object_type} ({grant}) (fix this with 'strip_invalid_grants' compatibility option)"
    fixed = f"User {user} had grant statement on a non-existent {object_type} removed ({grant})"
    return Compatibility_issue(error, fixed)

def comment_data_index_directory(schema, table):
    error = ""
    fixed = f"Table `{schema}`.`{table}` had {{DATA|INDEX}} DIRECTORY table option commented out"
    return Compatibility_issue(error, fixed)

def comment_encryption(schema, table):
    error = ""
    fixed = f"Table `{schema}`.`{table}` had ENCRYPTION table option commented out"
    return Compatibility_issue(error, fixed)

def comment_schema_encryption(schema):
    error = ""
    fixed = f"Database `{schema}` had unsupported ENCRYPTION option commented out"
    return Compatibility_issue(error, fixed)

def force_innodb_unsupported_storage(schema, table, engine = "MyISAM"):
    error = f"Table `{schema}`.`{table}` uses unsupported storage engine {engine} (fix this with 'force_innodb' compatibility option)"
    fixed = f"Table `{schema}`.`{table}` had unsupported engine {engine} changed to InnoDB"
    return Compatibility_issue(error, fixed)

def force_innodb_row_format_fixed(schema, table):
    error = f"Table `{schema}`.`{table}` uses unsupported ROW_FORMAT=FIXED option (fix this with 'force_innodb' compatibility option)"
    fixed = f"Table `{schema}`.`{table}` had unsupported ROW_FORMAT=FIXED option removed"
    return Compatibility_issue(error, fixed)

def strip_tablespaces(schema, table):
    error = f"Table `{schema}`.`{table}` uses unsupported tablespace option (fix this with 'strip_tablespaces' compatibility option)"
    fixed = f"Table `{schema}`.`{table}` had unsupported tablespace option removed"
    return Compatibility_issue(error, fixed)

def strip_definers_definer_clause(schema, name, label = "View", user = "`root`@`localhost`"):
    error = f"""{label} `{schema}`.`{name}` - definition uses DEFINER clause set to user {user.replace("'", "`").replace('"', "`")} which can only be executed by this user or a user with SET_ANY_DEFINER, SET_USER_ID or SUPER privileges (fix this with 'strip_definers' compatibility option)"""
    fixed = f"{label} `{schema}`.`{name}` had definer clause removed"
    return Compatibility_issue(error, fixed, error[:error.find(" (fix")])

def strip_definers_security_clause(schema, name, label = "View"):
    error = f"{label} `{schema}`.`{name}` - definition does not use SQL SECURITY INVOKER characteristic, which is mandatory when the DEFINER clause is omitted or removed (fix this with 'strip_definers' compatibility option)"
    fixed = f"{label} `{schema}`.`{name}` had SQL SECURITY characteristic set to INVOKER"
    return Compatibility_issue(error, fixed, error[:error.find(" (fix")])

def definer_clause_uses_restricted_user_name(schema, name, label = "View", user = "`root`@`localhost`"):
    error = f"""{label} `{schema}`.`{name}` - definition uses DEFINER clause set to user {user.replace("'", "`").replace('"', "`")} whose user name is restricted in MySQL HeatWave Service (this issue needs to be fixed manually)"""
    return Compatibility_issue(error, None)

def definer_clause_uses_unknown_account(schema, name, label = "View", user = "`root`@`localhost`"):
    warning = f"""{label} `{schema}`.`{name}` - definition uses DEFINER clause set to user {user.replace("'", "`").replace('"', "`")} which does not exist or is not included"""
    return Compatibility_issue(None, None, warning)

def definer_clause_uses_unknown_account_once():
    warning = "One or more DDL statements contain DEFINER clause but user information is not included in the dump. Loading will fail if accounts set as definers do not already exist in the target DB System instance."
    return Compatibility_issue(None, None, warning)

def skip_invalid_accounts_plugin(user, plugin):
    error = f"User {user} is using an unsupported authentication plugin '{plugin}' (fix this with 'skip_invalid_accounts' compatibility option)"
    fixed = f"User {user} is using an unsupported authentication plugin '{plugin}', this account has been removed from the dump"
    return Compatibility_issue(error, fixed)

def skip_invalid_accounts_no_password(user):
    error = f"User {user} does not have a password set (fix this with 'skip_invalid_accounts' compatibility option)"
    fixed = f"User {user} does not have a password set, this account has been removed from the dump"
    return Compatibility_issue(error, fixed)

def create_invisible_pks(schema, table):
    error = f"Table `{schema}`.`{table}` does not have a Primary Key, which is required for High Availability in MySQL HeatWave Service"
    fixed = f"Table `{schema}`.`{table}` does not have a Primary Key, this will be fixed when the dump is loaded"
    return Compatibility_issue(error, fixed)

def create_invisible_pks_name_conflict(schema, table):
    error = f"Table `{schema}`.`{table}` does not have a Primary Key, this cannot be fixed automatically because the table has a column named `my_row_id` (this issue needs to be fixed manually)"
    fixed = ""
    return Compatibility_issue(error, fixed)

def create_invisible_pks_auto_increment_conflict(schema, table):
    error = f"Table `{schema}`.`{table}` does not have a Primary Key, this cannot be fixed automatically because the table has a column with 'AUTO_INCREMENT' attribute (this issue needs to be fixed manually)"
    fixed = ""
    return Compatibility_issue(error, fixed)

def create_invisible_pks_partitioned_table(schema, table):
    error = f"Table `{schema}`.`{table}` does not have a Primary Key, this cannot be fixed automatically because the table is partitioned (this issue needs to be fixed manually)"
    fixed = ""
    return Compatibility_issue(error, fixed)

def ignore_missing_pks(schema, table):
    error = f"Table `{schema}`.`{table}` does not have a Primary Key, which is required for High Availability in MySQL HeatWave Service"
    fixed = f"Table `{schema}`.`{table}` does not have a Primary Key, this is ignored"
    return Compatibility_issue(error, fixed)

def too_many_columns(schema, table, columns):
    error = f"Table `{schema}`.`{table}` has {columns} columns, while the limit for the InnoDB engine is 1017 columns (this issue needs to be fixed manually)"
    fixed = ""
    return Compatibility_issue(error, fixed)

def ignore_wildcard_grants(user, grant):
    error = f"User {user} has a wildcard grant statement at the database level ({grant})"
    fixed = error + ", this issue is ignored"
    return Compatibility_issue(error, fixed)

def unescape_wildcard_grants(user, schema, fixed_schema = ""):
    warning = f"User {user} has a wildcard grant statement at the database level which is using escaped wildcard characters ({schema})"
    fixed = warning + ", schema name has been replaced with: " + fixed_schema
    return Compatibility_issue(None, fixed, warning)

def grant_on_excluded_object(user, grant):
    warning = f"User {user} has a grant statement on an object which is not included in the dump ({grant})"
    return Compatibility_issue(None, None, warning)

def deprecated_authentication_plugin(user, plugin):
    warning = f"User {user} is using a deprecated authentication plugin '{plugin}'"
    return Compatibility_issue(None, None, warning)

def deprecated_and_disabled_authentication_plugin(user):
    warning = f"User {user} is using a deprecated authentication plugin 'mysql_native_password' which is disabled by default"
    return Compatibility_issue(None, None, warning)

def view_references_excluded_table(schema, view, ref_schema, ref_table):
    warning = f"View `{schema}`.`{view}` references table/view `{ref_schema}`.`{ref_table}` which is not included in the dump"
    return Compatibility_issue(None, None, warning)

def view_references_invalid_table(schema, view, ref_schema, ref_table):
    warning = f"View `{schema}`.`{view}` references table/view `{ref_schema}`.`{ref_table}` which was not found using case-sensitive search"
    return Compatibility_issue(warning, None, warning)

def routine_references_excluded_library(label, schema, routine, library_schema, library_name):
    note = f"{label} `{schema}`.`{routine}` references library `{library_schema}`.`{library_name}` which is not included in the dump"
    return Compatibility_issue(None, None, None, note)

def routine_references_missing_library(label, schema, routine, library_schema, library_name):
    warning = f"{label} `{schema}`.`{routine}` references library `{library_schema}`.`{library_name}` which does not exist"
    return Compatibility_issue(None, None, warning)

def supports_set_any_definer_privilege(ver):
    return testutil.version_check(ver, ">=", "8.2.0")

def warn_target_version_does_not_support_libraries(ver):
    return f"The dump contains library DDL, however the 'targetVersion' option is set to {ver}, where this feature is not supported."

# messages reported by the loader
def routine_uses_missing_library(label, schema, routine, library_schema, library_name):
    return f"The {label} `{schema}`.`{routine}` uses library `{library_schema}`.`{library_name}` which does not exist."

def routine_skipped_due_to_missing_deps(label, schema, routine):
    return f"Skipping the {label} `{schema}`.`{routine}` due to missing dependencies."

def urlencode_object_name(s):
    ret = ""
    for c in s:
        if ord(c) <= 127:
            ret += urllib.parse.quote(c)
        else:
            ret += c
    return ret

def truncate_basename(basename):
    if len(basename) > 225:
        # there are no names which would create the same truncated basename,
        # always append "0" as ordinal number
        return basename[:225] + "0"
    else:
        return basename

# WL13807-FR8 - The base name of any schema-related file created during the dump must be in the form of `schema`, where:
# * schema - percent-encoded name of the schema.
# Only code points up to and including `U+007F` which are not Unreserved Characters (as specified in RFC3986) must be encoded, all remaining code points must not be encoded. If the length of base name exceeds `225` characters, it must be truncated to `225` characters and an ordinal number must be appended.
# WL13807-TSFR8_1
# WL13807-TSFR8_2
def encode_schema_basename(schema):
    return truncate_basename(urlencode_object_name(schema))

class Schema_ddl(Enum):
    Schema = 1
    Events = 2
    Libraries = 3
    Routines = 4

def schema_ddl_file(schema, ddl):
    names = {
        Schema_ddl.Schema: "",
        Schema_ddl.Events: ".events",
        Schema_ddl.Libraries: ".libraries",
        Schema_ddl.Routines: ".routines",
    }
    return encode_schema_basename(schema) + (names[ddl] if instance_supports_libraries or Schema_ddl.Libraries == ddl else "") + ".sql"

# WL13807-FR7.1 - The base name of any file created during the dump must be in the format `schema@table`, where:
# * `schema` - percent-encoded name of the schema which contains the table to be exported,
# * `table` - percent-encoded name of the table to be exported.
# Only code points up to and including `U+007F` which are not Unreserved Characters (as specified in RFC3986) must be encoded, all remaining code points must not be encoded. If the length of base name exceeds `225` characters, it must be truncated to `225` characters and an ordinal number must be appended.

def encode_table_basename(schema, table):
    return truncate_basename(urlencode_object_name(schema) + "@" + urlencode_object_name(table))

def encode_partition_basename(schema, table, partition):
    return truncate_basename(urlencode_object_name(schema) + "@" + urlencode_object_name(table) + "@" + urlencode_object_name(partition))

def count_files_with_basename(directory, basename):
    cnt = 0
    for f in os.listdir(directory):
        if f.startswith(basename):
            cnt += 1
    return cnt

def has_file_with_basename(directory, basename):
    return count_files_with_basename(directory, basename) > 0

def count_files_with_extension(directory, ext):
    cnt = 0
    for f in os.listdir(directory):
        if f.endswith(ext):
            cnt += 1
    return cnt

def validate_load_progress(file_name):
    with open(file_name) as fp:
        while True:
            line = fp.readline()
            if line:
                content = json.loads(line)
                EXPECT_TRUE("op" in content)
                EXPECT_TRUE("done" in content)
                EXPECT_TRUE("schema" in content or content["op"] in ["GTID-UPDATE", "SERVER-UUID"])
            else:
                break

def remove_file(name):
    try:
        os.remove(name)
    except:
        pass

def read_json(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)

def write_json(path, data):
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f)

def wipe_dir(path):
    try:
        testutil.rmdir(path, True)
    except:
        pass

def EXPECT_CAPABILITIES(path, capabilities):
    metadata = read_json(path)
    EXPECT_TRUE("capabilities" in metadata, "Metadata file should have 'capabilities' array")
    for expected in capabilities:
        actual = next(filter(lambda capability: expected["id"] == capability["id"], metadata["capabilities"]), None)
        EXPECT_TRUE(actual is not None, f"Metadata should have '{expected['id']}' capability")
        if actual:
            EXPECT_EQ(expected["id"], actual["id"], "IDs of the capability should match")
            EXPECT_EQ(expected["description"], actual["description"], "Descriptions of the capability should match")
            EXPECT_EQ(expected["versionRequired"], actual["versionRequired"], "Version required of the capability should match")

def EXPECT_NO_CAPABILITIES(path, capabilities):
    metadata = read_json(path)
    EXPECT_TRUE("capabilities" in metadata, "Metadata file should have 'capabilities' array")
    for expected in capabilities:
        actual = next(filter(lambda capability: expected["id"] == capability["id"], metadata["capabilities"]), None)
        EXPECT_FALSE(actual is not None, f"Metadata should NOT have '{expected['id']}' capability")

def quote_identifier(schema, object = None):
    def quote(identifier):
        return f"`{identifier.replace('`', '``')}`"
    ret = quote(schema)
    if object is not None:
        ret += "." + quote(object)
    return ret

def count_rows(schema, table):
    return session.run_sql("SELECT COUNT(*) FROM !.!", [ schema, table ]).fetch_one()[0]

def checksum_file_path(dir):
    return os.path.join(dir, "@.checksums.json")

def backup_file(path):
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
        stack.callback(lambda: os.remove(path) if os.path.isfile(path) else None)
    return stack

def create_test_table(sess, schema, table, create_definition, nrows = 1000):
    sess.run_sql("CREATE SCHEMA IF NOT EXISTS !", [ schema ])
    sess.run_sql(f"DROP TABLE IF EXISTS !.!", [ schema, table ])
    sess.run_sql(f"CREATE TABLE !.! {create_definition}", [ schema, table ])
    columns = {}
    next_int = 0
    def pk():
        nonlocal next_int
        next_int = next_int + 1
        return str(next_int)
    def random_int_string(n):
        return ''.join(random.choices("123456789", k = n))
    for c in sess.run_sql("SELECT COLUMN_NAME, COLUMN_TYPE, COLUMN_KEY='PRI' FROM information_schema.columns WHERE TABLE_SCHEMA=? AND TABLE_NAME=? AND EXTRA NOT LIKE '% GENERATED'", [ schema, table ]).fetch_all():
        fun = None
        if c[2]:
            if c[1].endswith("int") or "int(" in c[1]:
                fun = pk
            else:
                raise Exception("Only integer PKs are supported")
        else:
            if c[1].endswith("int") or "int(" in c[1]:
                fun = lambda: str(random.randint(-100, 100))
            elif c[1].startswith("varchar(") or c[1].startswith("char(") or c[1].startswith("varbinary(") or c[1].startswith("binary("):
                count = int(re.findall(r'\d+', c[1])[0])
                if c[1][0] == "v":
                    fun = lambda n=count: f"'{random_string(1, n)}'"
                else:
                    fun = lambda n=count: f"'{random_string(n)}'"
            elif c[1] == "double" or c[1] == "float":
                fun = lambda: str(random.random())
            elif c[1].startswith("decimal("):
                count = re.findall(r'(\d+),(\d+)', c[1])[0]
                fun = lambda x=int(count[0]),y=int(count[1]): f"'{random_int_string(x - y)}.{random_int_string(y)}'"
            elif c[1] == "datetime":
                fun = lambda: "now()"
            elif c[1] == "date":
                fun = lambda: "date(now())"
            elif c[1] == "time":
                fun = lambda: "time(now())"
            elif c[1].endswith("blob") or c[1].endswith("text"):
                fun = lambda: f"'{random_string(100)}'"
            elif c[1] == "json":
                fun = lambda: f'\'{{"key": "{random_string(20)}", "value": "{random_string(100)}"}}\''
            else:
                raise Exception(f"Unsupported column type: {c[1]}")
        columns[c[0]] = fun
    sql = f"INSERT INTO !.! ({','.join(columns.keys())}) VALUES {','.join([f'''({','.join([f() for f in columns.values()])})''' for i in range(nrows)])}"
    sess.run_sql(sql, [ schema, table ])
    sess.run_sql("ANALYZE TABLE !.!", [ schema, table ])

def grant_privilege(sess, privilege, level, user = f"`{__user}`@`{__host}`"):
    sess.run_sql(f"GRANT {privilege} ON {level} TO {user}")

def revoke_privilege(sess, privilege, level, user = f"`{__user}`@`{__host}`"):
    sess.run_sql(f"REVOKE {privilege} ON {level} FROM {user}")

def enable_bulk_load(sess):
    try:
        sess.run_sql("INSTALL COMPONENT 'file://component_bulk_loader'")
        return True
    except Exception as e:
        print(e)
        return False

def disable_bulk_load(sess):
    try:
        sess.run_sql("UNINSTALL COMPONENT 'file://component_bulk_loader'")
    except:
        pass

def enable_mle(sess):
    try:
        sess.run_sql("INSTALL COMPONENT 'file://component_mle'")
        return True
    except Exception as e:
        print(e)
        return False

def disable_mle(sess):
    try:
        sess.run_sql("UNINSTALL COMPONENT 'file://component_mle'")
    except:
        pass

def set_env_var(name, value):
    stack = ExitStack()
    if name in os.environ:
        old_value = os.environ[name]
        stack.callback(lambda: os.environ.update({name: old_value}))
    os.environ[name] = value
    stack.callback(lambda: os.environ.pop(name, None))
    return stack

def set_global_variable(name, default_value):
    value = globals().get(name)
    if value is None:
        globals()[name] = default_value

def del_global_variable(name):
    value = globals().get(name)
    if value is not None:
        del globals()[name]

def extract_import_table_code():
    output = testutil.fetch_captured_stdout(False)
    index = output.find("util.import_table")
    if -1 == index:
        raise Exception("Captured output does not contain the import table code")
    return output[index:]

class Http_server(HTTPServer):
    allow_reuse_address = True

class Http_test_server:
    def __init__(self, handler):
        self.__handler = handler
        self.__port = self.__server = self.__thread = None
    def start(self, no_proxy=False):
        while True:
            self.__port = random.randint(50000, 60000)
            try:
                self.__server = Http_server(("127.0.0.1", self.__port), self.__handler)
                break
            except OSError as e:
                # try again in case of EADDRINUSE
                if 98 != e.errno:
                    raise
        self.__thread = threading.Thread(target = self.__server.serve_forever)
        self.__thread.start()
        if no_proxy:
            self.__stack = set_env_var("no_proxy", "*")
    def stop(self):
        self.__server.shutdown()
        self.__server.server_close()
        self.__thread.join()
        if hasattr(self, "__stack"):
            self.__stack.close()
    def url(self):
        return f"http://127.0.0.1:{self.__port}"

class Generate_transactions:
    def __init__(self, uri, schema, table, tables_to_create=500):
        self.__uri = uri
        self.__schema = schema
        self.__table = table
        self.__tables_to_create = tables_to_create
        self.__session = self.__pid = None
    def generate_ddl(self):
        self.__setup(f"""v = {self.__tables_to_create}
while True:
    session.run_sql("CREATE TABLE {self.__schema}.{self.__table}_{{0}} (a int)".format(v))
    v = v + 1
session.close()
""", f"SELECT 1 FROM information_schema.tables WHERE table_schema='{self.__schema}' and table_name='{self.__table}_{self.__tables_to_create}'")
    def generate_data(self):
        self.__setup(f"""v = 0
while True:
    session.run_sql("INSERT INTO {self.__schema}.{self.__table} VALUES ({{0}})".format(v))
    v = v + 1
session.close()
""", f"SELECT 1 FROM (SELECT COUNT(*) AS c FROM {self.__schema}.{self.__table}) AS s WHERE s.c > 10")
    def stop(self, drop_schema=True):
        # terminate the process immediately
        testutil.wait_mysqlsh_async(self.__pid, 0)
        if drop_schema:
            self.__cleanup()
        self.__session.close()
        self.__session = self.__pid = None
    def __setup(self, async_script, wait_condition):
        self.__session = shell.open_session(self.__uri)
        self.__cleanup()
        self.__session.run_sql(f"CREATE SCHEMA {self.__schema}")
        self.__session.run_sql(f"CREATE TABLE {self.__schema}.{self.__table} (data INT PRIMARY KEY)")
        self.__session.run_sql(f"INSERT INTO {self.__schema}.{self.__table} VALUES {','.join([f'(-{i+1})' for i in range(10)])}")
        self.__session.run_sql(f"ANALYZE TABLE {self.__schema}.{self.__table}")
        for i in range(self.__tables_to_create):
            self.__session.run_sql(f"CREATE TABLE {self.__schema}.{self.__table}_{i} (data INT PRIMARY KEY)")
        # run a process
        self.__pid = testutil.call_mysqlsh_async(["--py", self.__uri], async_script)
        # wait for process to start
        time.sleep(1)
        while not session.run_sql(wait_condition).fetch_one():
            time.sleep(0.1)
    def __cleanup(self):
        self.__session.run_sql(f"DROP SCHEMA IF EXISTS {self.__schema}")

# constants
test_user = "sample_user"
test_user_account = get_test_user_account(test_user)
test_user_pwd = "p4$$"

instance_supports_libraries = __version_num >= 90200

partition_awareness_capability = {
    "id": "partition_awareness",
    "description": "Partition awareness - dumper treats each partition as a separate table, improving both dump and load times.",
    "versionRequired": "8.0.27",
}

multifile_schema_ddl_capability = {
    "id": "multifile_schema_ddl",
    "description": "Multifile schema DDL - schema DDL is split into multiple files, containing DDL for schema itself, events, routines and libraries.",
    "versionRequired": "9.3.0",
}

dump_dir_redirection_capability = {
    "id": "dump_dir_redirection",
    "description": "Dump directory redirection - output directory contains a link to another directory, where the standalone dump is stored.",
    "versionRequired": "9.4.1",
}

innodb_vector_store_capability = {
    "id": "innodb_vector_store",
    "description": "InnoDB vector store - dump contains InnoDB-based vector store tables.",
    "versionRequired": "9.4.1",
}

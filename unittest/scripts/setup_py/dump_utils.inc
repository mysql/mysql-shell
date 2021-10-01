import json
import re
import urllib.parse

def md5_table(session, schema, table):
    columns = ""
    has_pri = 0 != session.run_sql("SELECT COUNT(*) FROM information_schema.statistics WHERE index_name = 'PRIMARY' AND table_schema = ? AND table_name = ?", [schema, table]).fetch_one()[0]
    for c in session.run_sql("desc !.!", [schema, table]).fetch_all():
        columns += f", convert(`{c[0]}` using binary)"
    query = f"SELECT min(least(length(@crc := sha1(concat(@crc, sha1(concat_ws('#'{columns}))))),@cnt := @cnt + 1)) as discard from !.!"
    if has_pri:
        query += " use index(PRIMARY)"
    session.run_sql("SET @crc = ''")
    session.run_sql("SET @cnt = 0")
    session.run_sql(query, [schema, table])
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
    # cksum = session.run_sql("CHECKSUM TABLE !.!", [schema, table]).fetch_one()[1]
    # count = session.run_sql("SELECT count(*) FROM !.!", [schema, table])).fetch_one()[0]
    # return {"checksum":cksum, "rowcount":count}
    return md5_table(session, schema, table)

def snapshot_tables_and_triggers(session, schema):
    tables = {}
    for row in session.run_sql("SELECT TABLE_NAME FROM information_schema.tables WHERE table_schema = ? and table_type = 'BASE TABLE'", [schema]).fetch_all():
        name = row[0]
        tables[name] = snapshot_table_data(session, schema, name)
        tables[name]["ddl"] = session.run_sql("SHOW CREATE TABLE !.!", [schema, name]).fetch_one()[1]
        # on Windows "SHOW CREATE" keeps adding slashes at the end of a DATA|INDEX directory, mitigate that
        tables[name]["ddl"] = re.compile(r"\/+").sub("/", tables[name]["ddl"])
        triggers = {}
        for row in session.run_sql("SELECT trigger_name FROM information_schema.triggers WHERE trigger_schema = ? and event_object_table = ?", [schema, name]).fetch_all():
            trigger = row[0]
            triggers[trigger] = session.run_sql("SHOW CREATE TRIGGER !.!", [schema, trigger]).fetch_one()[2]
        tables[name]["triggers"] = triggers
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

def snapshot_events(session, schema):
    events = {}
    for row in session.run_sql("SHOW EVENTS IN !", [schema]).fetch_all():
        name = row.name
        events[name] = {}
        events[name]["ddl"] = session.run_sql("SHOW CREATE EVENT !.!", [schema, name]).fetch_one()[3]
    return events

def snapshot_schema(session, schema):
    obj = {}
    obj["tables"] = snapshot_tables_and_triggers(session, schema)
    obj["views"] = snapshot_views(session, schema)
    obj["procedures"] = snapshot_procedures(session, schema)
    obj["functions"] = snapshot_functions(session, schema)
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
    session.run_sql("reset master")
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
        EXPECT_EQ(str(r1[i]), str(r2[i]) if i < len(r2) else "<missing>")
    return r1

def compare_schema(session1, session2, schema, check_rows=True):
    tables = compare_query_results(session1, session2, "select table_name from information_schema.tables where table_schema=? and table_type='BASE TABLE' order by table_name", [schema])
    for table, in tables:
        compare_query_results(session1, session2, "show create table `"+schema+"`.`"+table+"`")
        if check_rows:
            columns = [c[0] for c in session1.run_sql("SELECT COLUMN_NAME FROM information_schema.columns WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ? ORDER BY ORDINAL_POSITION;", [schema, table]).fetch_all()]
            compare_query_results(session1, session2, "select {0} from !.!".format(("MD5(!)," * len(columns))[:-1]), columns + [schema, table])
    views = compare_query_results(session1, session2, "select table_name from information_schema.tables where table_schema=? and table_type='VIEW' order by table_name", [schema])
    for view, in views:
        compare_query_results(session1, session2, "show create view `"+schema+"`.`"+view+"`")
    # ignore Originator in the show events output
    compare_query_results(session1, session2, "show events in `"+schema+"`", ignore_columns=[11])
    compare_query_results(session1, session2, "select TRIGGER_NAME,EVENT_MANIPULATION,EVENT_OBJECT_SCHEMA,EVENT_OBJECT_TABLE,ACTION_ORDER,ACTION_CONDITION,ACTION_STATEMENT,ACTION_ORIENTATION,ACTION_TIMING,ACTION_REFERENCE_OLD_TABLE,ACTION_REFERENCE_NEW_TABLE,ACTION_REFERENCE_OLD_ROW,ACTION_REFERENCE_NEW_ROW,SQL_MODE,DEFINER,CHARACTER_SET_CLIENT,COLLATION_CONNECTION,DATABASE_COLLATION from information_schema.triggers where trigger_schema=? order by trigger_name",[schema])
    compare_query_results(session1, session2, "select SPECIFIC_NAME,ROUTINE_SCHEMA,ROUTINE_NAME,ROUTINE_TYPE,DATA_TYPE,CHARACTER_MAXIMUM_LENGTH,CHARACTER_OCTET_LENGTH,NUMERIC_PRECISION,NUMERIC_SCALE,DATETIME_PRECISION,CHARACTER_SET_NAME,COLLATION_NAME,DTD_IDENTIFIER,ROUTINE_BODY,ROUTINE_DEFINITION,EXTERNAL_NAME,EXTERNAL_LANGUAGE,PARAMETER_STYLE,IS_DETERMINISTIC,SQL_DATA_ACCESS,SQL_PATH,SECURITY_TYPE,SQL_MODE,ROUTINE_COMMENT,DEFINER,CHARACTER_SET_CLIENT,COLLATION_CONNECTION,DATABASE_COLLATION from information_schema.routines where routine_schema=? order by specific_name", [schema])


def compare_schemas(session1, session2, check_rows=True):
    schemas = compare_query_results(session1, session2, "show schemas")
    for schema, in schemas:
        if schema in ("mysql", "sys", "performance_schema", "information_schema"):
            continue
        compare_schema(session1, session2, schema, check_rows)

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


def compare_servers(session1, session2, *, check_rows=True, check_users=True):
    compare_schemas(session1, session2, check_rows)
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

def test_user_uri(port):
    return f"mysql://{get_test_user_name(test_user)}:{test_user_pwd}@{__host}:{port}"

class Compatibility_issue:
    def __init__(self, error, fixed):
        self.__error_msg = "ERROR: " + error if error else ""
        self.__fixed_msg = "NOTE: " + fixed if fixed else ""
    def error(self):
        if self.__error_msg:
            return self.__error_msg
        else:
            raise Exception("No error message")
    def fixed(self):
        if self.__fixed_msg:
            return self.__fixed_msg
        else:
            raise Exception("No fixed message")

def strip_restricted_grants(user, privileges):
    all_privileges = ", ".join(sorted(privileges))
    plural = "s" if len(privileges) > 1 else ""
    error = f"User {user} is granted restricted privilege{plural}: {all_privileges} (fix this with 'strip_restricted_grants' compatibility option)"
    fixed = f"User {user} had restricted privilege{plural} ({all_privileges}) removed"
    return Compatibility_issue(error, fixed)

def comment_data_index_directory(schema, table):
    error = ""
    fixed = f"Table '{schema}'.'{table}' had {{DATA|INDEX}} DIRECTORY table option commented out"
    return Compatibility_issue(error, fixed)

def comment_encryption(schema, table):
    error = ""
    fixed = f"Table '{schema}'.'{table}' had ENCRYPTION table option commented out"
    return Compatibility_issue(error, fixed)

def force_innodb_unsupported_storage(schema, table, engine = "MyISAM"):
    error = f"Table '{schema}'.'{table}' uses unsupported storage engine {engine} (fix this with 'force_innodb' compatibility option)"
    fixed = f"Table '{schema}'.'{table}' had unsupported engine {engine} changed to InnoDB"
    return Compatibility_issue(error, fixed)

def force_innodb_row_format_fixed(schema, table):
    error = f"Table '{schema}'.'{table}' uses unsupported ROW_FORMAT=FIXED option (fix this with 'force_innodb' compatibility option)"
    fixed = f"Table '{schema}'.'{table}' had unsupported ROW_FORMAT=FIXED option removed"
    return Compatibility_issue(error, fixed)

def strip_tablespaces(schema, table):
    error = f"Table '{schema}'.'{table}' uses unsupported tablespace option (fix this with 'strip_tablespaces' compatibility option)"
    fixed = f"Table '{schema}'.'{table}' had unsupported tablespace option removed"
    return Compatibility_issue(error, fixed)

def strip_definers_definer_clause(schema, name, label = "View"):
    error = f"{label} {schema}.{name} - definition uses DEFINER clause set to user `root`@`localhost` which can only be executed by this user or a user with SET_USER_ID or SUPER privileges (fix this with 'strip_definers' compatibility option)"
    fixed = f"{label} {schema}.{name} had definer clause removed"
    return Compatibility_issue(error, fixed)

def strip_definers_security_clause(schema, name, label = "View"):
    error = f"{label} {schema}.{name} - definition does not use SQL SECURITY INVOKER characteristic, which is required (fix this with 'strip_definers' compatibility option)"
    fixed = f"{label} {schema}.{name} had SQL SECURITY characteristic set to INVOKER"
    return Compatibility_issue(error, fixed)

def skip_invalid_accounts_plugin(user, plugin):
    error = f"User {user} is using an unsupported authentication plugin '{plugin}' (fix this with 'skip_invalid_accounts' compatibility option)"
    fixed = f"User {user} is using an unsupported authentication plugin '{plugin}', this account has been removed from the dump"
    return Compatibility_issue(error, fixed)

def skip_invalid_accounts_no_password(user):
    error = f"User {user} does not have a password set (fix this with 'skip_invalid_accounts' compatibility option)"
    fixed = f"User {user} does not have a password set, this account has been removed from the dump"
    return Compatibility_issue(error, fixed)

def create_invisible_pks(schema, table):
    error = f"Table '{schema}'.'{table}' does not have a Primary Key, which is required for High Availability in MDS"
    fixed = f"Table '{schema}'.'{table}' does not have a Primary Key, this will be fixed when the dump is loaded"
    return Compatibility_issue(error, fixed)

def create_invisible_pks_name_conflict(schema, table):
    error = f"Table '{schema}'.'{table}' does not have a Primary Key, this cannot be fixed automatically because the table has a column named `my_row_id` (this issue needs to be fixed manually)"
    fixed = ""
    return Compatibility_issue(error, fixed)

def create_invisible_pks_auto_increment_conflict(schema, table):
    error = f"Table '{schema}'.'{table}' does not have a Primary Key, this cannot be fixed automatically because the table has a column with 'AUTO_INCREMENT' attribute (this issue needs to be fixed manually)"
    fixed = ""
    return Compatibility_issue(error, fixed)

def ignore_missing_pks(schema, table):
    error = f"Table '{schema}'.'{table}' does not have a Primary Key, which is required for High Availability in MDS"
    fixed = f"Table '{schema}'.'{table}' does not have a Primary Key, this is ignored"
    return Compatibility_issue(error, fixed)

def too_many_columns(schema, table, columns):
    error = f"Table '{schema}'.'{table}' has {columns} columns, while the limit for the InnoDB engine is 1017 columns (this issue needs to be fixed manually)"
    fixed = ""
    return Compatibility_issue(error, fixed)

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

# constants
test_user = "sample_user"
test_user_account = get_test_user_account(test_user)
test_user_pwd = "p4$$"

partition_awareness_capability = {
    "id": "partition_awareness",
    "description": "Partition awareness - dumper treats each partition as a separate table, improving both dump and load times.",
    "versionRequired": "8.0.27",
}

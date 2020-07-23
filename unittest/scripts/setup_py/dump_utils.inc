

def wipeout_server(session):
    schemas = session.run_sql("show schemas").fetch_all()
    for schema, in schemas:
        if schema in ("mysql", "sys", "performance_schema", "information_schema"):
            continue
        session.run_sql("drop schema "+schema)
    for user,host in session.run_sql("select user,host from mysql.user").fetch_all():
        if user not in ["root", "mysql.infoschema", "mysql.sys", "mysql.session"]:
            session.run_sql("drop user ?@?", [user,host])
    session.run_sql("reset master")

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

def compare_query_results(session1, session2, query, args=[]):
    r1=session1.run_sql(query, args).fetch_all()
    r2=session2.run_sql(query, args).fetch_all()
    EXPECT_EQ(len(r1), len(r2), query)
    for i in range(len(r1)):
        EXPECT_EQ(str(r1[i]), str(r2[i]))
    return r1

def compare_schema(session1, session2, schema, check_rows=True):
    tables = compare_query_results(session1, session2, "select table_name from information_schema.tables where table_schema=? and table_type='BASE TABLE' order by table_name", [schema])
    for table, in tables:
        compare_query_results(session1, session2, "show create table "+schema+"."+table)
        if check_rows:
            columns = [c[0] for c in session1.run_sql("SELECT COLUMN_NAME FROM information_schema.columns WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ? ORDER BY ORDINAL_POSITION;", [schema, table]).fetch_all()]
            compare_query_results(session1, session2, "select {0} from !.! use index (primary)".format(("MD5(!)," * len(columns))[:-1]), columns + [schema, table])
    views = compare_query_results(session1, session2, "select table_name from information_schema.tables where table_schema=? and table_type='VIEW' order by table_name", [schema])
    for view, in views:
        compare_query_results(session1, session2, "show create view "+schema+"."+view)
    compare_query_results(session1, session2, "show events in "+schema)
    compare_query_results(session1, session2, "select TRIGGER_NAME,EVENT_MANIPULATION,EVENT_OBJECT_SCHEMA,EVENT_OBJECT_TABLE,ACTION_ORDER,ACTION_CONDITION,ACTION_STATEMENT,ACTION_ORIENTATION,ACTION_TIMING,ACTION_REFERENCE_OLD_TABLE,ACTION_REFERENCE_NEW_TABLE,ACTION_REFERENCE_OLD_ROW,ACTION_REFERENCE_NEW_ROW,SQL_MODE,DEFINER,CHARACTER_SET_CLIENT,COLLATION_CONNECTION,DATABASE_COLLATION from information_schema.triggers where trigger_schema=? order by trigger_name",[schema])
    compare_query_results(session1, session2, "select SPECIFIC_NAME,ROUTINE_SCHEMA,ROUTINE_NAME,ROUTINE_TYPE,DATA_TYPE,CHARACTER_MAXIMUM_LENGTH,CHARACTER_OCTET_LENGTH,NUMERIC_PRECISION,NUMERIC_SCALE,DATETIME_PRECISION,CHARACTER_SET_NAME,COLLATION_NAME,DTD_IDENTIFIER,ROUTINE_BODY,ROUTINE_DEFINITION,EXTERNAL_NAME,EXTERNAL_LANGUAGE,PARAMETER_STYLE,IS_DETERMINISTIC,SQL_DATA_ACCESS,SQL_PATH,SECURITY_TYPE,SQL_MODE,ROUTINE_COMMENT,DEFINER,CHARACTER_SET_CLIENT,COLLATION_CONNECTION,DATABASE_COLLATION from information_schema.routines where routine_schema=? order by specific_name", [schema])


def compare_schemas(session1, session2, check_rows=True):
    schemas = compare_query_results(session1, session2, "show schemas")
    for schema, in schemas:
        if schema in ("mysql", "sys", "performance_schema", "information_schema"):
            continue
        compare_schema(session1, session2, schema, check_rows)


def compare_servers(session1, session2, check_rows=True):
    compare_schemas(session1, session2, check_rows)

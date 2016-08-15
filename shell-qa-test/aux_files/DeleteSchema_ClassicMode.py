session.run_sql("DROP DATABASE IF EXISTS schema_test;")
session.run_sql("CREATE SCHEMA schema_test;")
session.get_schemas()

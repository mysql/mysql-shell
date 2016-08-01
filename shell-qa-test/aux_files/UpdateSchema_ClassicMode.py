session.run_sql("drop database if exists schema_test;")
session.run_sql("CREATE SCHEMA schema_test;")
session.run_sql("ALTER SCHEMA schema_test  DEFAULT COLLATE utf8_general_ci;")
session.run_sql("SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = 'schema_test' LIMIT 1;")

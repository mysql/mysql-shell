session.runSql("drop database if exists schema_test;")
session.runSql("CREATE SCHEMA schema_test;")
session.runSql("ALTER SCHEMA schema_test  DEFAULT COLLATE utf8_general_ci;")
session.runSql("SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = 'schema_test' LIMIT 1;")

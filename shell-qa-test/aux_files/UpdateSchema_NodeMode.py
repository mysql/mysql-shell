import mysqlx
session=mysqlx.getNodeSession('root:guidev!@localhost:33060')
session.sql("drop database if exists schema_test;").execute()

session.sql("CREATE SCHEMA schema_test;").execute()
session.sql("ALTER SCHEMA schema_test  DEFAULT COLLATE utf8_general_ci ;").execute()
session.sql("SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = 'schema_test' LIMIT 1;").execute()
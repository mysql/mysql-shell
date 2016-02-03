session.sql("DROP DATABASE IF EXISTS schema_test;").execute();

session.sql("CREATE SCHEMA schema_test;").execute();
session.getSchemas();

session.dropSchema('schema_test');

session.getSchemas();

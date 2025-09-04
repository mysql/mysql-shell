#@<> Setup
test_schema = 'foreign_test_db'
shell.connect(__mysql_uri)
session.run_sql(f"drop schema if exists {test_schema}")
session.run_sql(f"create schema {test_schema}")
session.run_sql(f"use {test_schema}")

#@ BUG38194922 false positive {VER(<9.0.0)}
session.run_sql("""
CREATE TABLE test_table_a (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `test_column` int NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `test_uniq_key` (`test_column`),
  KEY `test_key` (`test_column`)
);
""")
session.run_sql("""
CREATE TABLE test_table_b (
  `other_column` int NOT NULL DEFAULT '1',
  PRIMARY KEY (`other_column`),
  CONSTRAINT `test_constraint` FOREIGN KEY (`other_column`) REFERENCES `test_table_a` (`test_column`)
);""")

util.check_for_server_upgrade(None, {"include": ["foreignKeyReferences"]})

#@<> BUG38194922 false positive - cleanup {VER(<9.0.0)}
session.run_sql("drop table if exists test_table_b")
session.run_sql("drop table if exists test_table_a")

#@ BUG38194922 positive check {VER(<8.0.0)}
session.run_sql("""
CREATE TABLE test_table_a (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `test_column` int NOT NULL,
  `next_column` int NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `test_uniq_key` (`test_column`),
  KEY `test_key` (`next_column`)
);
""")
session.run_sql("""
CREATE TABLE test_table_b (
  `other_column` int NOT NULL DEFAULT '1',
  PRIMARY KEY `test_one_key` (`other_column`),
  CONSTRAINT `test_constraint_one` FOREIGN KEY `test_fore_one_key` (`other_column`) REFERENCES `test_table_a` (`test_column`)
);""")
session.run_sql("""
CREATE TABLE test_table_c (
  `other_column` int NOT NULL DEFAULT '1',
  PRIMARY KEY `test_two_key` (`other_column`),
  CONSTRAINT `test_constraint_two` FOREIGN KEY `test_fore_two_key` (`other_column`) REFERENCES `test_table_a` (`next_column`)
);""")

util.check_for_server_upgrade(None, {"include": ["foreignKeyReferences"]})

#@<> BUG38194922 positive check - cleanup {VER(<8.0.0)}
session.run_sql("drop table if exists test_table_c")
session.run_sql("drop table if exists test_table_b")
session.run_sql("drop table if exists test_table_a")

#@<> Cleanup
session.run_sql(f"drop schema {test_schema}")

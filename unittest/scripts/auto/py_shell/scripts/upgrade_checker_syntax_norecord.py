#@{VER(<8.0.0)}

#@<> Setup
shell.connect(__mysql_uri)
session.run_sql("""
INSERT INTO mysql.proc VALUES (
  'upgrade_issues_ex',
  'orphaned_procedure',
  'PROCEDURE',
  'orphaned_procedure',
  'SQL',
  'CONTAINS_SQL',
  'NO',
  'DEFINER',
  '',
  '',
  _binary 'begin\nselect count(*) from somedb.sometable;\nend',
  'root@localhost',
  '2022-11-23 11:46:34',
  '2022-11-23 11:46:34',
  'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION',
  '',
  'utf8mb4',
  'utf8mb4_general_ci',
  'latin1_swedish_ci',
  _binary 'begin\nselect count(*) from somedb.sometable;\nend');
""")



#@<> Run upgrade checker to ensure the syntax check no longer fails with an orphaned routine
util.check_for_server_upgrade(None, {"include":["orphanedObjects", "syntax"]})

#@<> Teardown
session.run_sql("DELETE FROM mysql.proc WHERE db='upgrade_issues_ex' AND name='orphaned_procedure';")
session.close()
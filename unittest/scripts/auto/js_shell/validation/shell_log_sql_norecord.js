// =================================
// Using 'error' logging option
// =================================

\option logSql = "error"

//@ Testing error sql logging with \use
||MySQL Error 1049: Unknown database 'unexisting'

//@ Testing error sql logging with \sql
||ERROR: 1064 (42000): You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near 'whatever' at line 1

//@ Testing error sql logging with \show
||reports.query: You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near 'whatever' at line 1

//@ Testing error sql logging from session object
||ClassicSession.runSql: Unknown column 'whatever' in 'field list' (MySQL Error 1054)

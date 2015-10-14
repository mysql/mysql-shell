
from utils import *

class RegressionTests(ShellTestCase):
    def test_crash_invalid_add(self):
        "adding [] to a collection crashes"
        self.loaddb("worldx")
        try:
            mysqlx_with_stdin("db.CountryInfo.add([]).execute();\n", "-uroot", "worldx")
        except MysqlxError, e:
            self.assertEqual(e.code, 1)


    @checkstdout
    def test_sql_multiline_end(self):
        "multiline in SQL is not being terminated when a ; is entered"
        mysqlx_with_stdin("select 1\n;\n", "-uroot", "--sqlc")

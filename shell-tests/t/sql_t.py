
from utils import *

class SqlModeTests(ShellTestCase):
    @checkstdout
    def test_script_file(self):
        "Feed script via file to SQL"

        f = self.tmpfile("tscript.sql")
        f.write("select 1;\n");
        f.close()

        mysqlx("-uroot", "--sqlc", "--file=$TMPDIR/tscript.sql")


    @checkstdout
    def test_script_stdin(self):
        mysqlx_feedstdin("select 1;", "-uroot", "--sqlc")


    #@checkstdout
    @unittest.skip("code needs to be fixed")
    def test_script_stdin_no_semicolon(self):
        "This is not working as of 2015-10-7"
        mysqlx_feedstdin("select 1", "-uroot", "--sqlc")

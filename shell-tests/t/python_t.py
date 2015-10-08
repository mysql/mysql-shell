
from utils import *

class PythonModeTests(ShellTestCase):
    @checkstdout
    def test_script_file(self):
        "Feed script via file"

        f = self.tmpfile("tscript.py")
        f.write("print 1\n");
        f.close()

        mysqlx("--py", "--file=$TMPDIR/tscript.py")


    def test_script_stdin(self):
        rc, out = mysqlx_with_stdin("print 1", "--py")
        self.assertEqual(rc, 0)
        self.assertEqual(out.strip(), "1")


    @checkstdout
    def test_comment(self):
        mysqlx_with_stdin("#", "--py", "--interactive")


    @checkstdout
    def test_error(self):
        "MYS-306"
        mysqlx_with_stdin("invalid()", "--py")

        f = self.tmpfile("broken.py")
        f.write("invalid()\n")
        f.close()
        mysqlx("--py", "--file=$TMPDIR/broken.py")

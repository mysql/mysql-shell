
from utils import *

class MultilineTests(ShellTestCase):
    @checkstdout
    def test_python(self):
        mysqlx_with_stdin(["print 'begin'", "if True:", "    print 'works'", ""], "-uroot", "--py", "--interactive")

    @checkstdout
    def test_javascript(self):
        mysqlx_with_stdin(["if (true)", "print(1234)", ""], "--interactive", "--js")


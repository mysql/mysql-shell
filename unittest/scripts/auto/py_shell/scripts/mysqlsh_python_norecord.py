#@ Python in interactive mode properly initializes sys.argv
testutil.call_mysqlsh(["--py", "-i" ,"-e", "import sys; print('sys.argv = {0}'.format(sys.argv))"])
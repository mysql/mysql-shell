import os
import os.path
import subprocess
import sys

#@<> Python in interactive mode properly initializes sys.argv
testutil.call_mysqlsh(["--py", "-i" ,"-e", "import sys; print('sys.argv = {0}'.format(sys.argv))"])
EXPECT_STDOUT_CONTAINS("sys.argv = ['']")

#@<> Check that -c works as expected
testutil.call_mysqlsh(["-c", "import sys; print(sys.argv)", "hello", "--foo", "--bar"])
EXPECT_STDOUT_CONTAINS("['-c', 'hello', '--foo', '--bar']")

#@<> Check that subprocess works
subproc_script = os.path.join(__tmp_dir, "subproc.py")

with open(subproc_script, "w+") as f:
    f.write(
        """
import multiprocessing
def run_proc(arg):
    print("".join(list(reversed("dlrow olleh"))).upper() + str(arg+1))

if __name__ == "__main__":
    with multiprocessing.Pool(3) as p:
        p.map(run_proc, [1,3,5,7])
"""
    )

testutil.call_mysqlsh(["-f", subproc_script])
EXPECT_STDOUT_CONTAINS("HELLO WORLD8")

#@<> ensure that sys.executable points to the right place and that it's in the same place as the python libraries (including when bundled)
print("executable =", sys.executable)
print("prefix =", sys.prefix)

EXPECT_TRUE(os.path.basename(sys.executable).lower().startswith("python"))
EXPECT_TRUE(os.path.realpath(sys.executable).lower().startswith(os.path.realpath(sys.prefix).lower()))

if __os_type == "windows":
    EXPECT_TRUE(os.path.basename(os.path.dirname(sys.executable)).lower().startswith("python"))
else:
    EXPECT_EQ("bin", os.path.basename(os.path.dirname(sys.executable)).lower())

#@<> BUG#36200851 make sure if it's possible to execute sys.executable
# ensure that sys.executable, sys.prefix and sys.exec_prefix point to the same place
r = subprocess.run(
    [sys.executable, "-c", "import sys;print(sys.executable, sys.prefix, sys.exec_prefix)"],
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT
)

EXPECT_EQ(" ".join([sys.executable, sys.prefix, sys.exec_prefix]), r.stdout.strip())

#@<> BUG#36200851 make sure that sys.executable is able to load the ssl module
r = subprocess.run(
    [sys.executable, "-c", "import ssl"],
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT
)

EXPECT_EQ("", r.stdout.strip())

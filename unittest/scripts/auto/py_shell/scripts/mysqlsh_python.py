# @<> Check that -c works as expected

testutil.call_mysqlsh(["-c", "import sys; print(sys.argv)", "hello", "--foo", "--bar"])
EXPECT_STDOUT_CONTAINS("['-c', 'hello', '--foo', '--bar']")

# @<> Check that subprocess works

with open(__tmp_dir + "/subproc.py", "w+") as f:
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

testutil.call_mysqlsh(["-f", __tmp_dir + "/subproc.py"])

EXPECT_STDOUT_CONTAINS("HELLO WORLD8")


#@<> Check that -c works as expected

testutil.call_mysqlsh(["-c", "import sys; print(sys.argv)", "hello", "--foo", "--bar"])
EXPECT_STDOUT_CONTAINS("['-c', 'hello', '--foo', '--bar']")

#@<> Check that subprocess works

with open(__tmp_dir+"/subproc.py", "w+") as f:
    f.write("""
import multiprocessing
def run_proc():
    print("".join(list(reversed("dlrow olleh"))).upper())

if __name__ == "__main__":
    proc = multiprocessing.Process(target=run_proc)

    proc.start()
    proc.join()
    proc.close()
""")

testutil.call_mysqlsh(["-f", __tmp_dir+"/subproc.py"])

EXPECT_STDOUT_CONTAINS("HELLO WORLD")



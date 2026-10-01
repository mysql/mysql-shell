import os
import os.path
import subprocess
import sys
from contextlib import ExitStack
from pathlib import Path
from tempfile import TemporaryDirectory
from zipfile import ZipFile

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

EXPECT_EQ(0, r.returncode)
EXPECT_EQ(" ".join([sys.executable, sys.prefix, sys.exec_prefix]), r.stdout.strip())

#@<> BUG#36200851 make sure that sys.executable is able to load the ssl module
r = subprocess.run(
    [sys.executable, "-c", "import ssl"],
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT
)

EXPECT_EQ(0, r.returncode)
EXPECT_EQ("", r.stdout.strip())

#@<> Shell startup loads packages, ZIPs, and all top-level .pth files in that order
python_packages = next(Path(p) for p in sys.path if Path(p).name == "python-packages")

check_imports = """
import importlib
import os
import sys

def normalize(path):
    return os.path.normcase(os.path.abspath(path))

for name, (source, location) in EXPECTED_MODULES.items():
    module = importlib.import_module(name)
    assert module.SOURCE == source, (name, module.SOURCE)
    assert normalize(module.__file__) == normalize(location), (name, module.__file__)

paths = [normalize(p) for p in sys.path]
positions = [paths.index(normalize(p)) for p in EXPECTED_PATHS]
assert positions == sorted(positions), (EXPECTED_PATHS, paths)
assert all(paths.count(normalize(p)) == 1 for p in EXPECTED_PATHS), paths
print("Python package paths verified")
"""

with ExitStack() as cleanup:
    # Unique names allow concurrent tests without touching existing packages.
    package = Path(cleanup.enter_context(TemporaryDirectory(prefix="mysqlsh_path_test_", dir=python_packages)))
    external = Path(cleanup.enter_context(TemporaryDirectory(dir=__tmp_dir)))
    name = package.name
    archive = python_packages / (name + ".zip")
    relative_dir = package / "relative"
    absolute_dir = external / "absolute"
    external_archive = external / "external.zip"
    relative_dir.mkdir()
    absolute_dir.mkdir()
    (package / "__init__.py").write_text("SOURCE = 'root'\n", encoding="utf-8")
    (relative_dir / (name + "_relative.py")).write_text("SOURCE = 'relative'\n", encoding="utf-8")
    (relative_dir / (name + "_zip.py")).write_text("SOURCE = 'pth'\n", encoding="utf-8")
    (absolute_dir / (name + "_absolute.py")).write_text("SOURCE = 'absolute'\n", encoding="utf-8")
    #
    with ZipFile(archive, "x") as zipped:
        cleanup.callback(archive.unlink)
        zipped.writestr(name + "/__init__.py", "SOURCE = 'zip'\n")
        zipped.writestr(name + "_zip.py", "SOURCE = 'zip'\n")
    #
    with ZipFile(external_archive, "x") as zipped:
        zipped.writestr(name + "_external_zip.py", "SOURCE = 'external zip'\n")
    #
    # Both .pth files must be processed. Their targets need not be sibling ZIPs.
    for suffix, contents in (
        ("_a.pth", str(relative_dir.relative_to(python_packages)) + "\n" + str(external_archive) + "\n"),
        ("_b.pth", str(absolute_dir) + "\n" + str(python_packages) + "\n" + str(archive) + "\n"),
    ):
        pth = python_packages / (name + suffix)
        with pth.open("x", encoding="utf-8") as path_file:
            cleanup.callback(pth.unlink)
            path_file.write(contents)
    #
    expected_modules = {
        name: ("root", str(package / "__init__.py")),
        name + "_zip": ("zip", str(archive / (name + "_zip.py"))),
        name + "_relative": ("relative", str(relative_dir / (name + "_relative.py"))),
        name + "_absolute": ("absolute", str(absolute_dir / (name + "_absolute.py"))),
        name + "_external_zip": ("external zip", str(external_archive / (name + "_external_zip.py"))),
    }
    expected_paths = [str(p) for p in (python_packages, archive, relative_dir, external_archive, absolute_dir)]
    #
    check_imports = "EXPECTED_MODULES = " + repr(expected_modules) + "\nEXPECTED_PATHS = " + repr(expected_paths) + "\n" + check_imports
    EXPECT_EQ(0, testutil.call_mysqlsh(
        ["--py", "--disable-builtin-plugins", "-e", check_imports],
        "", ["PYTHONDONTWRITEBYTECODE=1"]))
    EXPECT_STDOUT_CONTAINS("Python package paths verified")

from __future__ import print_function
import difflib
import hashlib
import mysqlsh
import random
import re
import string
import sys

def get_members(object):
  all_exports = dir(object)

  exports = []
  for member in all_exports:
    if not member.startswith('__'):
      exports.append(member)

  return exports

##
# Verifies if a variable is defined, returning true or false accordingly
# @param cb An anonymous function that simply executes the variable to be
# verified, example:
#
# defined(lambda:myVar)
#
# Will return True if myVar is defined or False if not.
##
def defined(cb):
  try:
    cb()
    return True
  except:
    return False

def create_root_from_anywhere(session):
  session.run_sql("SET SQL_LOG_BIN=0")
  session.run_sql("CREATE USER root@'%' IDENTIFIED BY 'root'")
  session.run_sql("GRANT ALL ON *.* to root@'%' WITH GRANT OPTION")
  session.run_sql("SET SQL_LOG_BIN=1")

def has_oci_environment(context):
  if context not in ['OS', 'MDS']:
    return False
  variables = ['OCI_CONFIG_HOME',
               'OCI_COMPARTMENT_ID',
               'OS_NAMESPACE',
               'OS_BUCKET_NAME']
  if context == 'MDS':
    variables = variables + ['MDS_URI']
  missing=[]
  for variable in variables:
    if variable not in globals():
      missing.append(variable)
  if (len(missing)):
    sys.stderr.write("Missing Variables: {}".format(", ".join(missing)))
    return False
  return True

def is_re_instance(o):
    return isinstance(o, is_re_instance.__re_type)

is_re_instance.__re_type = type(re.compile(''))

class __TextMatcher:
    def __init__(self, o):
        self.__is_re = is_re_instance(o)
        if not self.__is_re and not isinstance(o, str):
            raise Exception("Expected str or re.Pattern, but got: " + str(type(o)))
        self.__o = o
    def __str__(self):
        if self.__is_re:
            return "re.Pattern('" + self.__o.pattern + "')"
        else:
            return self.__o
    def matches(self, s):
        if self.__is_re:
            return self.__o.search(s) is not None
        else:
            return s.find(self.__o) != -1

def EXPECT_EQ(expected, actual, note=""):
  if expected != actual:
    context = "Tested values don't match as expected: "+note+"\n\tActual:   " + str(actual) + "\n\tExpected: " + str(expected)
    testutil.fail(context)

def EXPECT_NE(expected, actual, note=""):
  if expected == actual:
    context = "Tested values should not match: "+note+"\n\tActual: " + str(actual) + "\n\tExpected: " + str(expected)
    testutil.fail(context)

def EXPECT_LE(expected, actual, note=""):
  if expected > actual:
    context = "Tested values not as expected: "+note+"\n\t"+str(expected)+" (expected) <= "+str(actual)+" (actual)"
    testutil.fail(context)

def EXPECT_LT(expected, actual, note=""):
  if expected >= actual:
    context = "Tested values not as expected: "+note+"\n\t"+str(expected)+" (expected) < "+str(actual)+" (actual)"
    testutil.fail(context)

def EXPECT_GE(expected, actual, note=""):
  if expected < actual:
    context = "Tested values not as expected: "+note+"\n\t"+str(expected)+" (expected) >= "+str(actual)+" (actual)"
    testutil.fail(context)

def EXPECT_GT(expected, actual, note=""):
  if expected <= actual:
    context = "Tested values not as expected: "+note+"\n\t"+str(expected)+" (expected) > "+str(actual)+" (actual)"
    testutil.fail(context)

def EXPECT_BETWEEN(expected_from, expected_to, actual, note=""):
  if expected_from >= actual >= expected_to:
    context = "Tested value not as expected: "+note+f"\n\t{expected_from} >= {actual} >= {expected_to}"
    testutil.fail(context)

def EXPECT_NOT_BETWEEN(expected_from, expected_to, actual, note=""):
  if expected_from >= actual >= expected_to:
    pass
  else:
    context = "Tested value not as expected: "+note+f"\n\tNOT ({expected_from} >= {actual} >= {expected_to})"
    testutil.fail(context)

def EXPECT_TRUE(value, note=""):
  if not value:
    context = f"Tested value '{value}' expected to be true but is false"
    if note:
        context += ": "+note
    testutil.fail(context)

def EXPECT_FALSE(value, note=""):
  if value:
    context = f"Tested value '{value}' expected to be false but is true"
    if note:
        context += ": "+note
    testutil.fail(context)

def EXPECT_THROWS(func, etext):
  m = __TextMatcher(etext)
  try:
    func()
    testutil.fail("<red>Missing expected exception throw like " + str(m) + "</red>")
    return False
  except Exception as e:
    exception_message = type(e).__name__ + ": " + str(e)
    if not m.matches(exception_message):
      testutil.fail("<red>Exception expected:</red> " + str(m) + "\n\t<yellow>Actual:</yellow> " + exception_message)
      return False
    return True

def EXPECT_MAY_THROW(func, etext):
  m = __TextMatcher(etext)
  ret = None
  try:
    ret = func()
  except Exception as e:
    exception_message = type(e).__name__ + ": " + str(e)
    if not m.matches(exception_message):
      testutil.fail("<red>Exception expected:</red> " + str(m) + "\n\t<yellow>Actual:</yellow> " + exception_message)
  return ret

def EXPECT_NO_THROWS(func, context):
  try:
    func()
  except Exception as e:
    testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Unexpected exception thrown (" + context + "): " + str(e) + "</red>")

def EXPECT_STDOUT_CONTAINS(text):
  out = testutil.fetch_captured_stdout(False)
  err = testutil.fetch_captured_stderr(False)
  if out.find(text) == -1:
    context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err
    testutil.fail(context)

def EXPECT_STDERR_CONTAINS(text):
  out = testutil.fetch_captured_stdout(False)
  err = testutil.fetch_captured_stderr(False)
  if err.find(text) == -1:
    context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err
    testutil.fail(context)

def WIPE_STDOUT():
    line = testutil.fetch_captured_stdout(True)
    while line != "":
        line = testutil.fetch_captured_stdout(True)

def WIPE_STDERR():
    line = testutil.fetch_captured_stderr(True)
    while line != "":
        line = testutil.fetch_captured_stderr(True)

def WIPE_OUTPUT():
  WIPE_STDOUT()
  WIPE_STDERR()

def WIPE_SHELL_LOG():
    testutil.wipe_file_contents(testutil.get_shell_log_path())

def EXPECT_SHELL_LOG_CONTAINS(text):
    log_file = testutil.get_shell_log_path()
    match_list = testutil.grep_file(log_file, text)
    if len(match_list) == 0:
        log_out = testutil.cat_file(log_file)
        testutil.fail(f"<b>Context:</b> {__test_context}\n<red>Missing log output:</red> {text}\n<yellow>Actual log output:</yellow> {log_out}")

def EXPECT_SHELL_LOG_NOT_CONTAINS(text):
    log_file = testutil.get_shell_log_path()
    match_list = testutil.grep_file(log_file, text)
    if len(match_list) != 0:
        log_out = testutil.cat_file(log_file)
        testutil.fail(f"<b>Context:</b> {__test_context}\n<red>Unexpected log output:</red> {text}\n<yellow>Actual log output:</yellow> {log_out}")

def EXPECT_SHELL_LOG_MATCHES(re):
    log_file = testutil.get_shell_log_path()
    with open(log_file, "r", encoding="utf-8") as f:
        log_out = f.read()
    if re.search(log_out) is None:
        testutil.fail(f"<b>Context:</b> {__test_context}\n<red>Missing match for:</red> {re.pattern}\n<yellow>Actual log output:</yellow> {log_out}")

def EXPECT_STDOUT_MATCHES(re):
    out = testutil.fetch_captured_stdout(False)
    err = testutil.fetch_captured_stderr(False)
    if re.search(out) is None:
        context = "<b>Context:</b> " + __test_context + "\n<red>Missing match for:</red> " + re.pattern + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err
        testutil.fail(context)

def EXPECT_STDOUT_NOT_CONTAINS(text):
    out = testutil.fetch_captured_stdout(False)
    err = testutil.fetch_captured_stderr(False)
    if out.find(text) != -1:
        context = "<b>Context:</b> " + __test_context + "\n<red>Unexpected output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err
        testutil.fail(context)

def EXPECT_FILE_CONTAINS(expected, path):
    with open(path, encoding='utf-8') as f:
        contents = f.read()
    if contents.find(expected) == -1:
        context = "<b>Context:</b> " + __test_context + "\n<red>Missing contents:</red> " + expected + "\n<yellow>Actual contents:</yellow> " + contents + "\n<yellow>File:</yellow> " + path
        testutil.fail(context)

def EXPECT_FILE_NOT_CONTAINS(expected, path):
    with open(path, encoding='utf-8') as f:
        contents = f.read()
    if contents.find(expected) != -1:
        context = "<b>Context:</b> " + __test_context + "\n<red>Unexpected contents:</red> " + expected + "\n<yellow>Actual contents:</yellow> " + contents + "\n<yellow>File:</yellow> " + path
        testutil.fail(context)

def validate_crud_functions(crud, expected):
  actual = dir(crud)

  # Ensures expected functions are on the actual list
  missing = []
  for exp_funct in expected:
    try:
      pos = actual.index(exp_funct)
      actual.remove(exp_funct)
    except:
      missing.append(exp_funct)

  if len(missing) == 0:
    print("All expected functions are available\n")
  else:
    print("Missing Functions:", missing)

  if len(actual) == 0 or (len(actual) == 1 and actual[0] == 'help'):
    print("No additional functions are available\n")
  else:
    print("Extra Functions:", actual)

def validate_members(object, expected_members):
  all_members = dir(object)

  # Remove the python built in members
  members = []
  for member in all_members:
    if not member.startswith('__'):
      members.append(member)

  missing = []
  for expected in expected_members:
    try:
      index = members.index(expected)
      members.remove(expected)
    except:
      missing.append(expected)

  errors = []
  error = ""
  if len(members):
    error = "Unexpected Members: %s" % ', '.join(members)
    errors.append(error)

  error = ""
  if len(missing):
    error = "Missing Members: %s" % ', '.join(missing)
    errors.append(error)

  if len(errors):
    testutil.fail(', '.join(errors))

def print_differences(source, target):
    src_lines = []
    tgt_lines = []

    with open(source) as f:
        src_lines = f.readlines()

    with open(target) as f:
        tgt_lines = f.readlines()

    for line in difflib.context_diff(src_lines, tgt_lines, fromfile=source, tofile=target):
        testutil.dprint(line)

def ensure_plugin_enabled(plugin_name, session, plugin_soname=None):
    if plugin_soname is None:
        plugin_soname = plugin_name

    os = session.run_sql('select @@version_compile_os').fetch_one()[0]

    if os == "Win32" or os == "Win64":
        ext = "dll"
    else:
        ext = "so"

    try:
        session.run_sql("INSTALL PLUGIN {0} SONAME '{1}.{2}';".format(plugin_name, plugin_soname, ext))
    except mysqlsh.DBError as e:
        if 1125 != e.code:
            raise e

def ensure_plugin_disabled(plugin_name, session):
    is_installed = session.run_sql("SELECT COUNT(1) FROM INFORMATION_SCHEMA.PLUGINS WHERE PLUGIN_NAME LIKE '" + plugin_name + "';").fetch_one()[0]

    if is_installed:
        session.run_sql("UNINSTALL PLUGIN " + plugin_name + ";")


# Starting 8.0.24 the client lib started reporting connection error using
# host:port format, previous versions used just the host.
#
# This function is used to format the host description accordingly.
def libmysql_host_description(hostname, port):
  if testutil.version_check(__libmysql_version_id, ">", "8.0.23"):
    return hostname + ":" + str(port)

  return hostname

def random_string(lower, upper=None):
  if upper is None:
    upper = lower
  return ''.join(random.choices(string.ascii_letters + string.digits, k=random.randint(lower, upper)))

def random_email():
  return random_string(10, 40) + "@" + random_string(10, 40) + "." + random_string(3)

def md5sum(s):
  return hashlib.md5(s.encode("utf-8")).hexdigest()

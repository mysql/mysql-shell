from __future__ import print_function

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
    cb();
    return True;
  except:
    return False

def create_root_from_anywhere(session):
  session.run_sql("SET SQL_LOG_BIN=0")
  session.run_sql("CREATE USER root@'%' IDENTIFIED BY 'root'")
  session.run_sql("GRANT ALL ON *.* to root@'%' WITH GRANT OPTION")
  session.run_sql("SET SQL_LOG_BIN=1")


def EXPECT_EQ(expected, actual):
  if expected != actual:
    context = "Tested values don't match as expected:\n\tActual: " + actual + "\n\tExpected: " + expected
    testutil.fail(context)

def  EXPECT_TRUE(value):
  if not value:
    context = "Tested value expected to be true but is false"
    testutil.fail(context)

def EXPECT_FALSE(value):
  if value:
    context = "Tested value expected to be false but is true"
    testutil.fail(context)

def EXPECT_THROWS(func, etext):
  try:
    func()
    testutil.fail("<red>Missing expected exception throw like " + etext + "</red>")
  except Exception as e:
    exception_message = str(e)
    if exception_message.find(etext) == -1:
      testutil.fail("<red>Exception expected:</red> " + etext + "\n\t<yellow>Actual:</yellow> " + exception_message)

def EXPECT_NO_THROWS(func, context):
  try:
    func()
  except:
    testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Unexpected exception thrown (" + context + "): " + str(e) + "</red>");

def EXPECT_STDOUT_CONTAINS(text):
  out = testutil.fetch_captured_stdout(False)
  err = testutil.fetch_captured_stderr(False)
  if out.find(text) == -1:
    context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err
    testutil.fail(context)

def WIPE_STDOUT():
    line = testutil.fetch_captured_stdout(True)
    while line != "":
        line = testutil.fetch_captured_stdout(True)

def EXPECT_STDOUT_MATCHES(re):
    out = testutil.fetch_captured_stdout(False)
    err = testutil.fetch_captured_stderr(False)
    if re.match(out) is None:
        context = "<b>Context:</b> " + __test_context + "\n<red>Missing match for:</red> " + re.pattern + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err
        testutil.fail(context);

def EXPECT_STDOUT_NOT_CONTAINS(text):
    out = testutil.fetch_captured_stdout(False)
    err = testutil.fetch_captured_stderr(False)
    if out.find(text) != -1:
        context = "<b>Context:</b> " + __test_context + "\n<red>Unexpected output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
        testutil.fail(context);

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

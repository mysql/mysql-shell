
def get_members(object):
  all_exports = dir(object)

  exports = []
  for member in all_exports:
    if not member.startswith('__'):
      exports.append(member)

  return exports


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

def validate_crud_functions(crud, expected):
	actual = crud.__members__

	# Ensures expected functions are on the actual list
	missing = []
	for exp_funct in expected:
		try:
			pos = actual.index(exp_funct)
			actual.remove(exp_funct)
		except:
			missing.append(exp_funct)

	if len(missing) == 0:
		print ("All expected functions are available\n")
	else:
		print "Missing Functions:", missing

	if len(actual) == 0 or (len(actual) == 1 and actual[0] == 'help'):
		print "No additional functions are available\n"
	else:
		print "Extra Func tions:", actual

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
    except:
      missing.append(expected)

  unexpected = []
  for member in members:
    try:
      index = expected_members.index(member)
    except:
      unexpected.append(member)

  errors = []
  error = ""
  if len(unexpected):
    error = "Unexpected Members: %s" % ', '.join(unexpected)
    errors.append(error)

  error = ""
  if len(missing):
    error = "Missing Members: %s" % ', '.join(missing)
    errors.append(error)

  if len(errors):
    testutil.fail(', '.join(errors))

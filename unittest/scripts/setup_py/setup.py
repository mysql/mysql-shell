
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

	if len(actual) == 0:
		print "No additional functions are available\n"
	else:
		print "Extra Functions:", actual

def validateMember(memberList, member):
	index = -1
	try:
		index = memberList.index(member)
	except:
		pass

	if index != -1:
		print member + ": OK\n"
	else:
		print member + ": Missing\n"

def validateNotMember(memberList, member):
	index = -1
	try:
		index = memberList.index(member)
	except:
		pass

	if index != -1:
		print member + ": Unexpected\n"
	else:
		print member + ": OK\n"

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

def ensure_schema_does_not_exist(session, name):
	try:
		schema = session.getSchema(name)
		session.dropSchema(name)
	except:
		# Nothing happens, it means the schema did not exist
		pass
		
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

def getSchemaFromList(schemas, name):
  for schema in schemas:
    if schema.name == name:
      return schema
  
  return None

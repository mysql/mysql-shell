import mysqlx

# Variables assummed to exist on the script must be declared in global scope
testSession = None
db = None
myTable = None
myColl = None
nodeSession = None
session = None

def ensure_session():
  global testSession
  if testSession is None:
    print "Creating session...\n"

    testSession = mysqlx.getNodeSession(__uripwd)

    # Ensures the user on dev-api exists
    try:
      testSession.sql("create user mike@'%' identified by 's3cr3t!'").execute()
      testSession.sql("grant all on *.* to mike@'%' with grant option").execute()
    except:
      pass
  else:
    print "Session exists...\n"

def ensure_test_schema():
  ensure_session()

  try:
      s = testSession.getSchema('test')
      print "Test schema exists...\n"
  except:
    print "Creating test schema...\n"
    testSession.createSchema('test')

  testSession.setCurrentSchema('test')

def ensure_test_schema_on_db():
  global db
  ensure_test_schema()
  print "Assigning test schema to db...\n"
  db = testSession.getSchema('test')

def ensure_employee_table():
  ensure_test_schema_on_db()

  try:
    table = testSession.getSchema('test').getTable('employee')

    print "Employee table exists...\n"
  except:
    print "Creating employee table...\n"
    testSession.sql('create table test.employee (name varchar(50), age integer, gender varchar(20))').execute()
    table = db.getTable('employee')
    
    result = table.insert({'name': 'jack', 'age': 17, 'gender': 'male'}).execute()
    result = table.insert({'name': 'adam', 'age': 15, 'gender': 'male'}).execute()
    result = table.insert({'name': 'brian', 'age': 14, 'gender': 'male'}).execute()
    result = table.insert({'name': 'alma', 'age': 13, 'gender': 'female'}).execute()
    result = table.insert({'name': 'clare', 'age': 14, 'gender': 'female'}).execute()
    result = table.insert({'name': 'donna', 'age': 16, 'gender': 'female'}).execute()
    result = table.insert({'name': 'angel', 'age': 14, 'gender': 'male'}).execute()
    result = table.insert({'name': 'mike', 'age': 39, 'gender': 'male'}).execute()
    result = table.insert({'name': 'John', 'age': 20, 'gender': 'male'}).execute()
    result = table.insert({'name': 'johannes', 'age': 28, 'gender': 'male'}).execute()
    result = table.insert({'name': 'Sally', 'age': 19, 'gender': 'female'}).execute()
    result = table.insert({'name': 'Molly', 'age': 25, 'gender': 'female'}).execute()

def ensure_employee_table_on_mytable():
  global myTable
  ensure_employee_table()
  
  print "Assigning employee table to myTable...\n"
  
  myTable = testSession.getSchema('test').getTable('employee')

def ensure_empty_my_table_table():
  ensure_test_schema()
  
  print "Creating my_table table...\n"
  testSession.sql('drop table if exists test.my_table').execute()
  testSession.sql('create table test.my_table (id integer, name varchar(50))').execute()

def ensure_my_collection_collection():
  ensure_test_schema_on_db()

  try:
    test_coll = testSession.getSchema('test').getCollection('my_collection')

    print "my_collection collection exists...\n"
  except:
    print "Creating my_collection collection...\n"
    test_coll = db.createCollection('my_collection')

    result = test_coll.add({'name': 'jack', 'age': 17, 'gender': 'male'}).execute()
    result = test_coll.add({'name': 'adam', 'age': 15, 'gender': 'male'}).execute()
    result = test_coll.add({'name': 'brian', 'age': 14, 'gender': 'male'}).execute()
    result = test_coll.add({'name': 'alma', 'age': 13, 'gender': 'female'}).execute()
    result = test_coll.add({'name': 'clare', 'age': 14, 'gender': 'female'}).execute()
    result = test_coll.add({'name': 'donna', 'age': 16, 'gender': 'female'}).execute()
    result = test_coll.add({'name': 'angel', 'age': 14, 'gender': 'male'}).execute()
    result = test_coll.add({'name': 'mike', 'age': 39, 'gender': 'male'}).execute()
    result = test_coll.add({'name': 'John', 'age': 20, 'gender': 'male'}).execute()
    result = test_coll.add({'name': 'johannes', 'age': 28, 'gender': 'male'}).execute()
    result = test_coll.add({'name': 'Sally', 'age': 19, 'gender': 'female'}).execute()
    result = test_coll.add({'name': 'Molly', 'age': 25, 'gender': 'female'}).execute()


def ensure_not_my_collection_collection():
  ensure_test_schema()

  try:
    test_coll = testSession.getSchema('test').getCollection('my_collection')

    print ("Dropping my_collection...\n")
    testSession.dropCollection('test', 'my_collection')
  except:
    print "my_collection does not exist...\n"

def ensure_custom_id_unique():
  global myColl
  ensure_my_collection_collection()
  myColl = db.getCollection('my_collection')

  myColl.remove('_id = "custom_id"').execute()

def ensure_table_users_exists():
  global nodeSession
  ensure_test_schema()

  try:
    test_coll = testSession.getSchema('test').getTable('users')

    print 'users table exists...'
  except:
    print 'Creating users table...'
    testSession.sql('create table users (name varchar(50), age int)').execute()
    testSession.sql('insert into users values ("Jack", 17)').execute()

  nodeSession = testSession

def ensure_my_proc_procedure_exists():
  global nodeSession
  ensure_table_users_exists()

  procedure = """
	create procedure my_proc() 
	begin 
	  select * from test.users; 
	end"""
	
  testSession.sql("drop procedure if exists my_proc").execute()
  testSession.sql(procedure).execute()

  nodeSession = testSession

# Executes the functions associated to every assumption defined on the test case
for assumption in __assumptions__:
  print 'Assumption: %s\n' % assumption

  if assumption == "connected testSession":
    ensure_session()
  elif assumption == "test schema exists":
    ensure_test_schema()
  elif assumption == "test schema assigned to db":
    ensure_test_schema_on_db()
  elif assumption == "my_collection collection exists":
    ensure_my_collection_collection()
  elif assumption == "employee table exists":
    ensure_employee_table()
  elif assumption == "employee table exist on myTable":
    ensure_employee_table_on_mytable()
  elif assumption == "empty my_table table exists":
    ensure_empty_my_table_table()
  elif assumption == "my_collection collection not exists":
    ensure_not_my_collection_collection()
  elif assumption == "custom_id is unique":
    ensure_custom_id_unique()
  elif assumption == "users table exists":
    ensure_table_users_exists()
  elif assumption == "my_proc procedure exists":
    ensure_my_proc_procedure_exists()

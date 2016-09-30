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
  global session
  if testSession is None:
    print "Creating session...\n"

    testSession = mysqlx.get_node_session(__uripwd)

    # Ensures the user on dev-api exists
    try:
      testSession.sql("create user mike@'%' identified by 's3cr3t!'").execute()
      testSession.sql("grant all on *.* to mike@'%' with grant option").execute()
    except:
      pass
  else:
    print "Session exists...\n"

  session = testSession

def ensure_not_a_schema(schema):
  ensure_session()

  try:
      s = testSession.drop_schema(schema)
      print "%s schema has been deleted...\n" % schema
  except:
    print "%s schema does not exist...\n" % schema

def ensure_test_schema():
  ensure_session()

  try:
      s = testSession.get_schema('test')
      print "Test schema exists...\n"
  except:
    print "Creating test schema...\n"
    testSession.create_schema('test')

  testSession.set_current_schema('test')

def ensure_test_schema_on_db():
  global db
  ensure_test_schema()
  print "Assigning test schema to db...\n"
  db = testSession.get_schema('test')

def ensure_employee_table():
  ensure_test_schema_on_db()

  try:
    table = testSession.get_schema('test').get_table('employee')

    print "Employee table exists...\n"
  except:
    print "Creating employee table...\n"
    testSession.sql('create table test.employee (name varchar(50) primary key, age integer, gender varchar(20))').execute()
    table = db.get_table('employee')

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

def ensure_relatives_collection():
  ensure_test_schema_on_db()

  try:
    test_coll = testSession.get_schema('test').get_collection('relatives')

    print "Relatives collection exists...\n"
  except:
    print "Creating relatives collection...\n"
    test_coll = db.create_collection('relatives')

    result = test_coll.add({'name': 'jack', 'age': 17, 'alias': 'jack'}).execute()
    result = test_coll.add({'name': 'adam', 'age': 15, 'alias': 'jr'}).execute()
    result = test_coll.add({'name': 'brian', 'age': 14, 'alias': 'brian'}).execute()
    result = test_coll.add({'name': 'charles', 'age': 13, 'alias': 'jr'}).execute()
    result = test_coll.add({'name': 'clare', 'age': 14, 'alias': 'cla'}).execute()
    result = test_coll.add({'name': 'donna', 'age': 16, 'alias': 'donna'}).execute()

def ensure_employee_table_on_mytable():
  global myTable
  ensure_employee_table()

  print "Assigning employee table to myTable...\n"

  myTable = testSession.get_schema('test').get_table('employee')

def ensure_empty_my_table_table():
  ensure_test_schema()

  print "Creating my_table table...\n"
  testSession.sql('drop table if exists test.my_table').execute()
  testSession.sql('create table test.my_table (id integer primary key, name varchar(50))').execute()

def ensure_my_table_table():
  ensure_test_schema()

  print "Creating my_table table...\n"
  testSession.sql('drop table if exists test.my_table').execute()
  testSession.sql('create table test.my_table (_id integer NOT NULL AUTO_INCREMENT PRIMARY KEY, name varchar(50), birthday DATE, age INTEGER)').execute()


def ensure_my_collection_collection():
  ensure_test_schema_on_db()

  try:
    test_coll = testSession.get_schema('test').get_collection('my_collection')

    print "my_collection collection exists...\n"
  except:
    print "Creating my_collection collection...\n"
    test_coll = db.create_collection('my_collection')

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

def ensure_customers_collection():
  ensure_test_schema_on_db()

  try:
    test_coll = testSession.get_schema('test').get_collection('customers')

    print "customers collection exists...\n"
  except:
    print "Creating customers collection...\n"
    test_coll = db.create_collection('customers')

def ensure_my_coll_collection():
  ensure_test_schema_on_db()

  try:
    test_coll = testSession.get_schema('test').get_collection('my_coll')

    print "my_coll collection exists...\n"
  except:
    print "Creating my_coll collection...\n"
    test_coll = db.create_collection('my_coll')

def ensure_not_collection(name):
  ensure_test_schema()

  try:
    test_coll = testSession.get_schema('test').get_collection(name)

    print "Dropping %s...\n" % name
    testSession.drop_collection('test', name)
  except:
    print "%s does not exist...\n" % name

def ensure_custom_id_unique():
  global myColl
  ensure_my_collection_collection()
  myColl = db.get_collection('my_collection')

  myColl.remove('_id = "custom_id"').execute()

def ensure_table_users_exists():
  global nodeSession
  ensure_test_schema()

  try:
    test_coll = testSession.get_schema('test').get_table('users')

    print 'users table exists...'
  except:
    print 'Creating users table...'
    testSession.sql('create table users (name varchar(50) primary key, age int)').execute()

  testSession.sql('delete from test.users').execute()
  testSession.sql('insert into test.users values ("Jack", 17)').execute()
  testSession.sql('insert into test.users values ("John", 40)').execute()
  testSession.sql('insert into test.users values ("Alfred", 39)').execute()
  testSession.sql('insert into test.users values ("Mike", 40)').execute()
  testSession.sql('insert into test.users values ("Armand", 50)').execute()
  testSession.sql('insert into test.users values ("Rafa", 38)').execute()

  nodeSession = testSession

def ensure_my_proc_procedure_exists():
  global nodeSession
  ensure_table_users_exists()

  procedure = """
	create procedure my_proc()
	begin
	  select * from test.users where age > 40;
    select * from test.users where age < 40;
    delete from test.users where age = 40;
	end"""

  testSession.sql("drop procedure if exists my_proc").execute()
  testSession.sql(procedure).execute()

  nodeSession = testSession

# Executes the functions associated to every assumption defined on the test case
for assumption in __assumptions__:
  print 'Assumption: %s\n' % assumption

  if assumption == "connected session":
    ensure_session()
  elif assumption == "test schema exists":
    ensure_test_schema()
  elif assumption == "test schema assigned to db":
    ensure_test_schema_on_db()
  elif assumption == "my_collection collection exists":
    ensure_not_collection("my_collection");
    ensure_my_collection_collection()
  elif assumption == "customers collection exists":
    ensure_not_collection("customers")
    ensure_customers_collection()
  elif assumption == "employee table exists":
    ensure_employee_table()
  elif assumption == "employee table exist on myTable":
    ensure_employee_table_on_mytable()
  elif assumption == "empty my_table table exists":
    ensure_empty_my_table_table()
  elif assumption == "my_table table exists":
    ensure_my_table_table();
  elif assumption == "my_collection collection not exists":
    ensure_not_collection("my_collection")
  elif assumption == "my_coll collection is empty":
    ensure_not_collection("my_coll")
    ensure_my_coll_collection()
  elif assumption == "custom_id is unique":
    ensure_custom_id_unique()
  elif assumption == "users table exists":
    ensure_table_users_exists()
  elif assumption == "my_proc procedure exists":
    ensure_my_proc_procedure_exists()
  elif assumption == "relatives collection exists":
    ensure_not_collection("relatives")
    ensure_relatives_collection()
  elif assumption == "schema unexisting does not exists":
    ensure_not_a_schema('unexisting')

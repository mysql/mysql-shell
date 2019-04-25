# Assumptions: validate_crud_functions available
from __future__ import print_function
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.create_schema('js_shell_test')
mySession.set_current_schema('js_shell_test')

# Creates a test table with initial data
result = mySession.sql('create table table1 (name varchar(50) primary key, age integer, gender varchar(20))').execute()
result = mySession.sql('create view view1 (my_name, my_age, my_gender) as select name, age, gender from table1;').execute()
table = schema.get_table('table1')

result = table.insert({"name": 'jack', "age": 17, "gender": 'male'}).execute()
result = table.insert({"name": 'adam', "age": 15, "gender": 'male'}).execute()
result = table.insert({"name": 'brian', "age": 14, "gender": 'male'}).execute()
result = table.insert({"name": 'alma', "age": 13, "gender": 'female'}).execute()
result = table.insert({"name": 'carol', "age": 14, "gender": 'female'}).execute()
result = table.insert({"name": 'donna', "age": 16, "gender": 'female'}).execute()
result = table.insert({"name": 'angel', "age": 14, "gender": 'male'}).execute()

# ------------------------------------------------
# table.delete Unit Testing: Dynamic Behavior
# ------------------------------------------------
#@ TableDelete: valid operations after delete
crud = table.delete()
validate_crud_functions(crud, ['where', 'order_by', 'limit', 'execute'])

#@ TableDelete: valid operations after where
crud = crud.where("id < 100")
validate_crud_functions(crud, ['order_by', 'limit', 'bind', 'execute'])

#@ TableDelete: valid operations after order_by
crud = crud.order_by(['name'])
validate_crud_functions(crud, ['limit', 'bind', 'execute'])

#@ TableDelete: valid operations after limit
crud = crud.limit(1)
validate_crud_functions(crud, ['bind', 'execute'])

#@ TableDelete: valid operations after bind
crud = table.delete().where('name = :data').bind('data', 'donna')
validate_crud_functions(crud, ['bind', 'execute'])

#@ TableDelete: valid operations after execute
result = crud.execute()
validate_crud_functions(crud, ['limit', 'bind', 'execute'])

#@ Reusing CRUD with binding
print('Deleted donna:', result.affected_items_count, '\n')
result=crud.bind('data', 'alma').execute()
print('Deleted alma:', result.affected_items_count, '\n')


# ----------------------------------------------
# table.delete Unit Testing: Error Conditions
# ----------------------------------------------

#@# TableDelete: Error conditions on delete
crud = table.delete(5)

#@# TableDelete: Error conditions on where
crud = table.delete().where()
crud = table.delete().where(5)
crud = table.delete().where('test = "2')

#@# TableDelete: Error conditions on order_by
crud = table.delete().order_by()
crud = table.delete().order_by(5)
crud = table.delete().order_by([])
crud = table.delete().order_by(['name', 5])
crud = table.delete().order_by('name', 5)

#@# TableDelete: Error conditions on limit
crud = table.delete().limit()
crud = table.delete().limit('')

#@# TableDelete: Error conditions on bind
crud = table.delete().where('name = :data and age > :years').bind()
crud = table.delete().where('name = :data and age > :years').bind(5, 5)
crud = table.delete().where('name = :data and age > :years').bind('another', 5)

#@# TableDelete: Error conditions on execute
crud = table.delete().where('name = :data and age > :years').execute()
crud = table.delete().where('name = :data and age > :years').bind('years', 5).execute()

# ---------------------------------------
# table.delete Unit Testing: Execution
# ---------------------------------------

#@ TableDelete: delete under condition
result = table.delete().where('age = 15').execute()
print('Affected Rows:', result.affected_items_count, '\n')

records = table.select().execute().fetch_all()
print('Records Left:', len(records), '\n')

#@ TableDelete: delete with binding
result = table.delete().where('gender = :heorshe').limit(2).bind('heorshe', 'male').execute()
print('Affected Rows:', result.affected_items_count, '\n')

records = table.select().execute().fetch_all()
print('Records Left:', len(records), '\n')

#@ TableDelete: full delete with a view object
view = schema.get_table('view1')
result = view.delete().execute()
print('Affected Rows:', result.affected_items_count, '\n')

# Deletion is of course reflected on the target table
records = table.select().execute().fetch_all()
print('Records Left:', len(records), '\n')

#@ TableDelete: with limit 0
result = table.insert({"name": 'adam', "age": 15, "gender": 'male'}).execute()
result = table.insert({"name": 'brian', "age": 14, "gender": 'male'}).execute()
result = table.insert({"name": 'alma', "age": 13, "gender": 'female'}).execute()
result = table.insert({"name": 'carol', "age": 14, "gender": 'female'}).execute()
result = table.insert({"name": 'donna', "age": 16, "gender": 'female'}).execute()

records = table.select().execute().fetch_all()
print('Records Left:', len(records), '\n')

#@ TableDelete: with limit 1
result = table.delete().limit(2).execute()
print('Affected Rows:', result.affected_items_count, '\n')

records = table.select().execute().fetch_all()
print('Records Left:', len(records), '\n')

#@ TableDelete: with limit 2
result = table.delete().limit(2).execute()
print('Affected Rows:', result.affected_items_count, '\n')

records = table.select().execute().fetch_all()
print('Records Left:', len(records), '\n')

#@ TableDelete: with limit 3
result = table.delete().limit(2).execute()
print('Affected Rows:', result.affected_items_count, '\n')

records = table.select().execute().fetch_all()
print('Records Left:', len(records), '\n')


# Cleanup
mySession.drop_schema('js_shell_test')
mySession.close()

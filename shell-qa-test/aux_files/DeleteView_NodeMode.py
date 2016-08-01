session.sql('drop view if exists py_view;').execute()
session.sql("create view py_view as select first_name from actor where first_name like '%a%';").execute()
session.drop_view('sakila','py_view')
session.sql("SELECT table_name FROM information_schema.views WHERE information_schema.views.table_schema LIKE '%py_view%';").execute()
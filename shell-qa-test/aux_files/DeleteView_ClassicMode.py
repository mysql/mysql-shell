session.run_sql('drop view if exists py_view;');
session.run_sql("create view py_view as select first_name from actor where first_name like '%a%';");
session.get_schema('sakila').get_views();
session.drop_view('sakila','py_view');

session.run_sql("SELECT table_name FROM information_schema.views WHERE information_schema.views.table_schema LIKE '%py_view%';");
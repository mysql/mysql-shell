session.runSql('drop view if exists py_view;');
session.runSql("create view py_view as select first_name from actor where first_name like '%a%';");
session.getSchema('sakila').getViews();
session.dropView('sakila','py_view');

session.runSql("SELECT table_name FROM information_schema.views WHERE information_schema.views.table_schema LIKE '%py_view%';");
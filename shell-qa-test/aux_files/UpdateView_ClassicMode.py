session.run_sql("USE sakila")
session.run_sql("drop view if exists py_view;")
session.run_sql("create view py_view as select first_name from actor where first_name like '%a%';")

session.run_sql("alter view py_view as select count(actor_id) as ActorQuantity from actor;")
session.run_sql("SELECT * FROM py_view ;")

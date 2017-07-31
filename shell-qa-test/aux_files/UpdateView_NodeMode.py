session.sql("USE sakila")
session.sql("drop view if exists py_view;")
session.sql("create view py_view as select first_name from actor where first_name like '%a%';")
session.sql("alter view py_view as select count(actor_id) as ActorQuantity from actor;")
session.sql("SELECT * FROM py_view ;")



session.sql("USE sakila");
session.sql("drop view if exists js_viewnode;");

session.sql("create view js_viewnode as select first_name from actor where first_name like '%a%';");

session.sql("alter view js_viewnode as select count(actor_id) as ActorQuantity from actor;");

session.sql("SELECT * FROM js_viewnode;");
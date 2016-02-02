session.runSql("USE sakila");
session.runSql("drop view if exists js_view;");

session.runSql("create view js_view as select first_name from actor where first_name like '%a%';");

session.runSql("alter view js_view as select count(actor_id) as ActorQuantity from actor;");

session.runSql("SELECT * FROM js_view ;");

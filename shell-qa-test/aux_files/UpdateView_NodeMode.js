session.sql("USE sakila");
session.sql("alter view actor_info as select count(actor_id) as ActorQuantity4 from actor;");
session.sql("SELECT * FROM actor_info;");
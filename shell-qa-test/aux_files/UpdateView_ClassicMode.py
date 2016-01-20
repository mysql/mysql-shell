session.runSql("USE sakila");
session.runSql("alter view actor_info as select count(actor_id) as ActorQuantity84 from actor;");
session.runSql("SELECT * FROM actor_info;");
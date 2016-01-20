session.executeSql("USE sakila");
session.executeSql("alter view actor_info as select count(actor_id) as ActorQuantity3 from actor;");
session.executeSql("SELECT * FROM actor_info;");
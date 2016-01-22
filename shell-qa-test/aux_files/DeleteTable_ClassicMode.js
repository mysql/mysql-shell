session.runSql("delete from film_actor where actor_id = 1 order by film_id desc limit 1;");
session.runSql("select * from film_actor where actor_id = 1 order by film_id desc;");
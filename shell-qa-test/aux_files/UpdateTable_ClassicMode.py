session.run_sql('use sakila;')
session.run_sql("UPDATE actor SET first_name = 'Updated By JS Classic STDIN' where actor_id = 50 ;");
session.run_sql("SELECT * FROM sakila.actor where actor_id = 50;")
session.run_sql("UPDATE actor SET first_name = 'Old value' where actor_id = 50 ;")

USE sakila;
UPDATE actor SET first_name='Test',last_name='Test Last Name',last_update=now() WHERE actor_id=2;
SELECT * FROM actor WHERE actor_id=2;
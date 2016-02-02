use sakila;

DROP PROCEDURE if exists test_procedure;
delimiter $$
Create procedure test_procedure() BEGIN SELECT count(*) as NumberOfCountries_Upd FROM country; END$$
delimiter ;
call test_procedure;

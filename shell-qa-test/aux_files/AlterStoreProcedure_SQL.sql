Use sakila;

DROP procedure IF EXISTS sql_sptest ;

DELIMITER $$

create procedure sql_sptest (OUT param1 INT)
BEGIN

SELECT count(*) INTO param1 FROM country;

END$$

DELIMITER ;

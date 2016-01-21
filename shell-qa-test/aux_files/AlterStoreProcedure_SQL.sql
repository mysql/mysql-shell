Use sakila;
DROP procedure IF EXISTS Test;
DELIMITER $$
Create procedure Test()
BEGIN
SELECT count(*) FROM country;
END$$
DELIMITER ;

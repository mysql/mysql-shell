USE world_x;

DELIMITER $$
CREATE PROCEDURE InsertInfoSQLColl(IN rowQuantity int)
BEGIN
	DECLARE id_k int;
    DECLARE document varchar(400); 
    SET id_k = 1;
	WHILE id_k <= rowQuantity DO
#		select rowQuantity;
		SET document = (select concat('{"GNP": 414972, "Name": "Mexico", "IndepYear": 1810, "geography": {"Region": "Central America", "Continent": "North America", "SurfaceArea": 1958201}, "government": {"HeadOfState": "Enrique Penia Nieto", "GovernmentForm": "Federal Republic"}, "demographics": {"Population": 98881000, "LifeExpectancy": 71.5},',' "_id":"',id_k,'"}'));
		INSERT INTO world_x.bigdata_coll_sql VALUES (document,json_unquote(json_extract(doc, '$._id')));
		set id_k = id_k + 1;
    END WHILE;
END$$
DELIMITER ;

CREATE TABLE bigdata_coll_sql (doc JSON, _id VARCHAR(32) NOT NULL PRIMARY KEY);
CALL InsertInfoSQLColl(1000);

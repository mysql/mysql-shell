USE sakila;
DROP TABLE IF EXISTS example_SQLTABLE;
CREATE TABLE example_SQLTABLE (id INT, data VARCHAR(100) );
DROP TABLE IF EXISTS example_SQLTABLE;
SELECT * from information_schema.tables WHERE table_schema ='example_SQLTABLE';
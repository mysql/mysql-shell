Use sakila;
DROP VIEW IF EXISTS sql_viewtest;
create view sql_viewtest as select * from actor where first_name like '%as%';
DROP VIEW IF EXISTS sql_viewtest;
SELECT * from information_schema.views WHERE TABLE_NAME ='sql_viewtest';
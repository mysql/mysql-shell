Use sakila;
DROP VIEW IF EXISTS sql_viewtest;
create view sql_viewtest as select * from actor where first_name like '%as%';
alter view sql_viewtest as select count(*) as ActorQuantity from actor;
select * from sql_viewtest;
session.run_sql("use sakila;")
session.run_sql("DROP procedure IF EXISTS Test;")
session.run_sql("Create procedure Test() BEGIN SELECT count(*) as NumberOfCountries_Upd FROM sakila.country; END")
session.run_sql("call Test;")
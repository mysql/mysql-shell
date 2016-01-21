session.sql("DROP procedure IF EXISTS Test;").execute(); 
session.sql("Create procedure Test() BEGIN SELECT count(*) as NumberOfCountries_Upd3 FROM country; END").execute();
session.sql("call Test;").execute();
session.executeSql("DROP procedure IF EXISTS Test;"); 
session.executeSql("Create procedure Test() BEGIN SELECT count(*) as NumberOfCountries_Upd FROM country; END");
session.executeSql("call Test;");
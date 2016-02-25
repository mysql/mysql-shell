USE xshell_perf;
CREATE TABLE perf_mysql_js ( id INT NOT NULL AUTO_INCREMENT, stringCol VARCHAR(45) NOT NULL, datetimeCol DATETIME NOT NULL, blobCol BLOB NOT NULL, geometryCol GEOMETRY NOT NULL, PRIMARY KEY (id));
CALL InsertInfo(2);

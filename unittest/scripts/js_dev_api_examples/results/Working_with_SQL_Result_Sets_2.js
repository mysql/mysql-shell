
function print_result(res) {
  if (res.hasData()) {
    // SELECT
  } else {
    // INSERT, UPDATE, DELETE, ...
    print('Rows affected: ' + res.getAffectedRowCount());
  }
}

print_result(nodeSession.sql('DELETE FROM users WHERE age > 40').execute());
print_result(nodeSession.sql('SELECT COUNT(*) AS oldies FROM users WHERE age = 40').execute());

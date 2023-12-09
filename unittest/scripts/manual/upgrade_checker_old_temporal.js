// Tests handling for upgrade checker detecting old temporal types.
// This test requires to create data on server with version before 5.6.4
// and then upgrade to at least 5.7 to prepare the test data.

// Run with mysqlshrec -f <thisfile>, from the build dir.

var db_user = 'root'
var db_host = 'localhost'
var db_port = 3306

function fix_version(ver) {
    var idx = ver.indexOf('-');
    return idx < 0 ? ver : ver.substr(0, idx);
}

function compare_version_lt(left_version, right_version) {
    var l_ver = left_version.split('.');
    var r_ver = right_version.split('.');

    l_ver[2] = fix_version(l_ver[2]);

    if (Number(l_ver[0]) != Number(r_ver[0]))
        return Number(l_ver[0]) < Number(r_ver[0]);
    if (Number(l_ver[1]) != Number(r_ver[1]))
        return Number(l_ver[1]) < Number(r_ver[1]);
    if (Number(l_ver[2]) != Number(r_ver[2]))
        return Number(l_ver[2]) < Number(r_ver[2]);
    return false;
}

function check_sql_contains(result, column_name, column_type) {
    for (var i = 0; result.length; i++) {
        if (result[i].getField('column_name') == column_name &&
            result[i].getField('column_type') == column_type) {
            return true;
        }
    }
    return false;
}

println('Connecting to database');
shell.connect(`${db_user}@${db_host}:${db_port}`);

println('Checking database version...');
var db_ver_result = session.runSql('select version();').fetchOne()
var db_version = db_ver_result[0]

if (compare_version_lt(db_version, "5.6.4")) {
    println('Database version below 5.6.4 - correct for upgrade.');

    println('Creating test data...');

    session.runSql('drop schema if exists old_temporal_test;');
    session.runSql('create schema old_temporal_test;');

    session.runSql('use old_temporal_test;');

    session.runSql('create table old_temporal_table (ts_col TIMESTAMP, dt_col DATETIME, tm_col TIME);');
    session.runSql('insert into old_temporal_table VALUES(NOW(), NOW(), NOW());');

    println('Test data created.');

    println('Database can now be upgraded - preferably to version 5.7.');
    println('After upgrade, please run "upgrade_checker_old_temporal.js" again to continue testing.');
}
else {
    println('Database version above 5.6.4 - correct for testing.');

    println('Runing test, please verify output.');

    // EXPECTED OUTPUT: 
    // Warning (code 1287): '@@show_old_temporals' is deprecated and will be removed in a future release.
    session.runSql('set session show_old_temporals=ON');

    var db_result = session.runSql('select column_name, column_type from information_schema.columns where table_name=\'old_temporal_table\';');
    var results = db_result.fetchAll();

    if (check_sql_contains(results, 'ts_col', 'timestamp /* 5.5 binary format */') &&
        check_sql_contains(results, 'dt_col', 'datetime /* 5.5 binary format */') &&
        check_sql_contains(results, 'tm_col', 'time /* 5.5 binary format */')) {
        println('Database verified to contain proper test data.');
    }
    else {
        testutil.fail('Database does not contain test data. Test will fail.');
    }

    println('Running util.checkForServerUpgrade...');

    util.checkForServerUpgrade();

    println('Output should contain:');
    println('old_temporal_test.old_temporal_table.ts_col - timestamp /* 5.5 binary format */');
    println('old_temporal_test.old_temporal_table.dt_col - datetime /* 5.5 binary format */');
    println('old_temporal_test.old_temporal_table.tm_col - time /* 5.5 binary format */');

    println('Test ended, please verify results.');
}


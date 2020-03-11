


function md5_table(session, schema, table) {
    cols=session.runSql("desc `"+schema+"`.`"+table+"`").fetchAll();
    columns="";
    has_pri=false;
    for(c in cols) {
        if(cols[c][3]=="PRI")
            has_pri=true;
        columns+=", convert("+cols[c][0]+" using binary)";
    }
    query="SELECT min(least(length(@crc := sha1(concat(@crc, sha1(concat_ws('#'"+columns+"))))),"+
        "@cnt := @cnt + 1)) as discard from `"+schema+"`.`"+table+"`";
    if(has_pri) query+=" use index(PRIMARY)";
    session.runSql(query);
    row=session.runSql("select @crc, @cnt").fetchOne();

    return {"sha1": row[0], "count": row[1]};
}

function snapshot_account(session, auser, ahost) {
    user={}
    user["create"]=session.runSql("SHOW CREATE USER ?@?", [auser, ahost]).fetchOne()[0];
    user["grants"]=[];
    gres=session.runSql("SHOW GRANTS FOR ?@?", [auser, ahost]);
    row=gres.fetchOne();
    while(row) {
        user["grants"].push(row[0]);
        row=gres.fetchOne();
    }
    return user;
}

function snapshot_accounts(session) {
    accounts={};

    var res=session.runSql("SELECT user,host FROM mysql.user").fetchAll();
    for(r in res) {
        user=snapshot_account(session, res[r][0], res[r][1]);
        name=res[r][0]+"@"+res[r][1];
        accounts[name]=user;
    }

    return accounts;
}


function snapshot_table_data(session, schema, table) {
    // CHECKSUM TABLE returns different values for floating point values when they're loaded with LOAD DATA
    // Bug#31071891
    // cksum = session.runSql("CHECKSUM TABLE `"+schema+"`.`"+table+"`").fetchOne()[1];
    // count = session.runSql("SELECT count(*) FROM `"+schema+"`.`"+table+"`").fetchOne()[0];
    // return {"checksum":cksum, "rowcount":count};

    return md5_table(session, schema, table);
}

function snapshot_tables_and_triggers(session, schema) {
    var tables={};

    var names=session.runSql("SELECT TABLE_NAME FROM information_schema.tables WHERE table_schema = ? and table_type = 'BASE TABLE'", [schema]).fetchAll();
    for(t in names) {
        var tname=names[t]["TABLE_NAME"];

        tables[tname]=snapshot_table_data(session, schema, tname);

        tables[tname]["ddl"]=session.runSql("SHOW CREATE TABLE `"+schema+"`.`"+tname+"`").fetchOne()[1];
        // on Windows "SHOW CREATE" keeps adding slashes at the end of a DATA|INDEX directory, mitigate that
        tables[tname]["ddl"] = tables[tname]["ddl"].replace(/\/+/g, "/")

        var res=session.runSql("SELECT trigger_name FROM information_schema.triggers WHERE trigger_schema = ? and event_object_table = ?", [schema, tname]).fetchAll();
        var triggers={};
        for(tr in res) {
            var trname=res[tr][0];
            triggers[trname]=session.runSql("SHOW CREATE TRIGGER `"+schema+"`.`"+trname+"`").fetchOne()[1];
        }
        tables[tname]["triggers"]=triggers;
    }

    return tables;
}

function snapshot_views(session, schema) {
    var tables={};

    var names=session.runSql("SELECT TABLE_NAME FROM information_schema.tables WHERE table_schema = ? and table_type = 'VIEW'", [schema]).fetchAll();
    for(t in names) {
        var tname=names[t]["TABLE_NAME"];

        tables[tname]={};

        tables[tname]["ddl"]=session.runSql("SHOW CREATE VIEW `"+schema+"`.`"+tname+"`").fetchOne()[1];
    }

    return tables;
}

function snapshot_procedures(session, schema) {
    var procedures={};

    var names=session.runSql("SELECT * FROM information_schema.routines WHERE routine_schema = ? and routine_type = 'PROCEDURE'", [schema]).fetchAll();
    for(t in names) {
        var tname=names[t]["ROUTINE_NAME"];

        procedures[tname]={};

        procedures[tname]["ddl"]=session.runSql("SHOW CREATE PROCEDURE `"+schema+"`.`"+tname+"`").fetchOne()[1];
    }

    return procedures;
}

function snapshot_functions(session, schema) {
    var functions={};

    var names=session.runSql("SELECT * FROM information_schema.routines WHERE routine_schema = ? and routine_type = 'FUNCTION'", [schema]).fetchAll();
    for(t in names) {
        var tname=names[t]["ROUTINE_NAME"];

        functions[tname]={};

        functions[tname]["ddl"]=session.runSql("SHOW CREATE FUNCTION `"+schema+"`.`"+tname+"`").fetchOne()[1];
    }

    return functions;
}

function snapshot_events(session, schema) {
    var events={};

    var names=session.runSql("SHOW EVENTS IN `"+schema+"`").fetchAll();
    for(t in names) {
        var tname=names[t]["Name"];

        events[tname]={};

        events[tname]["ddl"]=session.runSql("SHOW CREATE EVENT `"+schema+"`.`"+tname+"`").fetchOne()[1];
    }

    return events;
}


function snapshot_schema(session, schema) {
    var obj={};
    obj["tables"]=snapshot_tables_and_triggers(session, schema);
    obj["views"]=snapshot_views(session, schema);
    obj["procedures"]=snapshot_procedures(session, schema);
    obj["functions"]=snapshot_functions(session, schema);
    obj["events"]=snapshot_events(session, schema);
    return obj;
}

function snapshot_schemas(session) {
    var schemas={};
    var names=session.runSql("SELECT * FROM information_schema.schemata WHERE schema_name not in ('sys', 'information_schema', 'mysql', 'performance_schema')").fetchAll();
    for(s in names) {
        var sname=names[s]["SCHEMA_NAME"];

        ddl=session.runSql("SHOW CREATE SCHEMA `"+sname+"`").fetchOne()[1];

        schemas[sname]=snapshot_schema(session, sname);

        schemas[sname]["ddl"]=ddl;
    }

    return schemas;
}


function snapshot_tablespaces(session) {
    // not supported atm
    return {};
}


function snapshot_instance(session) {
    var snapshot={};

    snapshot["accounts"]=snapshot_accounts(session);

    snapshot["tablespaces"]=snapshot_tablespaces(session);

    snapshot["schemas"]=snapshot_schemas(session);

    // normalize the JSON encoding
    return JSON.parse(JSON.stringify(snapshot));
}


function strip_keys(obj, keys) {
    obj=Object.assign({}, obj);
    for(k in keys) {
        delete obj[keys[k]];
    }
    return obj;
}

function strip_snapshot_data(snapshot) {
    copy=JSON.parse(JSON.stringify(snapshot));
    for(s in copy["schemas"]) {
        var tables=copy["schemas"][s]["tables"];
        for(t in tables) {
            delete tables[t]["count"];
            delete tables[t]["sha1"];
        }
    }
    return copy;
}

function wipe_instance(session) {
    var r=session.runSql("SHOW SCHEMAS");
    var rows=r.fetchAll();
    for(var i in rows) {
        var row=rows[i];
        if(["mysql", "performance_schema", "sys", "information_schema"].includes(row[0]))
            continue;
        session.runSql("DROP SCHEMA "+row[0]);
    }
    var r=session.runSql("SELECT user,host FROM mysql.user");
    var rows=r.fetchAll();
    for(var i in rows) {
        var row=rows[i];
        if(["mysql.sys", "mysql.session", "mysql.infoschema"].includes(row[0]))
            continue;
        if(row[0]=="root"&&(row[1]=="localhost"||row[1]=="%"))
            continue;
        session.runSql("DROP USER ?@?", [row[0], row[1]]);
    }
    session.runSql("RESET MASTER");
    session.runSql("RESET SLAVE ALL");
}

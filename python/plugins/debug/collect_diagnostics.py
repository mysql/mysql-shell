# Copyright (c) 2021, 2022, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

from mysqlsh import globals
from mysqlsh import mysql, DBError, Error
import socket
import zipfile
import yaml
import os
import time
import sys

from debug.host_info import collect_ping_stats


def repr_yaml_text(self, tag, value, style=None):
    if style is None:
        if "\n" in value:
            style = '|'
        else:
            style = self.default_style

    node = yaml.representer.ScalarNode(tag, value, style=style)
    if self.alias_key is not None:
        self.represented_objects[self.alias_key] = node
    return node


yaml.representer.BaseRepresenter.represent_scalar = repr_yaml_text


def dump_query(zf, fn, session, query, args=[], *, as_yaml=True,
               as_tsv=True, labels=True, filter=None, ignore_errors=False, include_timing=False):
    try:
        start_time = time.time()
        r = session.run_sql(query, args)
        end_time = time.time()
        all_rows = r.fetch_all()
    except Exception as e:
        if ignore_errors:
            with zf.open(fn+".error", "w") as f:
                f.write(f"{query}\n{e}\n".encode("utf-8"))
            return
        print(f"ERROR: While executing {query}: {e}")
        raise

    yaml_out = []
    tsv_out = []
    if labels and as_tsv:
        line = []
        for c in r.columns:
            line.append(c.column_label)
        tsv_out.append("# " + "\t".join(line))

    for row in all_rows:
        line = []
        if filter and not filter(row):
            continue
        entry = {}
        for i in range(len(row)):
            line.append(str(row[i]) if row[i] is not None else "NULL")
            if type(row[i]) in (int, str, bool, type(None), float):
                entry[r.columns[i].column_label] = row[i]
            else:
                entry[r.columns[i].column_label] = str(row[i])
        if as_tsv:
            tsv_out.append("\t".join(line))
        yaml_out.append(entry)

    if include_timing:
        yaml_out.append({"Execution Time": end_time-start_time})
        if as_tsv:
            tsv_out.append(f"# Execution Time: {end_time-start_time}")

    if as_tsv:
        with zf.open(fn+".tsv", "w") as f:
            f.write("\n".join(tsv_out).encode("utf-8"))
            f.write(b"\n")

    if as_yaml:
        with zf.open(fn+".yaml", "w") as f:
            f.write(yaml.dump_all(yaml_out).encode("utf-8"))

    return yaml_out


def dump_table(zf, fn, session, table, *, as_yaml=False, filter=None,
               ignore_errors=False):
    return dump_query(zf, fn, session, f"select * from {table}", filter=filter,
                      as_yaml=as_yaml, ignore_errors=ignore_errors)


def get_instance_id(session):
    try:
        return session.run_sql(
            "select instance_id from mysql_innodb_cluster_metadata.instances where mysql_server_uuid=@@server_uuid").fetch_one()[0]
    except Error as e:
        if e.code == mysql.ErrorCode.ER_BAD_DB_ERROR or e.code == mysql.ErrorCode.ER_NO_SUCH_TABLE:
            return 0  # no MD
        raise


def collect_metadata(zf, prefix, session, ignore_errors):
    try:
        r = session.run_sql(
            "show full tables in mysql_innodb_cluster_metadata").fetch_all()
    except Error as e:
        if e.code == mysql.ErrorCode.ER_BAD_DB_ERROR or e.code == mysql.ErrorCode.ER_NO_SUCH_TABLE:
            return None
        if ignore_errors:
            print(f"ERROR: Could not query InnoDB Cluster metadata: {e}")
            with zf.open(f"{prefix}/mysql_innodb_cluster_metadata.error", "w") as f:
                f.write(f"{e}\n".encode("utf-8"))
            return None
        raise

    print("Dumping mysql_innodb_cluster_metadata schema...")
    for row in r:
        if row[1] != "BASE TABLE" and row[0] not in ("schema_version", ):
            continue
        table = row[0]
        dump_table(
            zf, f"{prefix}/mysql_innodb_cluster_metadata.{table}", session,
            f"mysql_innodb_cluster_metadata.{table}", as_yaml=True,
            ignore_errors=ignore_errors)


def get_topology_members(session):
    return [(r[0], r[1]) for r in session.run_sql(
        "select instance_id, addresses->>'$.mysqlClassic' from mysql_innodb_cluster_metadata.instances").fetch_all()]


def collect_queries(zf, prefix, fn, session, queries, *, as_yaml=False,
                    ignore_errors=False, include_timing=False):
    prefix = prefix + "/" + (fn + "." if fn else "")
    info = {}
    for q in queries:
        if type(q) is tuple:
            label, query = q
        else:
            label, query = q, q
        print(f" - Gathering {label}...")
        info[label.replace(' ', '_')] = dump_query(zf, f"{prefix}{label.replace(' ', '_')}",
                                                   session, query, as_yaml=as_yaml,
                                                   ignore_errors=ignore_errors,
                                                   include_timing=include_timing)
    return info


def collect_innodb_metrics(zf, prefix, fn, session, innodb_mutex,
                           ignore_errors):
    queries = [("innodb_status", "SHOW ENGINE INNODB STATUS"),
               ("innodb_metrics", "SELECT * FROM information_schema.innodb_metrics")
               ]
    if innodb_mutex:
        queries += [
            ("innodb_mutex", "SHOW ENGINE INNODB MUTEX")
        ]

    print(f"Collecting InnoDB Metrics")
    return collect_queries(zf, prefix, fn, session, queries,
                           as_yaml=True, ignore_errors=ignore_errors)


def get_version_num(session):
    version = session.run_sql("select @@version").fetch_one()[0]
    if "-" in version:
        version = version.split("-")[0]
    a, b, c = version.split(".")
    return int(a)*10000 + int(b)*100 + int(c)


def collect_member_info(zf, prefix, fn, session, slow_queries, ignore_errors):
    version = get_version_num(session)

    info = {}

    try:
        tables = [t[0] for t in session.run_sql(
            "show tables in performance_schema like 'replication_%'").fetch_all()]
    except Exception as e:
        print(f"ERROR: Could not list tables in performance_schema: {e}")
        if ignore_errors:
            with zf.open(f"{prefix}/performance_schema.error", "w") as f:
                f.write(
                    f"show tables in performance_schema like 'replication_%'\n{e}\n".encode("utf-8"))
            return
        raise

    queries = [
        "SHOW BINARY LOGS",
        "SHOW SLAVE HOSTS",
        "SHOW MASTER STATUS",
        "SHOW GLOBAL STATUS",
        "SHOW PLUGINS",
        ("global variables", """SELECT g.variable_name name, g.variable_value value /*!80000, i.variable_source source*/
        FROM performance_schema.global_variables g
        /*!80000 JOIN performance_schema.variables_info i ON g.variable_name = i.variable_name */
        ORDER BY name""")
    ]

    if version >= 80000:
        if slow_queries:
            queries.append(("slow_log", "SELECT * FROM mysql.slow_log"))
        tables.append("persisted_variables")
        queries.append(("processlist", "SELECT * FROM sys.processlist"))
    else:
        queries.append("SHOW PROCESSLIST")

    with zf.open(f"{prefix}/{fn}.uri", "w") as f:
        f.write(f"{session.uri}\n".encode("utf-8"))

    for table in tables:
        print(f" - Gathering {table}...")
        info[table] = dump_table(zf, f"{prefix}/{fn}.{table}", session,
                                 "performance_schema." + table, as_yaml=True,
                                 ignore_errors=ignore_errors)

    info.update(collect_queries(zf, prefix, fn, session, queries,
                                as_yaml=True, ignore_errors=ignore_errors))

    if version >= 80022:
        def filter_pwd(row):
            if "temporary password" in row[5]:
                return None
            return row
        print(" - Gathering error_log")
        dump_table(zf, f"{prefix}/{fn}.error_log", session, "performance_schema.error_log",
                   filter=filter_pwd, as_yaml=False, ignore_errors=ignore_errors)
    else:
        print(
            f"MySQL error logs could not be collected for {session.uri}, please include error.log files if reporting bugs or seeking assistance")

    return info


def dump_accounts(zf, prefix, session, ignore_errors=False):
    accounts = session.run_sql(
        "select user,host from mysql.user where user like 'mysql_innodb_%'").fetch_all()
    with zf.open(prefix+"/cluster_accounts.tsv", "w") as f:
        for row in accounts:
            user, host = row[0], row[1]
            f.write(f"-- {user}@{host}\n".encode("utf-8"))
            try:
                for r in session.run_sql("show grants for ?@?", args=[user, host]).fetch_all():
                    f.write((r[0]+"\n").encode("utf-8"))
            except Exception as e:
                if ignore_errors:
                    print(
                        f"WARNING: Error getting grants for {user}@{host}: {e}")
                    f.write(
                        f"Could not get grants for {user}@{host}: {e}\n".encode("utf-8"))
                else:
                    print(
                        f"ERROR: Error getting grants for {user}@{host}: {e}")
                    raise
            f.write(b"\n")


def collect_schema_stats(zf, prefix, session, full=False, ignore_errors=False):
    if full:
        print(f"Collecting Schema Statistics")
    queries = [
        ("tables without a PK", """SELECT t.table_schema, t.table_name
            FROM information_schema.tables t
              LEFT JOIN information_schema.statistics s on t.table_schema=s.table_schema and t.table_name=s.table_name and s.index_name='PRIMARY'
            WHERE s.index_name is NULL and t.table_type = 'BASE TABLE'
                and t.table_schema not in ('performance_schema', 'sys', 'mysql', 'information_schema')""")
    ]
    if full:
        queries += [
            ("schema object overview", "select * from sys.schema_object_overview"),
            ("top biggest tables", """select t.table_schema, t.table_name, t.row_format, t.table_rows, t.avg_row_length, t.data_length, t.max_data_length, t.index_length, t.table_collation,
        json_objectagg(idx.index_name, json_object('columns', idx.col, 'type', idx.index_type, 'cardinality', idx.cardinality)) indexes,
        group_concat((select concat(c.column_name, ':', c.column_type)
          from information_schema.columns c
          where c.table_schema = t.table_schema and c.table_name = t.table_name and c.column_type in ('blob'))) blobs
    from information_schema.tables t
    join (select s.table_schema, s.table_name, s.index_name, s.index_type, s.cardinality, json_arrayagg(concat(c.column_name, ':', c.column_type)) col
          from information_schema.statistics s left join information_schema.columns c on s.table_schema=c.table_schema and s.table_name=c.table_name and s.column_name=c.column_name
          group by s.table_schema, s.table_name, s.index_name, s.index_type, s.cardinality
          order by s.table_schema, s.table_name, s.index_name, s.index_type, s.cardinality) idx
    on idx.table_schema=t.table_schema and idx.table_name = t.table_name
    where t.table_type = 'BASE TABLE' and t.table_schema not in ('mysql', 'information_schema', 'performance_schema')
    group by t.table_schema, t.table_name, t.engine, t.row_format, t.table_rows, t.avg_row_length, t.data_length, t.max_data_length, t.index_length, t.table_collation
    order by t.data_length desc limit 20""")
        ]
    collect_queries(zf, prefix, None, session, queries,
                    as_yaml=True, ignore_errors=ignore_errors)


def collect_basic_diagnostics(zf, prefix, session, info={}, shell_logs=True):
    def to_dict(options):
        out = {}
        for k in dir(options):
            if not callable(options[k]):
                if type(options[k]) in (str, int, float, bool):
                    out[k] = options[k]
                else:
                    out[k] = str(options[k])
        return out

    shell = globals.shell

    shell_info = dict(info)
    shell_info["version"] = shell.version
    shell_info["options"] = to_dict(shell.options)
    shell_info["destination"] = session.uri
    shell_info["hostname"] = socket.gethostname()

    with zf.open(prefix+"/shell_info.yaml", "w") as f:
        f.write(yaml.dump(shell_info).encode("utf-8"))

    if shell_logs:
        print("Copying shell log file...")
        if shell.options["logFile"] and os.path.exists(shell.options["logFile"]):
            logs = open(shell.options["logFile"]).read()
            with zf.open(prefix+"/mysqlsh.log", "w") as f:
                f.write(logs.encode("utf-8"))


def collect_topology_diagnostics(zf, prefix, session, creds, innodb_mutex=False,
                                 slow_queries=False, ignore_errors=False):
    instance_data = []
    members = get_topology_members(session)
    for instance_id, endpoint in members:
        data = {
            "instance_id": instance_id,
            "endpoint": endpoint
        }
        print(f"Collecting information from {endpoint}...")
        try:
            dest = dict(creds)
            dest["host"], dest["port"] = endpoint.split(":")

            session = mysql.get_session(dest)
        except Error as e:
            print(f"Could not connect to {endpoint}: {e}")
            with zf.open(f"{prefix}/{instance_id}.connect_error.txt", "w") as f:
                f.write((str(e)+"\n").encode("utf-8"))
            if e.code == mysql.ErrorCode.ER_ACCESS_DENIED_ERROR:
                raise
            continue

        info = collect_member_info(
            zf, prefix, f"{instance_id}", session, slow_queries=slow_queries,
            ignore_errors=ignore_errors)
        data["member_info"] = info

        info = collect_innodb_metrics(
            zf, prefix, f"{instance_id}", session, innodb_mutex=innodb_mutex,
            ignore_errors=ignore_errors)
        data["innodb_info"] = info
        print()

        instance_data.append(data)

    return instance_data


def dump_cluster_status(zf, prefix):
    try:
        status = globals.dba.get_cluster().status({"extended": 2})
        with zf.open(f"{prefix}/cluster_status.yaml", "w") as f:
            f.write(yaml.dump(eval(repr(status))).encode("utf-8"))
    except Exception as e:
        with zf.open(f"{prefix}/cluster_status.error", "w") as f:
            f.write(f"{e}\n".encode("utf-8"))
    try:
        status = globals.dba.get_cluster_set().status({"extended": 2})
        with zf.open(f"{prefix}/cluster_set_status.yaml", "w") as f:
            f.write(yaml.dump(eval(repr(status))).encode("utf-8"))
    except Exception as e:
        with zf.open(f"{prefix}/cluster_set_status.error", "w") as f:
            f.write(f"{e}\n".encode("utf-8"))
    try:
        status = globals.dba.get_replica_set().status({"extended": 2})
        with zf.open(f"{prefix}/replica_set_status.yaml", "w") as f:
            f.write(yaml.dump(eval(repr(status))).encode("utf-8"))
    except Exception as e:
        with zf.open(f"{prefix}/replica_set_status.error", "w") as f:
            f.write(f"{e}\n".encode("utf-8"))


def default_filename():
    return f"mysql-diagnostics-{time.strftime('%Y%m%d-%H%M%S')}.zip"


def diff_multi_tables(topology_info, get_data, get_key, get_value):
    fields = {}
    different_fields = {}
    for i, instance in enumerate(topology_info):
        for row in get_data(instance):
            key = get_key(row)
            value = get_value(row)
            if key not in fields:
                fields[key] = [None] * i + [value]
                if i > 0:
                    different_fields[key] = True
            else:
                if key not in different_fields:
                    if fields[key][-1] != value:
                        different_fields[key] = True
                fields[key].append(value)
    diff = []
    for key in different_fields.keys():
        diff.append((key, fields[key]))

    return diff


def dump_diff(f, key_label, instance_labels, diff, header):
    def get_column_widths(data):
        column_widths = [0] * len(instance_labels)
        for key, values in data:
            for i, v in enumerate(values):
                column_widths[i] = max(column_widths[i], len(v))
        return column_widths

    if header:
        f.write(f"# {header}\n".encode("utf-8"))

    # find widths of each column
    column_widths = [len(key_label)]
    for k, v in diff:
        column_widths[0] = max(column_widths[0], len(k))
    column_widths += get_column_widths(diff)

    line = [key_label.ljust(column_widths[0])]
    for i, label in enumerate(instance_labels):
        line.append(label.ljust(column_widths[1+i]))
    h = (" | ".join(line) + "\n")
    f.write(h.encode("utf-8"))
    f.write(("="*len(h)+"\n").encode("utf-8"))
    for k, v in diff:
        line = [k.ljust(column_widths[0])]
        for i, value in enumerate(v):
            line.append(value.ljust(column_widths[1+i]))
        f.write((" | ".join(line) + "\n").encode("utf-8"))


def analyze_topology_data(zf, prefix, topology_info):
    diff = diff_multi_tables(topology_info,
                             lambda i: i["member_info"]["global_variables"],
                             lambda row: row["name"], lambda row: row["value"])

    instance_names = []
    for i in topology_info:
        instance_names.append(i["endpoint"])

    with zf.open(f"{prefix}/diff.global_variables.txt", "w") as f:
        if diff:
            dump_diff(
                f, "Variable", instance_names, diff, f"{len(diff)} differing global variables between instances")


def analyze_instance_data(zf, prefix, instance_info):
    pass


def do_collect_diagnostics(session, path, orig_args, innodbMutex=False, allMembers=False,
                           schemaStats=False, slowQueries=False,
                           ignoreErrors=False):
    if not path.lower().endswith(".zip"):
        if path.endswith("/") or (sys.platform == "win32" and path.endswith("\\")):
            path = os.path.join(path, default_filename())
        else:
            path += ".zip"

    prefix = os.path.basename(path.replace("\\", "/"))[:-4]

    if os.path.exists(path):
        raise Error(path+" already exists")

    if get_version_num(session) < 50700:
        raise Error("MySQL 5.7 or newer required")

    if slowQueries:
        row = session.run_sql(
            "select @@slow_query_log, @@log_output").fetch_one()
        if not row[0] or row[1] != "TABLE":
            raise Error(
                "slowQueries option requires slow_query_log to be enabled and log_output to be set to TABLE")

    shell = globals.shell

    try:
        my_instance_id = get_instance_id(session)
    except Exception as e:
        if ignoreErrors:
            if allMembers:
                print(
                    f"ERROR querying InnoDB Cluster metadata. Cannot ignore error because allMembers is enabled: {e}")
                raise
            else:
                print(
                    f"ERROR querying InnoDB Cluster metadata: {e}")
            my_instance_id = 0
        else:
            raise

    creds = None
    if allMembers and my_instance_id:
        creds = shell.parse_uri(session.uri)
        creds["password"] = shell.prompt(
            f"Password for {creds['user']}: ", {"type": "password"})

    print(f"Collecting diagnostics information from {session.uri}...")
    try:
        with zipfile.ZipFile(path, mode="w") as zf:
            collect_metadata(zf, prefix, session, ignore_errors=ignoreErrors)
            collect_basic_diagnostics(zf, prefix, session,
                                      info={"command": "collectDiagnostics",
                                            "commandOptions": orig_args})
            collect_schema_stats(zf, prefix, session, full=schemaStats,
                                 ignore_errors=ignoreErrors)

            topology_data = None
            collected = False
            if my_instance_id:
                print("Gathering grants for mysql_innodb_% accounts...")
                dump_accounts(zf, prefix, session,
                              ignore_errors=ignoreErrors)

                print("Gathering cluster status...")
                dump_cluster_status(zf, prefix)

                if allMembers and my_instance_id:
                    collected = True
                    topology_data = collect_topology_diagnostics(
                        zf, prefix, session, creds, innodb_mutex=innodbMutex,
                        slow_queries=slowQueries, ignore_errors=ignoreErrors)

                    analyze_topology_data(zf, prefix, topology_data)
            elif my_instance_id is None:
                # None means get_instance_id() failed with error
                my_instance_id = 0

            if not collected:
                instance_data = {}
                info = collect_member_info(zf, prefix, str(my_instance_id),
                                           session, slow_queries=slowQueries,
                                           ignore_errors=ignoreErrors)
                instance_data["member_info"] = info
                info = collect_innodb_metrics(
                    zf, prefix, str(my_instance_id), session,
                    innodb_mutex=innodbMutex, ignore_errors=ignoreErrors)
                instance_data["innodb_info"] = info
                analyze_instance_data(zf, prefix, instance_data)

            # collect local host info if we're connected to localhost
            target = shell.parse_uri(session.uri)
            if topology_data:
                if "host" not in target or target["host"] in ("127.0.0.1", "localhost"):
                    print("Connected to local server, collecting ping stats...")
                    collect_ping_stats(zf, prefix, topology_data)
                else:
                    print(
                        "Connected to remote server, ping stats not be collected.")
    except:
        if os.path.exists(path):
            os.remove(path)
        print()
        print("An error occurred during data collection. Partial output deleted.")
        raise

    print()
    print(f"Diagnostics information was written to {path}")

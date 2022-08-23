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

from mysqlsh.plugin_manager import plugin, plugin_function
from mysqlsh import globals
from mysqlsh import Error, DBError

from debug.collect_diagnostics import do_collect_diagnostics, do_collect_high_load_diagnostics, do_collect_slow_query_diagnostics

@plugin(parent="util")
class debug:
    """
    Debugging and diagnostic utilities.
    """


@plugin_function("util.debug.collectDiagnostics", cli=True)
def collect_diagnostics(path: str, **options):
    """Collects MySQL diagnostics information for standalone and managed
    topologies

    A zip file containing diagnostics information collected from the server
    connected to and other members of the same topology.

    The following information is collected

    General
    @li Error logs from performance_schema (if available)
    @li Shell logs, configuration options and hostname where it is executed
    @li InnoDB locks and metrics
    @li InnoDB storage engine status and metrics
    @li Names of tables without a Primary Key
    @li Slow queries (if enabled)
    @li Performance Schema configuration

    Replication/InnoDB Cluster
    @li mysql_innodb_cluster_metadata schema and contents
    @li Replication related tables in performance_schema
    @li Replication status information
    @li InnoDB Cluster accounts and their grants
    @li InnoDB Cluster metadata schema
    @li Output of ping for 5s
    @li NDB Thread Blocks

    Schema Statistics
    @li Number of schema objects (sys.schema_table_overview)
    @li Top 20 biggest tables with index, blobs and partitioning information

    All members of a managed topology (e.g. InnoDB Cluster) will be scanned by
    default. If the `allMembers` option is set to False, then only the member
    the Shell is connected to is scanned.

    Args:
        path (str): path to write the zip file with diagnostics information
        **options (dict): Optional arguments

    Keyword Args:
        allMembers (bool): If true, collects information of all members of the
            topology, plus ping stats. Default false.
        innodbMutex (bool): If true, also collects output of
            SHOW ENGINE INNODB MUTEX. Disabled by default, as this command can
            have some impact on production performance.
        schemaStats (bool): If true, collects schema size statistics.
            Default false.
        slowQueries (bool): If true, collects slow query information.
            Requires slow_log to be enabled and configured for TABLE output.
            Default false.
        ignoreErrors (bool): If true, ignores query errors during collection.
            Default false.
        customSql (list): Custom list of SQL statements to execute.
        customShell (list): Custom list of shell commands to execute.
    """
    if not globals.shell.get_session():
        raise Error(
            "Shell must be connected to a member of the desired MySQL topology.")

    session = globals.shell.open_session()

    do_collect_diagnostics(session, path, orig_args=options, **options)


@plugin_function("util.debug.collectHighLoadDiagnostics", cli=True)
def collect_high_load_diagnostics(path: str, **options):
    """Collects MySQL high load diagnostics information

    A zip file containing diagnostics information collected from the server
    connected to.

    The following information is collected

    General
    @li Error logs from performance_schema or error log file (if available)
    @li Shell configuration options and hostname where it is executed
    @li Names of tables without a Primary Key
    @li Benchmark info (SELECT BENCHMARK())
    @li Process list, open tables, host cache
    @li System info collected from the OS (if connected to localhost)

    Schema Statistics
    @li Number of schema objects (sys.schema_table_overview)
    @li Full list of tables with basic stats
    @li Top 20 biggest tables with index, blobs and partitioning information
    @li Stored procedure and function stats

    The following performance metrics are collected multiple times, as specified
    by the `iterations` option.

    @li InnoDB locks and metrics
    @li InnoDB storage engine status and metrics
    @li InnoDB buffer pool stats
    @li InnoDB transaction info

    If the `pfsInstrumentation` option is set to a value other than 'current',
    additional PERFORMANCE_SCHEMA instruments and consumers are enabled for the
    duration of the collection. If set to 'full', all instruments and consumers
    are enabled. If set to 'medium', non-history consumers and instruments
    other than wait/synch/% are enabled. Enabling additional instrumentation
    may provide additional insights, at the cost of an larger impact on server
    performance.

    Args:
        path (str): path to write the zip file with diagnostics information
        **options (dict): Optional arguments

    Keyword Args:
        iterations (int): Number of iterations to collect perf stats (default 2)
        delay (int): Number of seconds to wait between iterations
            (default 5min)
        innodbMutex (bool): If true, also collects output of
            SHOW ENGINE INNODB MUTEX. Disabled by default, as this command can
            have some impact on production performance.
        pfsInstrumentation (string): One of current, medium, full. Controls
            whether additional PERFORMANCE_SCHEMA instruments and consumers are
            temporarily enabled (default 'current').
        customSql (list): Custom list of SQL statements to execute.
            If the statement is prefixed with `before:` or nothing, it will be
            executed once, before the metrics collection loop. If prefixed with
            `after:`, it will be executed after the loop. If prefixed with
            `during:`, it will be executed once for each iteration of the
            collection loop.
        customShell (list): Custom list of shell commands to execute.
            If the command is prefixed with `before:` or nothing, it will be
            executed once, before the metrics collection loop. If prefixed with
            `after:`, it will be executed after the loop. If prefixed with
            `during:`, it will be executed once for each iteration of the
            collection loop.
    """
    if not globals.shell.get_session():
        raise Error(
            "Shell must be connected to the MySQL server to be diagnosed.")

    session = globals.shell.open_session()

    do_collect_high_load_diagnostics(
        session, path, orig_args=options, **options)


@plugin_function("util.debug.collectSlowQueryDiagnostics", cli=True)
def collect_slow_query_diagnostics(path: str, query: str, **options):
    """Collects MySQL diagnostics and profiling information for a slow query

    A zip file containing diagnostics information collected from the server
    connected to.

    The given query is executed once. Performance metrics are collected
    during execution of the query.

    The following information is collected, in addition to what's collected
    by `util.debug.collectHighLoadDiagnostics()`

    @li EXPLAIN output of the query
    @li Optimizer trace of the query
    @li DDL of tables used in the query
    @li Warnings generated by the query

    If the `pfsInstrumentation` option is set to a value other than 'current',
    additional PERFORMANCE_SCHEMA instruments and consumers are enabled for the
    duration of the collection. If set to 'full', all instruments and consumers
    are enabled. If set to 'medium', non-history consumers and instruments
    other than wait/synch/% are enabled. Enabling additional instrumentation
    may provide additional insights, at the cost of an larger impact on server
    performance.

    Args:
        path (str): path to write the zip file with diagnostics information
        query (str): query to be analyzed
        **options (dict): Optional arguments

    Keyword Args:
        delay (int): Number of seconds to wait between collection iterations
            (default 5s)
        innodbMutex (bool): If true, also collects output of
            SHOW ENGINE INNODB MUTEX. Disabled by default, as this command can
            have some impact on production performance.
        pfsInstrumentation (string): One of current, medium, full. Controls
            whether additional PERFORMANCE_SCHEMA instruments and consumers are
            temporarily enabled (default 'current').
        customSql (list): Custom list of SQL statements to execute.
            If the statement is prefixed with `before:` or nothing, it will be
            executed once, before the metrics collection loop. If prefixed with
            `after:`, it will be executed after the loop. If prefixed with
            `during:`, it will be executed once for each iteration of the
            collection loop.
        customShell (list): Custom list of shell commands to execute.
            If the command is prefixed with `before:` or nothing, it will be
            executed once, before the metrics collection loop. If prefixed with
            `after:`, it will be executed after the loop. If prefixed with
            `during:`, it will be executed once for each iteration of the
            collection loop.
    """

    if not globals.shell.get_session():
        raise Error(
            "Shell must be connected to a MySQL server.")

    session = globals.shell.open_session()

    do_collect_slow_query_diagnostics(
        session, path, query, orig_args=options, **options)

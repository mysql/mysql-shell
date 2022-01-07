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

from mysqlsh.plugin_manager import plugin_function
from mysqlsh import globals
from mysqlsh import Error, DBError

from debug.collect_diagnostics import do_collect_diagnostics


@plugin_function("util.debug.collectDiagnostics", cli=True)
def collect_diagnostics(path, **options):
    """Collects MySQL diagnostics information for standalone and managed
    topologies

    A zip file containing diagnostics information collected from the server
    connected to.

    The following information is collected

    General
    @li Error logs from performance_schema (if available)
    @li Shell logs, configuration options and hostname where it is executed
    @li InnoDB locks and metrics
    @li InnoDB storage engine status and metrics
    @li Names of tables without a Primary Key
    @li Slow queries (if enabled)

    Replication/InnoDB Cluster
    @li mysql_innodb_cluster_metadata schema and contents
    @li Replication related tables in performance_schema
    @li Replication status information
    @li InnoDB Cluster accounts and their grants
    @li InnoDB Cluster metadata schema
    @li Output of ping for 5s

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
    """
    session = globals.shell.get_session()

    if not session:
        raise Error(
            "Shell must be connected to a member of the desired MySQL topology.")

    do_collect_diagnostics(session, path, orig_args=options, **options)

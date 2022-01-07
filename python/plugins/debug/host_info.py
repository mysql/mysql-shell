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

import zipfile
import socket
import threading
import subprocess
from mysqlsh import globals
import sys


k_host_info_cmds_posix = ["date",
                          "uname -a",
                          "getenforce",
                          "free -m",
                          "swapon -s",
                          "lsb_release -a",
                          "mount -v",
                          "df -h",
                          "cat /proc/cpuinfo",
                          "cat /proc/meminfo",
                          "cat /etc/fstab",
                          "mpstat -P ALL 1 4",
                          "iostat -m -x 1 4",
                          "vmstat 1 4",
                          "top -b -n 4 -d 1",
                          "ps aux",
                          "ulimit -a",
                          "dmesg",
                          "egrep -i 'err|fault|mysql' /var/log/*",
                          "pvs", "pvdisplay",
                          "vgs", "vgdisplay",
                          "lvs", "lvdisplay",
                          "netstat -lnput",
                          "numactl --hardware",
                          "numastat -m",
                          "sysctl -a",
                          "dmidecode -s system-product-name"]


def dump_shell_cmd(f, cmd):
    r = subprocess.run(
        cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    f.write(f"# {cmd}\n".encode("utf-8"))
    f.write(r.stdout)
    f.write(b"\n\n\n")
    return r.returncode


def collect_host_info(zf):
    if sys.platform != "win32":
        print(f"Collecting system information for {socket.gethostname()}")
        with zf.open("host_info.txt", "w") as f:
            for cmd in k_host_info_cmds_posix:
                print(f" - Executing {cmd}")
                dump_shell_cmd(f, cmd)


def collect_ping_stats(zf, prefix, topology_data):
    def ping(host, instance_id, timings):
        try:
            r = subprocess.run(["ping",
                                "/n" if sys.platform == "win32" else "-c", "5",
                                host], capture_output=True)
            timings.append((instance_id, host, r.stdout))
        except FileNotFoundError as e:
            timings.append((instance_id, host, str(e).encode("utf-8")))

    shell = globals.shell

    threads = []
    timings = []
    print(
        f"Executing ping from {socket.gethostname()} to other member hosts...")
    for instance in topology_data:
        endpoint = instance["endpoint"]
        instance_id = instance["instance_id"]
        host = shell.parse_uri(endpoint)["host"]

        print(f" - Executing ping {host} for 5s...")
        threads.append(threading.Thread(
            target=ping, args=(host, instance_id, timings)))
        threads[-1].start()

    for thd in threads:
        thd.join()

    with zf.open(f"{prefix}/ping.txt", "w") as f:
        for instance_id, host, out in timings:
            f.write(
                f"# instance {instance_id} - ping {host}\n".encode("utf-8"))
            f.write(b"\n")
            f.write(out)
            f.write(b"\n")


if __name__ == "__main__":
    with zipfile.ZipFile("test.zip", "w") as zf:
        collect_ping_stats(
            zf, "test", [{"endpoint": "localhost"}, {"endpoint": "www.oracle.com"}])

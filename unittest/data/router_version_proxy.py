# Copyright (c) 2026, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

"""Forward classic traffic, replacing only the greeting's version string.

The tests disable TLS and compression. A special SELECT reads the number of
backend-version probes; an optional error or invalid version exercises fallback.
"""

import select
import socket
import struct
import sys
import threading


def receive(connection, size):
    data = bytearray()
    while len(data) < size:
        chunk = connection.recv(size - len(data))
        if not chunk:
            raise EOFError()
        data.extend(chunk)
    return bytes(data)


def send_packet(connection, payload, sequence):
    connection.sendall(len(payload).to_bytes(3, "little") +
                       bytes([sequence]) + payload)


def forward(client, backend_port, greeting, probe_error, invalid_version):
    try:
        with client, socket.create_connection(("127.0.0.1", backend_port)) as backend:
            header = receive(backend, 4)
            payload = receive(backend, int.from_bytes(header[:3], "little"))
            version_end = payload.index(b"\0", 1)
            payload = payload[:1] + greeting + payload[version_end:]
            send_packet(client, payload, header[3])
            probe_count = 0
            while True:
                ready, _, _ = select.select([client, backend], [], [], 30)
                if not ready:
                    return
                for source in ready:
                    if source is backend:
                        data = backend.recv(65536)
                        if not data:
                            return
                        client.sendall(data)
                        continue

                    header = receive(client, 4)
                    payload = receive(client, int.from_bytes(header[:3], "little"))
                    if header[3] == 0 and payload[:1] == b"\x03":
                        # SQL follows any query attributes. Only these exact
                        # test statements are intercepted; all others pass through.
                        version_query = b"SELECT @@version"
                        if payload.endswith(version_query):
                            probe_count += 1
                            if probe_error:
                                send_packet(client, b"\xff" +
                                            struct.pack("<H", probe_error) +
                                            b"#HY000Injected version lookup error", 1)
                                continue
                            if invalid_version:
                                payload = payload[:-len(version_query)] + \
                                    b"SELECT 'invalid-version'"
                        count_query = b"SELECT 'router_version_probe_count'"
                        if payload.endswith(count_query):
                            payload = payload[:-len(count_query)] + \
                                b"SELECT " + str(probe_count).encode()
                    send_packet(backend, payload, header[3])
    except (EOFError, ConnectionResetError, BrokenPipeError):
        pass


backend_port = int(sys.argv[1])
greeting = sys.argv[2].encode()
probe_error = int(sys.argv[3])
invalid_version = sys.argv[4] == "1"
with socket.socket() as listener:
    listener.bind(("127.0.0.1", 0))
    listener.listen()
    print("PORT", listener.getsockname()[1], flush=True)
    while True:
        client, _ = listener.accept()
        threading.Thread(target=forward,
                         args=(client, backend_port, greeting, probe_error,
                               invalid_version),
                         daemon=True).start()

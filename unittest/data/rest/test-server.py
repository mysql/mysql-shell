#!/usr/bin/env python
#
# Copyright (c) 2019, 2024, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is designed to work with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have either included with
# the program or referenced in the documentation.
#
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#

from __future__ import print_function

try:
    from http.server import BaseHTTPRequestHandler, HTTPServer
    from socketserver import ThreadingMixIn
except:
    from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
    from SocketServer import ThreadingMixIn

import base64
import json
import os
import re
import ssl
import sys
import time
import traceback

from oci._vendor.jwt.utils import base64url_encode

try:
    from urllib.parse import parse_qsl, urlparse
except:
    from urlparse import parse_qsl, urlparse


class TestRequestHandler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    token_expiration = int(time.time()) + 60 * 60

    def __init__(self, *args, **kwargs):
        self._handlers = {
            r'^/timeout/([0-9]*\.?[0-9]*)$': self.handle_timeout,
            r'^/redirect/([1-9][0-9]*)$': self.handle_redirect,
            r'^/server_error/([1-9][0-9]*)/?([^/]*)/?(.*)$': self.handle_server_error,
            r'^/basic/([^/]+)/(.+)$': self.handle_basic,
            r'^/headers?.+$': self.handle_headers,
            r'^/partial_file/([1-9][0-9]*)$': self.handle_partial_file,
            r'^/20180711/resourcePrincipalToken/(.+)$': self.handle_rpt,
            r'^/20180711/resourcePrincipalTokenV2/(.+)$': self.handle_rpt_v2,
            r'^/v1/resourcePrincipalSessionToken$': self.handle_rpst,
            r'^/nested_rpt/([1-9][0-9]*)$': self.handle_nested_rpt,
            r'^/ecs?.+$': self.handle_ecs,
            r'^/imds(.+)$': self.handle_imds,
        }

        try:
            BaseHTTPRequestHandler.__init__(self, *args, **kwargs)
        except Exception as e:
            self.log_message(traceback.format_exc())

    def do_GET(self):
        self.handle_request()

    def do_HEAD(self):
        self.handle_request()

    def do_POST(self):
        self.handle_request()

    def do_PUT(self):
        self.handle_request()

    def do_PATCH(self):
        self.handle_request()

    def do_DELETE(self):
        self.handle_request()

    def handle_timeout(self, args):
        seconds = min(10, float(args[0]))
        time.sleep(seconds)
        self.reply()
        return True

    def handle_redirect(self, args):
        n = min(50, int(args[0]))
        self.send_response(302)
        self.send_header(
            'Location', '/%s' % self.command.lower()
            if n == 1 else '/redirect/%d' % (n - 1))
        self.send_header('Content-Length', '0')
        self.end_headers()
        return True

    def handle_basic(self, args):
        user = args[0]
        password = args[1]
        authorization = self.getheader('Authorization', 'Basic ')
        authenticated = ('%s:%s' % (user, password)) == base64.b64decode(
            authorization[6:]).decode('ascii')
        self.reply(200 if authenticated else 401,
                   {'authentication': 'OK' if authenticated else 'NO'})
        return True

    def handle_server_error(self, args):
        response = {}
        if len(args) >= 2:
            response['message'] = args[1]
        if len(args) >= 3:
            response['code'] = args[2]
        self.reply(status=int(args[0]), extra_response=response)
        return True

    def handle_headers(self, args):
        self.reply(extra_headers=dict(parse_qsl(urlparse(self.path).query, keep_blank_values=True)))
        return True

    def handle_partial_file(self, args):
        # Content-Length does not match length of the response body
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', f'{args[0]}')
        self.end_headers()
        self.wfile.close()
        return True

    def handle_rpt(self, args):
        if self.command != 'GET':
            return False
        self.reply(extra_response={
            "resourcePrincipalToken": self.to_jwt({"instance_id": args[0], "token_type": "rpt"}),
            "servicePrincipalSessionToken": self.to_jwt({"instance_id": args[0], "token_type": "spst"}),
        })
        return True

    def handle_rpt_v2(self, args):
        if self.command != 'GET':
            return False
        self.reply(extra_response={
            "resourcePrincipalToken": self.to_jwt({"resource_id": args[0], "token_type": "rpt"}),
            "servicePrincipalSessionToken": self.to_jwt({"resource_id": args[0], "token_type": "spst"}),
        })
        return True

    def handle_nested_rpt(self, args):
        if self.command != 'GET':
            return False
        self.reply(extra_response={
            "resourcePrincipalToken": self.to_jwt({"nested": True, "token_type": "rpt"}),
            "servicePrincipalSessionToken": self.to_jwt({"nested": True, "token_type": "spst"}),
        }, extra_headers={
            "opc-parent-rpt-url": f"http://127.0.0.1:{args[0]}/20180711/resourcePrincipalToken/id"
        })
        return True

    def handle_rpst(self, args):
        if self.command != 'POST':
            return False
        self.reply(extra_response={
            "token": self.to_jwt({
                "rpt": self.json_body["resourcePrincipalToken"],
                "spst": self.json_body["servicePrincipalSessionToken"],
                "pub": "sessionPublicKey" in self.json_body,
                "res_type": "rpst"}
            ),
        })
        return True

    def handle_ecs(self, args):
        extra_response = dict(parse_qsl(urlparse(self.path).query, keep_blank_values = True))
        authorization = self.getheader('Authorization', None)
        if authorization is not None:
            extra_response['Token'] = authorization
        for k, v in extra_response.items():
            if v.isnumeric():
                extra_response[k] = int(v)
        self.reply(extra_response = extra_response)
        return True

    def handle_imds(self, args):
        handled = False
        def reply(response):
            self.send_response(200)
            self.send_header('Content-Type', 'text/plain')
            self.send_header('Content-Length', str(len(response)))
            self.end_headers()
            self.wfile.write(response.encode('ascii'))
            nonlocal handled
            handled = True

        request = {}
        p = args[0].find('/path/')
        request['path'] = args[0][p + 5:]
        args = args[0][:p].split('/')[1:]

        for i in range(0, len(args), 2):
            request[args[i]] = args[i + 1]

        for k, v in request.items():
            if v.isnumeric():
                request[k] = int(v)

        token_path = '/latest/api/token'
        credentials_path = '/latest/meta-data/iam/security-credentials/'

        if self.command == 'PUT' and request['path'] == token_path:
            if self.getheader('x-aws-ec2-metadata-token-ttl-seconds', None):
                reply(request['security'])
        elif self.command == 'GET' and request['path'].startswith(credentials_path):
            if self.getheader('x-aws-ec2-metadata-token', None) == (request['security'] if request['security'] else None):
                if request['path'] == credentials_path:
                    reply(request['role'])
                elif request['path'] == credentials_path + request['role']:
                    reply(json.dumps(request))

        if not handled:
            reply('{"Code": "UnexpectedError", "Message": "This was unexpected"}')

        return True

    def invoke_handler(self):
        for path, handler in self._handlers.items():
            m = re.match(path, self.path)
            if m:
                return handler(m.groups())

        return False

    def handle_request(self):
        self.get_request_body()
        if not self.invoke_handler():
            self.reply()

    def reply(self, status=200, extra_response={}, extra_headers={}):
        response = {}
        response['method'] = self.command
        response['path'] = self.path
        response['headers'] = self.getheaders()
        response['data'] = self.body
        response['json'] = self.json_body
        response.update(extra_response)
        response = json.dumps(response)

        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', '%d' % (len(response)))

        for k, v in extra_headers.items():
            self.send_header(k, v)

        self.end_headers()

        if self.command != 'HEAD':
            self.wfile.write(response.encode('ascii'))

    def get_request_body(self):
        content_length = int(self.getheader('Content-Length', 0))
        self.body = None
        self.json_body = None

        if content_length > 0:
            self.body = self.rfile.read(content_length).decode('ascii')

            if self.getheader('Content-Type', 'unknown') == 'application/json':
                self.json_body = json.loads(self.body)

    def log_message(self, format, *args):
        BaseHTTPRequestHandler.log_message(self, format, *args)
        sys.stderr.flush()

    def getheader(self, name, default):
        if hasattr(self.headers, 'getheader'):
            return self.headers.getheader(name, default)
        else:
            return self.headers.get(name, default)

    def getheaders(self):
        headers = {}
        for k, v in dict(self.headers).items():
            headers[k.lower()] = v
        return headers

    def to_jwt(self, payload):
        payload["res_tenant"] = "ocid.tenant"
        payload["exp"] = self.token_expiration
        segments = []
        segments.append(base64url_encode(b'{"typ":"JWT","alg":"RS256"}'))
        segments.append(base64url_encode(json.dumps(payload).encode("ascii")))
        segments.append(b"sig")
        return b".".join(segments).decode("ascii")

class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    daemon_threads = True

def usage():
    print('Usage:')
    print('')
    print(' ', os.path.basename(__file__), 'port', '[no-https]')
    print('')


def test_server(port, https):
    server = ThreadedHTTPServer(('127.0.0.1', port), TestRequestHandler)
    server_type = 'HTTP'

    if https:
        server_type += 'S'
        ssl_dir = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'ssl')
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.verify_mode = ssl.CERT_NONE
        context.load_cert_chain(os.path.join(ssl_dir, 'cert.pem'), os.path.join(ssl_dir, 'key.pem'))
        server.socket = context.wrap_socket(server.socket, server_side=True)

    print(f'{server_type} test server running on 127.0.0.1:{port}')

    if '2' == os.environ.get('TEST_DEBUG'):
        print('Environment variables:')
        for k, v in os.environ.items():
            print(f'\tos.environ[{k}] = {v}')
        sys.stdout.flush()

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass

    server.server_close()
    print(f'{server_type} test server stopped')
    sys.stdout.flush()


def main(args):
    cargs = len(args)

    if cargs < 1 or cargs > 2:
        usage()
    else:
        https = True if cargs == 1 else False
        test_server(int(args[0]), https)


if __name__ == '__main__':
    main(sys.argv[1:])

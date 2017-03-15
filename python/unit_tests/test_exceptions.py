#
# Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#

"""
Unit tests for  mysql_gadgets.exceptions module.
"""

import unittest

from mysql_gadgets.common.server import Server
from mysql_gadgets.exceptions import (GadgetError, GadgetLogError,
                                      GadgetCnxInfoError, GadgetCnxFormatError,
                                      GadgetServerError, GadgetCnxError,
                                      GadgetQueryError, GadgetDBError,
                                      GadgetRPLError, GadgetConfigParserError,
                                      GadgetVersionError)


class TestExceptions(unittest.TestCase):
    """TestExceptions class.
    """

    def test_gadget_error(self):
        """Test gadget error.
        """
        # Raise GadgetError with default options.
        with self.assertRaises(GadgetError) as cm:
            raise GadgetError("I am a GadgetError")
        self.assertEqual(str(cm.exception), "I am a GadgetError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetError")
        self.assertEqual(cm.exception.errno, 0)
        self.assertIsNone(cm.exception.cause)

        # Raise GadgetError with specific options.
        with self.assertRaises(GadgetError) as cm:
            raise GadgetError("I am a GadgetError", errno=1234,
                              cause=Exception("Root cause error"))
        self.assertEqual(str(cm.exception), "I am a GadgetError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetError")
        self.assertEqual(cm.exception.errno, 1234)
        self.assertIsNotNone(cm.exception.cause)
        self.assertEqual(str(cm.exception.cause), "Root cause error")

    def test_gadget_log_error(self):
        """Test gadget log error.
        """
        # Raise GadgetLogError with default options.
        with self.assertRaises(GadgetLogError) as cm:
            raise GadgetLogError("I am a GadgetLogError")
        self.assertEqual(str(cm.exception), "I am a GadgetLogError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetLogError")
        self.assertEqual(cm.exception.errno, 0)
        self.assertIsNone(cm.exception.cause)
        self.assertIsInstance(cm.exception, GadgetError)

        # Raise GadgetLogError with specific options.
        with self.assertRaises(GadgetLogError) as cm:
            raise GadgetLogError("I am a GadgetLogError", errno=1234,
                                 cause=Exception("Root cause error"))
        self.assertEqual(str(cm.exception), "I am a GadgetLogError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetLogError")
        self.assertEqual(cm.exception.errno, 1234)
        self.assertIsNotNone(cm.exception.cause)
        self.assertEqual(str(cm.exception.cause), "Root cause error")

    def test_gadget_cnx_info_error(self):
        """Test gadget connection information error.
        """
        # Raise GadgetCnxInfoError with default options.
        with self.assertRaises(GadgetCnxInfoError) as cm:
            raise GadgetCnxInfoError("I am a GadgetCnxInfoError")
        self.assertEqual(str(cm.exception), "I am a GadgetCnxInfoError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetCnxInfoError")
        self.assertEqual(cm.exception.errno, 0)
        self.assertIsNone(cm.exception.cause)
        self.assertIsInstance(cm.exception, GadgetError)

        # Raise GadgetCnxInfoError with specific options.
        with self.assertRaises(GadgetCnxInfoError) as cm:
            raise GadgetCnxInfoError("I am a GadgetCnxInfoError", errno=1234,
                                     cause=Exception("Root cause error"))
        self.assertEqual(str(cm.exception), "I am a GadgetCnxInfoError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetCnxInfoError")
        self.assertEqual(cm.exception.errno, 1234)
        self.assertIsNotNone(cm.exception.cause)
        self.assertEqual(str(cm.exception.cause), "Root cause error")

    def test_gadget_cnx_format_error(self):
        """Test gadget connection format error.
        """
        # Raise GadgetCnxFormatError with default options.
        with self.assertRaises(GadgetCnxFormatError) as cm:
            raise GadgetCnxFormatError("I am a GadgetCnxFormatError")
        self.assertEqual(str(cm.exception), "I am a GadgetCnxFormatError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetCnxFormatError")
        self.assertEqual(cm.exception.errno, 0)
        self.assertIsNone(cm.exception.cause)
        self.assertIsInstance(cm.exception, GadgetError)

        # Raise GadgetCnxFormatError with specific options.
        with self.assertRaises(GadgetCnxFormatError) as cm:
            raise GadgetCnxFormatError("I am a GadgetCnxFormatError",
                                       errno=1234,
                                       cause=Exception("Root cause error"))
        self.assertEqual(str(cm.exception), "I am a GadgetCnxFormatError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetCnxFormatError")
        self.assertEqual(cm.exception.errno, 1234)
        self.assertIsNotNone(cm.exception.cause)
        self.assertEqual(str(cm.exception.cause), "Root cause error")

    def test_gadget_server_error(self):
        """Test gadget server error.
        """
        # Raise GadgetServerError with default options.
        with self.assertRaises(GadgetServerError) as cm:
            raise GadgetServerError("I am a GadgetServerError")
        self.assertEqual(str(cm.exception), "I am a GadgetServerError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetServerError")
        self.assertEqual(cm.exception.errno, 0)
        self.assertIsNone(cm.exception.cause)
        self.assertIsNone(cm.exception.server)
        self.assertIsInstance(cm.exception, GadgetError)

        # Raise GadgetServerError with specific options.
        srv = Server({'conn_info': {'user': 'myuser', 'host': 'myhost',
                                    'port': 3306}})
        with self.assertRaises(GadgetServerError) as cm:
            raise GadgetServerError("I am a GadgetServerError",
                                    errno=1234,
                                    cause=Exception("Root cause error"),
                                    server=srv)
        self.assertEqual(str(cm.exception),
                         "'myhost@3306' - I am a GadgetServerError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetServerError")
        self.assertEqual(cm.exception.errno, 1234)
        self.assertIsNotNone(cm.exception.cause)
        self.assertEqual(str(cm.exception.cause), "Root cause error")
        self.assertIsNotNone(cm.exception.server)

    def test_gadget_cnx_error(self):
        """Test gadget connection error.
        """
        # Raise GadgetCnxError with default options.
        with self.assertRaises(GadgetCnxError) as cm:
            raise GadgetCnxError("I am a GadgetCnxError")
        self.assertEqual(str(cm.exception), "I am a GadgetCnxError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetCnxError")
        self.assertEqual(cm.exception.errno, 0)
        self.assertIsNone(cm.exception.cause)
        self.assertIsNone(cm.exception.server)
        self.assertIsInstance(cm.exception, GadgetError)
        self.assertIsInstance(cm.exception, GadgetServerError)

        # Raise GadgetCnxError with specific options.
        srv = Server({'conn_info': {'user': 'myuser', 'host': 'myhost',
                                    'port': 3306}})
        with self.assertRaises(GadgetCnxError) as cm:
            raise GadgetCnxError("I am a GadgetCnxError",
                                 errno=1234,
                                 cause=Exception("Root cause error"),
                                 server=srv)
        self.assertEqual(str(cm.exception),
                         "'myhost@3306' - I am a GadgetCnxError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetCnxError")
        self.assertEqual(cm.exception.errno, 1234)
        self.assertIsNotNone(cm.exception.cause)
        self.assertEqual(str(cm.exception.cause), "Root cause error")
        self.assertIsNotNone(cm.exception.server)

    def test_gadget_query_error(self):
        """Test gadget query error.
        """
        # Raise GadgetQueryError with default options.
        with self.assertRaises(GadgetQueryError) as cm:
            raise GadgetQueryError("I am a GadgetQueryError", "SELECT 1")
        self.assertEqual(str(cm.exception),
                         "I am a GadgetQueryError. Query: SELECT 1")
        self.assertEqual(cm.exception.query, "SELECT 1")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetQueryError")
        self.assertEqual(cm.exception.errno, 0)
        self.assertIsNone(cm.exception.cause)
        self.assertIsNone(cm.exception.server)
        self.assertIsInstance(cm.exception, GadgetError)
        self.assertIsInstance(cm.exception, GadgetServerError)

        # Raise GadgetQueryError with specific options.
        srv = Server({'conn_info': {'user': 'myuser', 'host': 'myhost',
                                    'port': 3306}})
        with self.assertRaises(GadgetQueryError) as cm:
            raise GadgetQueryError("I am a GadgetQueryError", "SELECT 1",
                                   errno=1234,
                                   cause=Exception("Root cause error"),
                                   server=srv)
        self.assertEqual(str(cm.exception),
                         "'myhost@3306' - I am a GadgetQueryError. "
                         "Query: SELECT 1")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetQueryError")
        self.assertEqual(cm.exception.query, "SELECT 1")
        self.assertEqual(cm.exception.errno, 1234)
        self.assertIsNotNone(cm.exception.cause)
        self.assertEqual(str(cm.exception.cause), "Root cause error")
        self.assertIsNotNone(cm.exception.server)

    def test_gadget_db_error(self):
        """Test gadget database error.
        """
        # Raise GadgetDBError with default options.
        with self.assertRaises(GadgetDBError) as cm:
            raise GadgetDBError("I am a GadgetDBError")
        self.assertEqual(str(cm.exception), "I am a GadgetDBError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetDBError")
        self.assertEqual(cm.exception.errno, 0)
        self.assertIsNone(cm.exception.cause)
        self.assertIsNone(cm.exception.server)
        self.assertIsNone(cm.exception.db)
        self.assertIsInstance(cm.exception, GadgetError)
        self.assertIsInstance(cm.exception, GadgetServerError)

        # Raise GadgetDBError with specific options.
        srv = Server({'conn_info': {'user': 'myuser', 'host': 'myhost',
                                    'port': 3306}})
        with self.assertRaises(GadgetDBError) as cm:
            raise GadgetDBError("I am a GadgetDBError",
                                errno=1234,
                                cause=Exception("Root cause error"),
                                server=srv, db='test_db')
        self.assertEqual(str(cm.exception),
                         "'myhost@3306' - I am a GadgetDBError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetDBError")
        self.assertEqual(cm.exception.errno, 1234)
        self.assertIsNotNone(cm.exception.cause)
        self.assertEqual(str(cm.exception.cause), "Root cause error")
        self.assertIsNotNone(cm.exception.server)
        self.assertEqual(cm.exception.db, 'test_db')

    def test_gadget_rpl_error(self):
        """Test gadget replication error.
        """
        # Raise GadgetRPLError with default options.
        with self.assertRaises(GadgetRPLError) as cm:
            raise GadgetRPLError("I am a GadgetRPLError")
        self.assertEqual(str(cm.exception), "I am a GadgetRPLError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetRPLError")
        self.assertEqual(cm.exception.errno, 0)
        self.assertIsNone(cm.exception.cause)
        self.assertIsInstance(cm.exception, GadgetError)

        # Raise GadgetRPLError with specific options.
        with self.assertRaises(GadgetRPLError) as cm:
            raise GadgetRPLError("I am a GadgetRPLError", errno=1234,
                                 cause=Exception("Root cause error"))
        self.assertEqual(str(cm.exception), "I am a GadgetRPLError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetRPLError")
        self.assertEqual(cm.exception.errno, 1234)
        self.assertIsNotNone(cm.exception.cause)
        self.assertEqual(str(cm.exception.cause), "Root cause error")

    def test_gadget_config_parser_error(self):
        """Test gadget config parser error.
        """
        # Raise GadgetConfigParserError with default options.
        with self.assertRaises(GadgetConfigParserError) as cm:
            raise GadgetConfigParserError("I am a GadgetConfigParserError")
        self.assertEqual(str(cm.exception), "I am a GadgetConfigParserError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetConfigParserError")
        self.assertEqual(cm.exception.errno, 0)
        self.assertIsNone(cm.exception.cause)
        self.assertIsInstance(cm.exception, GadgetError)

        # Raise GadgetConfigParserError with specific options.
        with self.assertRaises(GadgetConfigParserError) as cm:
            raise GadgetConfigParserError("I am a GadgetConfigParserError",
                                          errno=1234,
                                          cause=Exception("Root cause error"))
        self.assertEqual(str(cm.exception), "I am a GadgetConfigParserError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetConfigParserError")
        self.assertEqual(cm.exception.errno, 1234)
        self.assertIsNotNone(cm.exception.cause)
        self.assertEqual(str(cm.exception.cause), "Root cause error")

    def test_gadget_version_error(self):
        """Test gadget version error.
        """
        # Raise GadgetVersionError with default options.
        with self.assertRaises(GadgetVersionError) as cm:
            raise GadgetVersionError("I am a GadgetVersionError")
        self.assertEqual(str(cm.exception), "I am a GadgetVersionError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetVersionError")
        self.assertEqual(cm.exception.errno, 0)
        self.assertIsNone(cm.exception.cause)
        self.assertIsInstance(cm.exception, GadgetError)

        # Raise GadgetVersionError with specific options.
        with self.assertRaises(GadgetVersionError) as cm:
            raise GadgetVersionError("I am a GadgetVersionError", errno=1234,
                                     cause=Exception("Root cause error"))
        self.assertEqual(str(cm.exception), "I am a GadgetVersionError")
        self.assertEqual(cm.exception.errmsg, "I am a GadgetVersionError")
        self.assertEqual(cm.exception.errno, 1234)
        self.assertIsNotNone(cm.exception.cause)
        self.assertEqual(str(cm.exception.cause), "Root cause error")

if __name__ == '__main__':
    unittest.main()

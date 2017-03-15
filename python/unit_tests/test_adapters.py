# -*- coding: utf-8 -*-
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
This file contains unit unittests for the mysql_gadgets.adapters.__init__.py
file.
"""
import abc
import unittest

from mysql_gadgets.adapters import (
    ImageClone, StreamClone, WithValidation, _abstractstatic, _with_metaclass)


class TestAdapters(unittest.TestCase):
    """Unit Test Class for the mysql_gadgets.adapters.__init__
    """
    def setUp(self):
        """setUp method"""
        pass

    def test__abstractstatic(self):
        """Test _abstractstatic decorator"""
        # it must set the __isaabstractmethod__ attribute to True.
        @_abstractstatic
        def foo():  # pylint: disable=C0102
            """foo method"""
            pass
        self.assertEqual(foo.__isabstractmethod__, True)

        # must raise and error since foo is a abstract method and as such C
        # can't be instantiated.
        @_with_metaclass(abc.ABCMeta)
        class C(object):
            """Class C"""

            @_abstractstatic
            def foo():  # pylint: disable=C0102
                """foo method"""
                return 3
        self.assertRaises(TypeError, C)

        # however D which extends C and implements foo as a static method can
        # be instantiated.
        class D(C):
            """Class D"""
            @staticmethod
            def foo():  # pylint: disable=C0102
                return 4
        self.assertEqual(D.foo(), 4)
        self.assertEqual(D().foo(), 4)

    def test_ImageClone(self):
        """Test ImageClone abstract class"""
        # it shouldn't be possible to instantiate a class until it implements
        # abstract methods backup_to_image, move_image and restore_from_image.
        class A(ImageClone):
            """Class A"""
            pass
        with self.assertRaises(TypeError):
            A()

        class B(ImageClone):
            """Class B"""
            @staticmethod
            def backup_to_image(connection_dict, image_path):
                return 1
        with self.assertRaises(TypeError):
            B()

        class C(ImageClone):
            """Class C"""
            @staticmethod
            def backup_to_image(connection_dict, image_path):
                return 1

            @staticmethod
            def move_image(connection_dict, image_path):
                return 2

        with self.assertRaises(TypeError):
            C()

        # you must either extend all methods, either by extending a class that
        # extends ImageClone and implementing the missing methods or
        # by extending ImageClone directly and implementing all the methods.
        class D(C):
            """Class D"""
            # pylint: disable=W0613
            @staticmethod
            def move_image(connection_dict, image_name):
                return 22

            @staticmethod
            def restore_from_image(connection_dict, image_name):
                return 3

        self.assertEqual(D.backup_to_image(None, None), 1)
        self.assertEqual(D().backup_to_image(None, None), 1)
        self.assertEqual(D.move_image(None, None), 22)
        self.assertEqual(D().move_image(None, None), 22)
        self.assertEqual(D.restore_from_image(None, None), 3)
        self.assertEqual(D().restore_from_image(None, None), 3)

        class E(ImageClone):
            """Class E"""
            @staticmethod
            def backup_to_image(connection_dict, image_path):
                return 1

            @staticmethod
            def move_image(connection_dict, image_path):
                return 2

            @staticmethod
            def restore_from_image(connection_dict, image_path):
                return 3

        self.assertEqual(E.backup_to_image(None, None), 1)
        self.assertEqual(E().backup_to_image(None, None), 1)
        self.assertEqual(E.move_image(None, None), 2)
        self.assertEqual(E().move_image(None, None), 2)
        self.assertEqual(E.restore_from_image(None, None), 3)
        self.assertEqual(E().restore_from_image(None, None), 3)

    def test_StreamClone(self):
        """Test StreamClone abstract class """
        # it shouldn't be possible to instantiate a class until it
        # implements abstract method stream_clone
        class A(StreamClone):
            """Class A"""
            pass
        with self.assertRaises(TypeError):
            A()

        # you must either extend all methods, either by extending a class that
        # extends StreamClone and implementing the missing methods or
        # by extending ImageClone directly and implementing all the methods.

        class B(A):
            """Class B"""
            @staticmethod
            def clone_stream(connection_dict):
                return 1

        self.assertEqual(B.clone_stream(None), 1)
        self.assertEqual(B().clone_stream(None), 1)

        class C(StreamClone):
            """Class C"""
            @staticmethod
            def clone_stream(connection_dict):
                return 1
        self.assertEqual(C.clone_stream(None), 1)
        self.assertEqual(C().clone_stream(None), 1)

    def test_WithValidation(self):
        """Test StreamClone abstract class """
        # it shouldn't be possible to instantiate a class until it
        # implements abstract methods pre_validation and post_validation.
        class A(WithValidation):
            """Class A"""
            pass
        with self.assertRaises(TypeError):
            A()

        class B(WithValidation):
            """Class B"""
            @staticmethod
            def pre_validation(connection_dict):
                return 1
        with self.assertRaises(TypeError):
            B()

        class C(WithValidation):
            """Class C"""
            @staticmethod
            def post_validation(connection_dict):
                return 2

        with self.assertRaises(TypeError):
            C()

        # you must either extend all methods, either by extending a class that
        # extends ImageClone and implementing the missing methods or
        # by extending ImageClone directly and implementing all the methods.
        class D(C):
            """Class D"""
            # pylint: disable=W0613
            @staticmethod
            def pre_validation(connection_dict):
                return 22

        self.assertEqual(D().pre_validation(None), 22)
        self.assertEqual(D().post_validation(None), 2)

        class E(B, C):
            """Class E"""
            pass

        self.assertEqual(E().pre_validation(None), 1)
        self.assertEqual(E().post_validation(None), 2)

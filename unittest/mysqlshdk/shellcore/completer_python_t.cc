/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include <gtest/gtest.h>
#include <gtest/gtest_prod.h>

#include "mysqlshdk/shellcore/provider_python.h"

namespace shcore {
namespace completer {

TEST(Provider_python, parsing) {
  Provider_python c(std::shared_ptr<Object_registry>(nullptr),
                    std::shared_ptr<Python_context>(nullptr));

  Provider_script::Chain chain;

  chain = c.process_input("", nullptr);
  EXPECT_TRUE(chain.empty());

  chain = c.process_input("foo", nullptr);
  ASSERT_EQ(1, chain.size());
  EXPECT_EQ(Provider_script::Chain::Variable, chain.peek_type());
  EXPECT_EQ("foo", chain.next().second);

  chain = c.process_input("foo.", nullptr);
  ASSERT_EQ(2, chain.size());
  EXPECT_EQ("foo", chain.next().second);
  EXPECT_EQ("", chain.next().second);

  chain = c.process_input("foo.bar", nullptr);
  ASSERT_EQ(2, chain.size());
  EXPECT_EQ("foo", chain.next().second);
  EXPECT_EQ("bar", chain.next().second);

  chain = c.process_input("foo.bar.baz", nullptr);
  ASSERT_EQ(3, chain.size());
  EXPECT_EQ("foo", chain.next().second);
  EXPECT_EQ("bar", chain.next().second);
  EXPECT_EQ("baz", chain.next().second);

  chain = c.process_input("foo .bar", nullptr);
  ASSERT_EQ(0, chain.size());

  chain = c.process_input(".bar", nullptr);
  ASSERT_EQ(0, chain.size());

  chain = c.process_input("foo. bar", nullptr);
  ASSERT_EQ(1, chain.size());
  EXPECT_EQ("bar", chain.next().second);

  chain = c.process_input("foo.bar()", nullptr);
  ASSERT_EQ(0, chain.size());

  chain = c.process_input("a.b()).c", nullptr);
  ASSERT_EQ(0, chain.size());

  chain = c.process_input("foo.bar().", nullptr);
  ASSERT_EQ(3, chain.size());
  EXPECT_EQ("foo", chain.next().second);
  EXPECT_EQ(Provider_script::Chain::Function, chain.peek_type());
  EXPECT_EQ("bar", chain.next().second);
  EXPECT_EQ("", chain.next().second);

  chain = c.process_input("f.b().ba", nullptr);
  ASSERT_EQ(3, chain.size());
  EXPECT_EQ("f", chain.next().second);
  EXPECT_EQ(Provider_script::Chain::Function, chain.peek_type());
  EXPECT_EQ("b", chain.next().second);
  EXPECT_EQ(Provider_script::Chain::Variable, chain.peek_type());
  EXPECT_EQ("ba", chain.next().second);

  chain = c.process_input("f.b('foo=bar)').ba", nullptr);
  ASSERT_EQ(3, chain.size());
  EXPECT_EQ("f", chain.next().second);
  EXPECT_EQ(Provider_script::Chain::Function, chain.peek_type());
  EXPECT_EQ("b", chain.next().second);
  EXPECT_EQ(Provider_script::Chain::Variable, chain.peek_type());
  EXPECT_EQ("ba", chain.next().second);

  chain =
      c.process_input("foo.bar('foo=bar)', bla, ble, 123, foo[1]).ba", nullptr);
  ASSERT_EQ(3, chain.size());
  EXPECT_EQ("foo", chain.next().second);
  EXPECT_EQ(Provider_script::Chain::Function, chain.peek_type());
  EXPECT_EQ("bar", chain.next().second);
  EXPECT_EQ(Provider_script::Chain::Variable, chain.peek_type());
  EXPECT_EQ("ba", chain.next().second);

  chain = c.process_input("foo.bar('foo=bar)', bla, ble(), 123, foo[1]).ba",
                          nullptr);
  ASSERT_EQ(3, chain.size());
  EXPECT_EQ("foo", chain.next().second);
  EXPECT_EQ(Provider_script::Chain::Function, chain.peek_type());
  EXPECT_EQ("bar", chain.next().second);
  EXPECT_EQ(Provider_script::Chain::Variable, chain.peek_type());
  EXPECT_EQ("ba", chain.next().second);

  chain = c.process_input("foo.bar()[12].ba", nullptr);
  ASSERT_EQ(3, chain.size());
  EXPECT_EQ("foo", chain.next().second);
  EXPECT_EQ(Provider_script::Chain::Function, chain.peek_type());
  EXPECT_EQ("bar", chain.next().second);
  EXPECT_EQ(Provider_script::Chain::Variable, chain.peek_type());
  EXPECT_EQ("ba", chain.next().second);

  chain = c.process_input("foo.bar[12].ba", nullptr);
  ASSERT_EQ(3, chain.size());
  EXPECT_EQ("foo", chain.next().second);
  EXPECT_EQ("bar", chain.next().second);
  EXPECT_EQ(Provider_script::Chain::Variable, chain.peek_type());
  EXPECT_EQ("ba", chain.next().second);

  chain = c.process_input("foo.bar({\"a\":\"b\"}, (1+1)).ba", nullptr);
  ASSERT_EQ(3, chain.size());
  EXPECT_EQ("foo", chain.next().second);
  EXPECT_EQ("bar", chain.next().second);
  EXPECT_EQ(Provider_script::Chain::Variable, chain.peek_type());
  EXPECT_EQ("ba", chain.next().second);

  chain = c.process_input("print(foo.bar[12].ba", nullptr);
  ASSERT_EQ(3, chain.size());
  EXPECT_EQ("foo", chain.next().second);
  EXPECT_EQ("bar", chain.next().second);
  EXPECT_EQ(Provider_script::Chain::Variable, chain.peek_type());
  EXPECT_EQ("ba", chain.next().second);

  chain = c.process_input("if bla: foo.bar", nullptr);
  ASSERT_EQ(2, chain.size());
  EXPECT_EQ("foo", chain.next().second);
  EXPECT_EQ(Provider_script::Chain::Variable, chain.peek_type());
  EXPECT_EQ("bar", chain.next().second);

  chain = c.process_input("if bla: foo.bar\nelse: bla", nullptr);
  ASSERT_EQ(1, chain.size());
  EXPECT_EQ("bla", chain.next().second);

  chain = c.process_input("#linecomment\nfoo", nullptr);
  ASSERT_EQ(1, chain.size());
  EXPECT_EQ("foo", chain.next().second);

  chain = c.process_input("#linecomment foo", nullptr);
  ASSERT_EQ(0, chain.size());

  chain = c.process_input("'string", nullptr);
  ASSERT_EQ(0, chain.size());

  chain = c.process_input("a.b('st\\'ring').c", nullptr);
  ASSERT_EQ(3, chain.size());
  EXPECT_EQ("a", chain.next().second);
  EXPECT_EQ("b", chain.next().second);
  EXPECT_EQ("c", chain.next().second);

  chain = c.process_input("a.b('st\"ring').c", nullptr);
  ASSERT_EQ(3, chain.size());
  EXPECT_EQ("a", chain.next().second);
  EXPECT_EQ("b", chain.next().second);
  EXPECT_EQ("c", chain.next().second);

  chain = c.process_input("a.b('st\"ring\\'').c", nullptr);
  ASSERT_EQ(3, chain.size());
  EXPECT_EQ("a", chain.next().second);
  EXPECT_EQ("b", chain.next().second);
  EXPECT_EQ("c", chain.next().second);

  chain = c.process_input("a.b('''st\"r\ni'n'g\\'''').c", nullptr);
  ASSERT_EQ(3, chain.size());
  EXPECT_EQ("a", chain.next().second);
  EXPECT_EQ("b", chain.next().second);
  EXPECT_EQ("c", chain.next().second);
}
}  // namespace completer
}  // namespace shcore

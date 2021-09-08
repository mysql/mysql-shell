/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/mysql/gtid_utils.h"

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_mysql_session.h"
#include "unittest/test_utils/shell_test_env.h"

namespace mysqlshdk {
namespace mysql {

class Gtid_utils : public tests::Shell_test_env {
 public:
};

TEST_F(Gtid_utils, gtid_set_basics) {
  Gtid_set gs1;
  Gtid_set gs2_r(Gtid_range{"8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 1, 43});
  Gtid_set gs2_s(
      Gtid_set::from_string("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43"));
  Gtid_set gs2;
  Gtid_set gs3(Gtid_range{"8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 1, 5});
  Gtid_set gs4(Gtid_range{"8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 43, 44});
  Gtid_set gs5(Gtid_range{"8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 45, 50});
  Gtid_set gs6(Gtid_set::from_string(
      "8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43:45-50:99"));

  {
    Gtid_set gs;

    auto session = std::make_shared<testing::Mock_mysql_session>();
    mysqlshdk::mysql::Instance server(session);

    session->expect_query("select @@global.gtid_executed")
        .then({""})
        .add_row({"8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43"});

    gs = Gtid_set::from_gtid_executed(server);
    EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43", gs.str());

    session->expect_query("select @@global.gtid_purged")
        .then({""})
        .add_row({"8b8dc2ba-8803-11eb-af3d-a1178d81dccc:5"});

    gs = Gtid_set::from_gtid_purged(server);
    EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:5", gs.str());
  }

  EXPECT_THROW(gs2_s.count(), std::invalid_argument);

  auto session = db::mysql::Session::create();
  session->connect(db::Connection_options(_mysql_uri));
  mysqlshdk::mysql::Instance server(session);

  gs2_s.normalize(server);
  gs6.normalize(server);

  EXPECT_EQ(gs2_r, gs2_s);
  EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43", gs2_r.str());

  gs2 = gs2_r;
  EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43", gs2.str());
  EXPECT_EQ(gs2, gs2_r);
  EXPECT_EQ(gs2, gs2_s);

  EXPECT_TRUE(gs1.empty());
  EXPECT_EQ(0, gs1.count());

  EXPECT_TRUE(Gtid_set() == gs1);
  EXPECT_FALSE(Gtid_set() == gs2);

  EXPECT_TRUE(Gtid_set() != gs2);
  EXPECT_FALSE(Gtid_set() != gs1);

  EXPECT_TRUE(gs1 != gs2);
  EXPECT_FALSE(gs1 == gs2);

  EXPECT_FALSE(gs2.empty());
  EXPECT_EQ(43, gs2.count());

  EXPECT_TRUE(gs2_r.contains(gs2_s, server));
  EXPECT_TRUE(gs2_r.contains(gs2_s, server));
  EXPECT_TRUE(gs2.contains(gs3, server));
  EXPECT_FALSE(gs3.contains(gs2, server));

  EXPECT_FALSE(gs2.contains(gs4, server));
  EXPECT_FALSE(gs4.contains(gs2, server));

  EXPECT_FALSE(gs2.contains(gs5, server));
  EXPECT_FALSE(gs5.contains(gs2, server));

  EXPECT_TRUE(gs6.contains(gs2, server));
  EXPECT_TRUE(gs6.contains(gs5, server));

  EXPECT_EQ(50, gs6.count());
}

TEST_F(Gtid_utils, gtid_set_ops) {
  auto session = db::mysql::Session::create();
  session->connect(db::Connection_options(_mysql_uri));

  mysqlshdk::mysql::Instance server(session);

  Gtid_set gs1;
  Gtid_set gs2_r(Gtid_range{"8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 1, 43});
  Gtid_set gs2_s(
      Gtid_set::from_string("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43"));
  Gtid_set gs2;
  Gtid_set gs3(Gtid_range{"8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 1, 5});
  Gtid_set gs4(Gtid_range{"8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 43, 44});
  Gtid_set gs5(Gtid_range{"8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 45, 70});
  Gtid_set gs6(Gtid_range{"8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 44, 48});
  Gtid_set gs7(Gtid_range{"8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 10, 20});
  Gtid_set gs8(Gtid_range{"88888888-8803-11eb-af3d-a1178d81dccc", 1, 8});

  gs1.add(gs2_r);
  EXPECT_EQ(gs1, gs2_r);
  gs1 = Gtid_set();

  gs2 = gs2_r;
  gs2.add(gs2_s);
  gs2.normalize(server);
  EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43", gs2.str());

  gs2 = gs2_r;
  gs2.add(gs1);
  EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43", gs2.str());

  gs2 = gs2_r;
  gs2.add(gs3);
  EXPECT_THROW(
      {
        bool x = gs3 == gs2;
        (void)x;
      },
      std::invalid_argument);
  EXPECT_THROW(gs2.count(), std::invalid_argument);
  gs2.normalize(server);
  EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43", gs2.str());

  gs2 = gs2_r;
  gs2.add(gs4);
  gs2.normalize(server);
  EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-44", gs2.str());

  gs2 = gs2_r;
  gs2.add(gs5);
  gs2.normalize(server);
  EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43:45-70", gs2.str());

  gs2 = gs2_r;
  gs2.add(gs4);
  gs2.add(gs5);
  gs2.normalize(server);
  EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-70", gs2.str());

  gs2 = gs2_r;
  gs2.add(gs8);
  gs2.normalize(server);
  EXPECT_EQ(
      "88888888-8803-11eb-af3d-a1178d81dccc:1-8,\n8b8dc2ba-8803-11eb-af3d-"
      "a1178d81dccc:1-43",
      gs2.str());

  gs2 = gs2_r;
  gs2.add(Gtid_range("8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 99, 99));
  EXPECT_THROW(
      {
        bool x = gs1 == gs2;
        (void)x;
      },
      std::invalid_argument);
  gs2.normalize(server);
  EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43:99", gs2.str());

  gs2 = gs2_r;
  gs2.add(Gtid_range("9b8dc2ba-0000-11eb-af3d-a1178d81dccc", 99, 99));
  gs2.normalize(server);
  EXPECT_EQ(
      "8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43,\n9b8dc2ba-0000-11eb-af3d-"
      "a1178d81dccc:99",
      gs2.str());

  gs2 = gs2_r;
  gs2.add(Gtid_range("9b8dc2ba-0000-11eb-af3d-a1178d81dccc", 10, 99));
  gs2.normalize(server);
  EXPECT_EQ(
      "8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43,\n9b8dc2ba-0000-11eb-af3d-"
      "a1178d81dccc:10-99",
      gs2.str());

  gs2 = gs2_r;
  gs2.add(Gtid_range("8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 10, 99));
  gs2.normalize(server);
  EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-99", gs2.str());

  gs2 = gs2_r;
  gs2.subtract(gs1, server);
  EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43", gs2.str());

  gs2 = gs2_r;
  gs2.subtract(gs2, server);
  EXPECT_EQ("", gs2.str());

  gs2 = gs2_r;
  gs2.subtract(gs5, server);
  EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-43", gs2.str());

  gs2 = gs2_r;
  gs2.subtract(gs3, server);
  EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:6-43", gs2.str());

  gs2 = gs2_r;
  gs2.subtract(gs7, server);
  EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-9:21-43", gs2.str());
}

TEST_F(Gtid_utils, gtid_set_enumerate) {
  auto session = db::mysql::Session::create();
  session->connect(db::Connection_options(_mysql_uri));

  mysqlshdk::mysql::Instance server(session);

  Gtid_set gs1(Gtid_range{"8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 1, 9});
  Gtid_set gs2(Gtid_range{"8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 1, 1});
  Gtid_set gs3(
      Gtid_set::from_string("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-9:20-30,"
                            "\n9b8dc2ba-0000-11eb-af3d-a1178d81dccc:1-5"));

  {
    int calls = 0;
    Gtid_set result;
    gs1.enumerate([&](const mysqlshdk::mysql::Gtid &gtid) {
      result.add(gtid);
      ++calls;
    });
    result.normalize(server);
    EXPECT_EQ(gs1, result);
    EXPECT_EQ(gs1.count(), calls);
  }
  {
    int calls = 0;
    Gtid_set result;
    gs2.enumerate([&](const mysqlshdk::mysql::Gtid &gtid) {
      result.add(gtid);
      ++calls;
    });
    result.normalize(server);
    EXPECT_EQ(gs2.str(), result.str());
    EXPECT_EQ(gs2.count(), calls);
  }

  EXPECT_THROW(gs3.enumerate([&](const auto &) {}), std::invalid_argument);
  gs3.normalize(server);

  {
    int calls = 0;
    Gtid_set result;
    gs3.enumerate([&](const mysqlshdk::mysql::Gtid &gtid) {
      result.add(gtid);
      ++calls;
    });
    result.normalize(server);
    EXPECT_EQ(gs3.str(), result.str());
    EXPECT_EQ(gs3.count(), calls);
  }
}

TEST_F(Gtid_utils, gtid_set_enumerate_ranges) {
  auto session = db::mysql::Session::create();
  session->connect(db::Connection_options(_mysql_uri));

  mysqlshdk::mysql::Instance server(session);

  Gtid_set gs1(Gtid_range{"8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 1, 9});
  Gtid_set gs2(Gtid_range{"8b8dc2ba-8803-11eb-af3d-a1178d81dccc", 1, 1});
  Gtid_set gs3(
      Gtid_set::from_string("8b8dc2ba-8803-11eb-af3d-a1178d81dccc:1-9:20-30,"
                            "\n9b8dc2ba-0000-11eb-af3d-a1178d81dccc:1-5"));

  {
    int calls = 0;
    Gtid_set result;
    gs1.enumerate_ranges([&](const mysqlshdk::mysql::Gtid_range &gtids) {
      result.add(gtids);
      ++calls;
    });
    result.normalize(server);
    EXPECT_EQ(gs1, result);
    EXPECT_EQ(1, calls);
  }
  {
    int calls = 0;
    Gtid_set result;
    gs2.enumerate_ranges([&](const mysqlshdk::mysql::Gtid_range &gtids) {
      result.add(gtids);
      ++calls;
    });
    result.normalize(server);
    EXPECT_EQ(gs2.str(), result.str());
    EXPECT_EQ(1, calls);
  }

  EXPECT_THROW(gs3.enumerate_ranges([&](const auto &) {}),
               std::invalid_argument);
  gs3.normalize(server);

  {
    int calls = 0;
    Gtid_set result;
    std::vector<mysqlshdk::mysql::Gtid_range> ranges;
    gs3.enumerate_ranges([&](const mysqlshdk::mysql::Gtid_range &gtids) {
      result.add(gtids);
      ranges.push_back(gtids);
      ++calls;
    });
    result.normalize(server);
    EXPECT_EQ(gs3.str(), result.str());
    EXPECT_EQ(3, calls);
    EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc", std::get<0>(ranges[0]));
    EXPECT_EQ(1, std::get<1>(ranges[0]));
    EXPECT_EQ(9, std::get<2>(ranges[0]));
    EXPECT_EQ("8b8dc2ba-8803-11eb-af3d-a1178d81dccc", std::get<0>(ranges[1]));
    EXPECT_EQ(20, std::get<1>(ranges[1]));
    EXPECT_EQ(30, std::get<2>(ranges[1]));
    EXPECT_EQ("9b8dc2ba-0000-11eb-af3d-a1178d81dccc", std::get<0>(ranges[2]));
    EXPECT_EQ(1, std::get<1>(ranges[2]));
    EXPECT_EQ(5, std::get<2>(ranges[2]));
  }
}

TEST_F(Gtid_utils, subtract_view_changes) {
  auto session = db::mysql::Session::create();
  session->connect(db::Connection_options(_mysql_uri));

  mysqlshdk::mysql::Instance server(session);

  auto gtid_set = Gtid_set::from_string(
      "ec32d2c0-d3f0-11eb-abf3-eb7171e21adc:1-79,\nec32e076-d3f0-11eb-abf3-"
      "eb7171e21adc:1-3,\nf37283fa-d3f0-11eb-84e6-06d82947e5a7:1-2");

  gtid_set.normalize(server);

  auto view_changes =
      gtid_set.get_gtids_from("f37283fa-d3f0-11eb-84e6-06d82947e5a7");

  gtid_set.subtract(view_changes, server);

  EXPECT_EQ(
      "ec32d2c0-d3f0-11eb-abf3-eb7171e21adc:1-79,\nec32e076-d3f0-11eb-abf3-"
      "eb7171e21adc:1-3",
      gtid_set.str());
}

}  // namespace mysql
}  // namespace mysqlshdk

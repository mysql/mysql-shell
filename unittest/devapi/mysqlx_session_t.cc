/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "db/session.h"
#include "modules/devapi/mod_mysqlx.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

template <typename Message_type>
std::string message_to_text(const Message_type &msg) {
  std::string result;

  google::protobuf::TextFormat::PrintToString(msg, &result);

  return msg.GetDescriptor()->full_name() + " { " + result + " }";
}

void print_message(const std::string &direction,
                   const xcl::XProtocol::Message &msg) {
  std::cout << direction << "\t" << msg.ByteSizeLong() + 1 << "\t"
            << message_to_text(msg) << "\n";
}

xcl::Handler_result trace_send_messages(
    xcl::XProtocol * /* protocol */,
    const xcl::XProtocol::Client_message_type_id /* msg_id */,
    const xcl::XProtocol::Message &msg) {
  print_message(">>>> SEND ", msg);

  // None of processed messages should be filtered out
  return xcl::Handler_result::Continue;
}

extern mysqlshdk::utils::Version g_target_server_version;

xcl::Handler_result trace_received_messages(
    xcl::XProtocol * /* protocol */,
    const xcl::XProtocol::Server_message_type_id /* msg_id */,
    const xcl::XProtocol::Message &msg) {
  print_message("<<<< RECEIVE ", msg);

  // None of processed messages should be filtered out
  return xcl::Handler_result::Continue;
}

void enable_trace(xcl::XSession *sess) {
  auto &protocol = sess->get_protocol();

  protocol.add_received_message_handler(
      [](xcl::XProtocol *proto,
         const xcl::XProtocol::Server_message_type_id msg_id,
         const xcl::XProtocol::Message &msg) -> xcl::Handler_result {
        return trace_received_messages(proto, msg_id, msg);
      });

  protocol.add_send_message_handler(
      [](xcl::XProtocol *proto,
         const xcl::XProtocol::Client_message_type_id msg_id,
         const xcl::XProtocol::Message &msg) -> xcl::Handler_result {
        return trace_send_messages(proto, msg_id, msg);
      });
}

namespace mysqlsh {
namespace mysqlx {

class Mysqlx_session : public tests::Shell_base_test {
 public:
};

TEST_F(Mysqlx_session, connect) {
  {
    std::shared_ptr<Session> session(new Session());
    EXPECT_THROW_NOTHING(
        session->connect(mysqlshdk::db::Connection_options(_uri)));
  }

  {
    // FIXME when exception cleanup happens, these should expect nested
    // FIXME exceptions with db::Error inside
    auto session = std::make_unique<Session>();
    mysqlshdk::db::Connection_options opts(_uri_nopasswd);
    opts.set_password("???");
    if (g_target_server_version >= mysqlshdk::utils::Version(8, 0, 12)) {
      EXPECT_THROW_LIKE(session->connect(opts), shcore::Exception,
                        "Access denied for user 'root'@'localhost'");
    } else {
      EXPECT_THROW_LIKE(session->connect(opts), shcore::Exception,
                        "Invalid user or password");
    }
  }

  {
    auto session = std::make_unique<Session>();
    mysqlshdk::db::Connection_options opts;
    opts.set_host("blabla");
    EXPECT_THROW_LIKE(session->connect(opts), shcore::Exception,
                      "No such host is known");
  }
}

TEST_F(Mysqlx_session, create_schema) {
  {
    std::shared_ptr<Session> session(new Session());
    EXPECT_THROW_NOTHING(
        session->connect(mysqlshdk::db::Connection_options(_uri)));

    EXPECT_NO_THROW(session->execute_sql("drop schema if exists `testsch`",
                                         shcore::Array_t()));

    EXPECT_THROW_NOTHING(session->create_schema("testsch"));
    EXPECT_THROW_LIKE(session->create_schema("testsch"), shcore::Exception,
                      "database exists");

    EXPECT_NO_THROW(session->drop_schema("testsch"));
  }
}

TEST_F(Mysqlx_session, get_uuid) {
  Session session;

  std::string uuid = session.get_uuid();
  std::string base_prefix = uuid.substr(0, 12);  // Gets the node element
  std::map<std::string, int> uuids;

  uuids[uuid] = 1;

  for (int index = 0; index < 1000; index++) {
    uuid = session.get_uuid();

    // Ensures the new UUID has the same prefix as the base
    ASSERT_STREQ(base_prefix.c_str(), uuid.substr(0, 12).c_str());

    // Ensures the UUID is unique
    ASSERT_EQ(0, uuids.count(uuid));

    uuids[uuid] = 1;
  }
}

// Our own tests to check for regressions in libmysqlx

#if 0
// This is a libmysqlxclient test
TEST_F(Mysqlx_session, libmysqlx_next_resultset) {
  std::unique_ptr<xcl::XSession> sess(::xcl::create_session());

  xcl::XError err = sess->connect(_host.c_str(), _port_number, _user.c_str(),
                                  _pwd.c_str(), "");
  ASSERT_FALSE(err);

  {
    std::unique_ptr<xcl::XQuery_result> result(
        sess->execute_sql("select 1", &err));
    xcl::XError rerr;
    EXPECT_TRUE(result->get_next_row(&rerr));
    EXPECT_FALSE(result->get_next_row(&rerr));
    // if (result->has_resultset()) {
    //   xcl::XError err2;
    //   result->next_resultset(&err2);
    // }
    EXPECT_STREQ("", err.what());

    std::unique_ptr<xcl::XQuery_result> result2(
        sess->execute_sql("select 1", &err));
    EXPECT_STREQ("", err.what());
    if (result2 && result2->has_resultset())
      result2->next_resultset(&err);
  }

  {
    std::unique_ptr<xcl::XQuery_result> result(
        sess->execute_sql("set @a = 1", &err));
    if (result->has_resultset()) {
      xcl::XError err2;
      result->next_resultset(&err2);
    }
    EXPECT_STREQ("", err.what());
    std::unique_ptr<xcl::XQuery_result> result2(
        sess->execute_sql("select 1", &err));
    EXPECT_STREQ("", err.what());
    if (result2 && result2->has_resultset())
      result2->next_resultset(&err);
  }
}
#endif
}  // namespace mysqlx
}  // namespace mysqlsh

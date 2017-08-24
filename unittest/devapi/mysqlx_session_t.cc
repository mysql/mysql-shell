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
  std::cout << direction << "\t" << msg.ByteSize() + 1 << "\t"
            << message_to_text(msg) << "\n";
}

xcl::Handler_result trace_send_messages(
    xcl::XProtocol *protocol,
    const xcl::XProtocol::Client_message_type_id msg_id,
    const xcl::XProtocol::Message &msg) {
  print_message(">>>> SEND ", msg);

  /** Non of processed messages should be filtered out*/
  return xcl::Handler_result::Continue;
}

xcl::Handler_result trace_received_messages(
    xcl::XProtocol *protocol,
    const xcl::XProtocol::Server_message_type_id msg_id,
    const xcl::XProtocol::Message &msg) {
  print_message("<<<< RECEIVE ", msg);

  /** Non of processed messages should be filtered out*/
  return xcl::Handler_result::Continue;
}

void enable_trace(xcl::XSession *sess) {
  auto &protocol = sess->get_protocol();

  protocol.add_received_message_handler(
      [](xcl::XProtocol *protocol,
         const xcl::XProtocol::Server_message_type_id msg_id,
         const xcl::XProtocol::Message &msg) -> xcl::Handler_result {
        return trace_received_messages(protocol, msg_id, msg);
      });

  protocol.add_send_message_handler(
      [](xcl::XProtocol *protocol,
         const xcl::XProtocol::Client_message_type_id msg_id,
         const xcl::XProtocol::Message &msg) -> xcl::Handler_result {
        return trace_send_messages(protocol, msg_id, msg);
      });
}

namespace mysqlsh {
namespace mysqlx {

class Mysqlx_session : public tests::Shell_base_test {
 public:
};

TEST_F(Mysqlx_session, connect) {
  {
    std::shared_ptr<NodeSession> session(new NodeSession());
    EXPECT_THROW_NOTHING(
        session->connect(mysqlshdk::db::Connection_options(_uri)));
  }

  {
    // FIXME when exception cleanup happens, these should expect nested
    // FIXME exceptions with db::Error inside
    std::unique_ptr<NodeSession> session(new NodeSession());
    mysqlshdk::db::Connection_options opts(_uri_nopasswd);
    opts.set_password("???");
    EXPECT_THROW_LIKE(session->connect(opts), shcore::Exception,
                      "Invalid user or password");
  }

  {
    std::unique_ptr<NodeSession> session(new NodeSession());
    mysqlshdk::db::Connection_options opts;
    opts.set_host("blabla");
    EXPECT_THROW_LIKE(session->connect(opts), shcore::Exception,
                      "No such host is known");
  }
}

TEST_F(Mysqlx_session, create_schema) {
  {
    std::shared_ptr<NodeSession> session(new NodeSession());
    EXPECT_THROW_NOTHING(
        session->connect(mysqlshdk::db::Connection_options(_uri)));

    EXPECT_THROW_NOTHING(session->create_schema("testsch"));
    EXPECT_THROW_LIKE(session->create_schema("testsch"), mysqlshdk::db::Error,
                      "database exists");

    EXPECT_NO_THROW(session->drop_schema("testsch"));
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

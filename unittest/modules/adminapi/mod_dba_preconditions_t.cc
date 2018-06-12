/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/dba/preconditions.h"

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/utils/version.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils/shell_base_test.h"

namespace testing {

class Dba_preconditions : public tests::Shell_base_test {
 protected:
  void SetUp() {
    tests::Shell_base_test::SetUp();

    m_mock_session = std::make_shared<Mock_session>();
  }

  void TearDown() { tests::Shell_base_test::TearDown(); }

  std::shared_ptr<Mock_session> m_mock_session;
};

TEST_F(Dba_preconditions, validate_session) {
  // Test inexistent session
  try {
    mysqlsh::dba::validate_session(nullptr);
    SCOPED_TRACE("Unexpected success calling validate_session");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "The shell must be connected to a member of the InnoDB cluster being "
        "managed",
        e.what());
  }

  // Test closed session
  EXPECT_CALL(*m_mock_session, is_open()).WillOnce(Return(false));

  try {
    mysqlsh::dba::validate_session(m_mock_session);
    SCOPED_TRACE("Unexpected success calling validate_session");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "The session was closed. An open session is required to perform this "
        "operation",
        e.what());
  }

  // Test invalid server version
  EXPECT_CALL(*m_mock_session, is_open()).WillOnce(Return(true));
  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("9.0")));

  try {
    mysqlsh::dba::validate_session(m_mock_session);
    SCOPED_TRACE("Unexpected success calling validate_session");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Unsupported server version: AdminAPI operations require MySQL server "
        "versions 5.7 or 8.0",
        e.what());
  }

  // Test different invalid server versions
  EXPECT_CALL(*m_mock_session, is_open()).Times(5).WillRepeatedly(Return(true));

  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("8.1")));
  EXPECT_THROW(mysqlsh::dba::validate_session(m_mock_session),
               shcore::Exception);

  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("8.2.0")));
  EXPECT_THROW(mysqlsh::dba::validate_session(m_mock_session),
               shcore::Exception);

  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("9.0.0")));
  EXPECT_THROW(mysqlsh::dba::validate_session(m_mock_session),
               shcore::Exception);

  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("5.6")));
  EXPECT_THROW(mysqlsh::dba::validate_session(m_mock_session),
               shcore::Exception);

  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("5.1")));
  EXPECT_THROW(mysqlsh::dba::validate_session(m_mock_session),
               shcore::Exception);

  // Test valid server version
  EXPECT_CALL(*m_mock_session, is_open()).WillOnce(Return(true));
  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("8.0")));

  try {
    mysqlsh::dba::validate_session(m_mock_session);
  } catch (const shcore::Exception &e) {
    std::string error = "Unexpected failure calling validate_session: ";
    error += e.what();
    SCOPED_TRACE(error);
    ADD_FAILURE();
  }

  // Test different valid server versions
  EXPECT_CALL(*m_mock_session, is_open()).Times(5).WillRepeatedly(Return(true));

  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("8.0.1")));
  EXPECT_NO_THROW(mysqlsh::dba::validate_session(m_mock_session));

  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("8.0.11")));
  EXPECT_NO_THROW(mysqlsh::dba::validate_session(m_mock_session));

  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("8.0.99")));
  EXPECT_NO_THROW(mysqlsh::dba::validate_session(m_mock_session));

  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("5.7")));
  EXPECT_NO_THROW(mysqlsh::dba::validate_session(m_mock_session));

  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("5.7.22")));
  EXPECT_NO_THROW(mysqlsh::dba::validate_session(m_mock_session));
}
}  // namespace testing

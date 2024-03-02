/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <cstdlib>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#include "mysqlshdk/libs/db/filtering_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/common/dump/checksums.h"
#include "modules/util/dump/indexes.h"
#include "modules/util/dump/instance_cache.h"

namespace mysqlsh {
namespace dump {
namespace common {
namespace {

std::string to_string(Checksums::Hash hash) {
  switch (hash) {
    case Checksums::Hash::SHA_224:
      return "sha224";
    case Checksums::Hash::SHA_256:
      return "sha256";
    case Checksums::Hash::SHA_384:
      return "sha384";
    case Checksums::Hash::SHA_512:
      return "sha512";
  }

  throw std::logic_error("to_string(Hash)");
}

class Lock_5_6 final {
 public:
  Lock_5_6() = delete;

  explicit Lock_5_6(std::shared_ptr<mysqlshdk::db::ISession> session)
      : m_session(std::move(session)) {
    m_session->execute("CREATE TABLE IF NOT EXISTS mysql.lock56 (a int)");

    while (true) {
      try {
        m_session->execute("ALTER TABLE mysql.lock56 ADD COLUMN locked int");
        break;
      } catch (...) {
        shcore::sleep_ms(1000);
      }
    }
  }

  Lock_5_6(const Lock_5_6 &) = delete;
  Lock_5_6(Lock_5_6 &&) = delete;

  Lock_5_6 &operator=(const Lock_5_6 &) = delete;
  Lock_5_6 &operator=(Lock_5_6 &&) = delete;

  ~Lock_5_6() {
    m_session->execute("ALTER TABLE mysql.lock56 DROP COLUMN locked");
  }

 private:
  std::shared_ptr<mysqlshdk::db::ISession> m_session;
};

class Checksums_test : public Shell_core_test_wrapper {
 protected:
  void TearDown() override {
    if (m_session) {
      cleanup();
      m_session->close();
    }

    Shell_core_test_wrapper::TearDown();
  }

  static std::shared_ptr<mysqlshdk::db::ISession> connect_session(
      const std::string &uri) {
    auto session = mysqlshdk::db::mysql::Session::create();
    session->connect(mysqlshdk::db::Connection_options(uri));
    return session;
  }

  void run_sha2_test(const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    m_session = session;

    cleanup();
    setup();

    std::vector<Test> tests = {
        // whole tables
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(FACE2E5CB77AF1C6321E78B6032D4BD8F0777E74E38C13CFDEDB48C7)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(05BF658E3C29F9E85B023CAF43EC48982840E7FFC8DFDE6BE678D74DAC681FBC)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(CD27A30922D6A34D9E0AD107E5B5550F26613B3C36536AA77CEB53C608147FB6575E9CD7398B3088F67C73A458D5A184)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(9C930EE88AF05941B703D31E5FBD632937FE7435B0F6EDD4D491E106410505F9E588B8A4684BD2A401AA4B2658B94A269AEC7DA0CE22EF97DD3712CB228F293F)",
                },
            },
            1001,
            k_table_with_pk,
        },
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(E54D28453330D5D2883ADDA7165962AC93BB3E682D1E5C017CCD1126)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(C847EBF2928869D44187BDA942375756A4787E30CF1ED0C7D146ED7B2F03438F)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(E4DCE860BC3FB6FB1479BA206B0BC1393D8D4F1313BA6CEC1B1E53840C5AE1583F23BA9B1095C81DDA510F0FD1EC848C)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(E66810CF8320E446C37F145EE71DFE00EE729BA0A932EFFE9B8CA06B95487B03A04D32C465CB91D7DBBF3627C8A2EC07D81F81F21ECDDC993A142916905EDE6A)",
                },
            },
            1002,
            k_table_with_unique_null,
        },
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(24C57C8312D7C859E3A72E7B85AEA968C963E44E9BA214BABBFF2D41)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(997617E9257BC705D5A406F6457E39F06BEA15BFEA843BE2AF5BE9D9F89AFFBD)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(C38D7AEF1939D9A72E75EE3F790AEE02B60BCB25F0F187F192279C14B6E99D2317AA8C3F6CD6B8FB3B4C19D7133867C5)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(ACC049066602D307483A51415B14E1169E747911BF9436202377503BEE309962E586A1ED7594336AB08379D76F2CF8AECAE32D59210B51BC1E3EDD3FDA2766A6)",
                },
            },
            1001,
            k_table_without_index,
        },
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(459E64D38A7B6C7292D63A5A3F90BF6CA291B27A32957E62512FD5F4)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(33448262FD15094468E01038F611AC84900D5D9DC7C54E8C6F306906B372565C)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(6F3E43FABD309B74DF5B0BC0E2AAE074315FBBF0FA614C98B28C6A9252FFBFA0AD53C04F90C5585A89F816193331F448)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(0A66F329D6AF545750F70D0D03CCFEE69B54E0DAD5B6F4AEFBB9AE1C3E3A086E1D8653B1675238CBD6AF053E87409A4102B20BD40F6C0506FEA739A1DE6FFA1E)",
                },
            },
            1000,
            k_partitioned_table,
        },
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(00000000000000000000000000000000000000000000000000000000)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(0000000000000000000000000000000000000000000000000000000000000000)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000)",
                },
            },
            0,
            k_empty_table,
        },
        // partitions
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(AC1A4D7E94F88C1BD59BC54C5B250E67EC33448F7B8BDA68651A77D9)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(0FFCE3A4485EDF182817E0392BBB04C559B7815564100D8B1394DAB07347A4AC)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(117BAED03214EF46E4C99C1EF9600ACD24EB36D198C345335913FEA26F730D74BABA738C937A86C62C1A50F9B236BA39)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(7E1AFF4C2A651E38368BC1EF0C0EE3C087ED9DB577916E55CE044060EC12CA09ADBADD405DC9279612182423728B749E9E53001DD43E55BF3E53E36D4950F83B)",
                },
            },
            500,
            k_partitioned_table,
            "p0",
        },
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(E98429AD1E83E069474DFF1664B5B10B4EA2F6F5491EA40A3435A22D)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(3CB861C6B54BD65C40F7F001DDAAA841C9BADCC8A3D543077CA4B3B6C035F2F0)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(7E45ED2A8F2474323B9297DE1BCAEAB915B48D2162A209ABEB9F94303D8CB2D417E9B3C303BFDE9CA5E246E081074E71)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(747C0C65FCCA4A6F667CCCE20FC21D261CB97D6FA2279AFB35BDEE7CD228C267B03C8EF13A9B1F5DC4B7211DF5CBEEDF9CE10BC9DB5250B9C0F4DACC973F0225)",
                },
            },
            500,
            k_partitioned_table,
            "p1",
        },
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(00000000000000000000000000000000000000000000000000000000)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(0000000000000000000000000000000000000000000000000000000000000000)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000)",
                },
            },
            0,
            k_partitioned_table,
            // partition with no data
            "p2",
        },
        // filtering
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(FA14D831908F15980944A031FA60DE143E2C980008A566F0416227D3)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(FF89380FEBDB4FD63A7CCFAB3D3BC7CDF09005A2680F6699DCB7113B3E1E46B3)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(BC1AE9B701182F6C98CFC4F121A7AB3064C07E867868EC2B41E8B6DEABCC9C64C20E4FC886B57545B073361A32B4395D)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(8635065E691102F207A35C12E018269798BBB9E12CF110EB4EAD2861B8E7AE07CEA28AF8FBFD9D4B6C8D87739CA49B3A26DE544159F8D0085C67B2714C1FE8CB)",
                },
            },
            1000,
            k_table_with_unique_null,
            "",
            // this gives the same data as in the whole partitioned table,
            // hashes are different because this table has a nullable ID column
            "(id is not null)",
        },
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(1924102E4F0F20364167CA89641DC674500DD6137BD126893F2429EB)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(731508B993E1B29108B6EFD7B7249B2056DBD204E8B6BD45B0F3C0F5CFF8EE43)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(75D1420ACC85A0C31648DED9165720C36776623C982EC6CB14D0A00D3F24C317ED39632BC84FEF0A3E1C602D37825521)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(C4416EFFFEB399642B90FD6E22492913D3D617069F0E867889384DAA0F3402E386C3474BF23364C70117A8E28D112D64A8F800B39ACCFA0A702D9247DBD89AFB)",
                },
            },
            351,
            k_table_with_unique_null,
            "",
            "",
            "(id between 50 and 400)",
        },
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(932E96ED37A358F74FDA1E21D57E094A70820342413DAE68A3693178)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(64703C8E59307F5E562115E6A75E1B778F7332764C794CAE60CAAFCFEAC127F1)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(9FBBE62F7F97B781E985B309667607089543C26778E91EEA362562A92806480339F36870C6232CE9D03E42B8B8A6389F)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(944909B93CD270D764DDE1987F32DB0CEBEBB0B3E7D8A27DDE51A09F7983F072E97ED74FF46E46C246406EFD1B6F9A818A4901736FEB21971BCAFC42ECC2A35C)",
                },
            },
            300,
            k_table_with_unique_null,
            "",
            "(id > 100)",
            "(id between 50 and 400)",
        },
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(00000000000000000000000000000000000000000000000000000000)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(0000000000000000000000000000000000000000000000000000000000000000)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000)",
                },
            },
            0,
            k_table_with_unique_null,
            "",
            "(id > 400)",
            "(id between 50 and 400)",
        },
        // partition + filtering
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(C781FF2936D9F4BADE4DCC6B42F78BD600C12CD1C0AA3ACE0C747902)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(3D40295A3264E7C688BE64A973E7C113B25D7756D19F131D1BD31B62D4DFC6CD)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(C980B9AFA76DBB81F92C943FC4F76374ABF9DF33C61608E583778C24433000CDEC4B1D83C7605DA52557AE2CBF23010F)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(43E5FB19088062BC232BB4ABC8C307C0EE9B8D6C1194C39112AFAC0F78E9FFE5DD06A02D85215FD39370DD68AB872D0F2BA4C11C4B59ED5E58C8D0C803E5DE1D)",
                },
            },
            20,
            k_partitioned_table,
            "p0",
            "(a < 1020)",
        },
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(69F4FD4BEE972AE1AA3E34930769D21F56C44C0F88C5440606C49C0A)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(0D79ED4D9864A1589C008BBB6504728BE7EB545F0FE40C217221F5A7FC5CF1BA)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(41C6F5EA36B58881B7BDD52A85213B4DA34766DB2A5B9A18BFB7EFDF43706BE792EB6033F9B046882C5B04E1557C7DD5)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(4DB6F77C9CBAE96CDC0C5A74F125973D7FB8658F7692826D390E02A29C6EFD341EA933AB02BAE2AE5FF6738D5836755866540749A61146AE406BDB757E37DAB1)",
                },
            },
            351,
            k_partitioned_table,
            "p0",
            "",
            "(id between 50 and 400)",
        },
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(D14AC50DAEE71A0B026198F31E0FAC91D2AC5445238BBEABE825A85F)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(0E9635FBE4BF61579B567B6D19C4CB94A56AB24384D485398F21F20126842659)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(3CFE702A0655A8BC9FAA1C1F41FC9E6001099E04EEEA7C594EAA91E13B40418A4E5AE7E1B173CDB8D135BFC580CD2BBC)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(855058ABD1BBD2AF10E27B1CD71221CEBDAACCC8702E9565CFC8680C74C2206790B69D3A7329B0EE9B804F03BA68E160EFFFA1E096729B51E8BD1CDF1A076490)",
                },
            },
            50,
            k_partitioned_table,
            "p0",
            "(a < 1100)",
            "(id between 50 and 400)",
        },
        {
            {
                {
                    Checksums::Hash::SHA_224,
                    R"(00000000000000000000000000000000000000000000000000000000)",
                },
                {
                    Checksums::Hash::SHA_256,
                    R"(0000000000000000000000000000000000000000000000000000000000000000)",
                },
                {
                    Checksums::Hash::SHA_384,
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000)",
                },
                {
                    Checksums::Hash::SHA_512,
                    R"(00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000)",
                },
            },
            0,
            k_partitioned_table,
            "p0",
            "(id > 500)",
        },
    };

    for (const auto &test : tests) {
      execute(test);
    }
  }

 private:
  struct Expected {
    Checksums::Hash hash;
    std::string checksum;
  };

  struct Test {
    std::vector<Expected> expected;
    uint64_t expected_count;
    std::string table;
    std::string partition = {};
    std::string filter = {};
    std::string boundary = {};
  };

  void execute(const Test &test) {
    assert(m_session);

    for (const auto &expected : test.expected) {
      SCOPED_TRACE("hash: " + to_string(expected.hash) + ", table: `" +
                   test.table + "`, partition: `" + test.partition +
                   "`, filter: '" + test.filter + "', boundary: '" +
                   test.boundary + "'");

      Checksums checksums{expected.hash};
      checksums.configure(m_session);

      const auto &table = m_cache.schemas[k_schema].tables[test.table];
      checksums.initialize_table(k_schema, test.table, &table,
                                 select_index(table).first, test.filter);

      auto checksum = checksums.prepare_checksum(
          k_schema, test.table, test.partition, -1, test.boundary);
      checksum->compute(m_session, "");

      EXPECT_EQ(expected.checksum, checksum->result().checksum);
      EXPECT_EQ(test.expected_count, checksum->result().count);
    }
  }

  void cleanup() {
    assert(m_session);

    m_session->executef("DROP SCHEMA IF EXISTS !", k_schema);
  }

  void setup() {
    assert(m_session);

    // create schema and tables
    m_session->executef("CREATE SCHEMA !", k_schema);
    m_session->executef("USE !", k_schema);

    m_session->executef(
        "CREATE TABLE ! (id int PRIMARY KEY, a int NULL, b int NULL)",
        k_table_with_pk);
    m_session->executef(
        "CREATE TABLE ! (id int NULL UNIQUE KEY, a int NULL, b int NULL)",
        k_table_with_unique_null);
    m_session->executef("CREATE TABLE ! (id int NULL, a int NULL, b int NULL)",
                        k_table_without_index);
    m_session->executef(
        "CREATE TABLE ! (id int PRIMARY KEY, a int NULL, b int NULL) "
        "PARTITION BY RANGE (id) ("
        "PARTITION p0 VALUES LESS THAN (500),"
        "PARTITION p1 VALUES LESS THAN (1000),"
        "PARTITION p2 VALUES LESS THAN MAXVALUE"
        ")",
        k_partitioned_table);
    m_session->executef(
        "CREATE TABLE ! (id int PRIMARY KEY, a int NULL, b int NULL)",
        k_empty_table);

    // prepare insert query
    std::string insert = "INSERT INTO ! VALUES ";

    constexpr int max_rows = 1000;

    for (int i = 0; i < max_rows; ++i) {
      insert +=
          shcore::str_format("(%d,%d,%d),", i, max_rows + i, 2 * max_rows + i);
    }

    // remove last comma
    insert.pop_back();

    // insert data
    m_session->executef(insert, k_table_with_pk);
    m_session->executef("INSERT INTO ! VALUES (?, NULL, ?)", k_table_with_pk,
                        max_rows, max_rows);

    m_session->executef("INSERT INTO ! VALUES (NULL, -1, -2)",
                        k_table_with_unique_null);
    m_session->executef(insert, k_table_with_unique_null);
    m_session->executef("INSERT INTO ! VALUES (NULL, -3, -4)",
                        k_table_with_unique_null);

    // contents is like k_table_with_pk, just last row has values reversed -
    // hash needs to be different
    m_session->executef(insert, k_table_without_index);
    m_session->executef("INSERT INTO ! VALUES (?, ?, NULL)",
                        k_table_without_index, max_rows, max_rows);

    m_session->executef(insert, k_partitioned_table);

    mysqlshdk::db::Filtering_options filter;
    filter.schemas().include(k_schema);
    m_cache = Instance_cache_builder{m_session, filter}.metadata({}).build();
  }

  const std::string k_schema = "checksum_test";
  const std::string k_table_with_pk = "pk";
  const std::string k_table_with_unique_null = "unique_null";
  const std::string k_table_without_index = "no_index";
  const std::string k_partitioned_table = "partitioned";
  const std::string k_empty_table = "empty";

  std::shared_ptr<mysqlshdk::db::ISession> m_session;
  Instance_cache m_cache;
};

TEST_F(Checksums_test, sha2_hashes) {
  run_sha2_test(connect_session(_mysql_uri));
}

TEST_F(Checksums_test, sha2_hashes_5_6) {
  const auto uri = ::getenv("MYSQL56_URI");

  if (!uri || !uri[0]) {
    SKIP_TEST("The 5.6 server is not available");
  }

  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 0)) {
    // we need to run this test only once, and we want to avoid executing it in
    // parallel
    SKIP_TEST("Executed only when 8.0+ server is present");
  }

  Lock_5_6 lock{connect_session(uri)};
  run_sha2_test(connect_session(uri));
}

}  // namespace
}  // namespace common
}  // namespace dump
}  // namespace mysqlsh

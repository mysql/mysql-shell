/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include <string_view>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_private_key.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace shcore {
namespace ssl {
namespace {

constexpr std::string_view k_private_key = R"(-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDA2n7QmELJoNov
BaQyFNbcG5kM37p417tUtXfH1yvNAp+I9lfk8xAZ9aUJyuSiZXG9eoIuIjps8xpr
PQLX6FpjybJj/AylFjV01ixgI+JnZ4NyeYeMlh0kvqfp3dmF5lmNR1FzOTNqbYkH
kTGvxYpBcZDnoO3QFzztmZnoxCCMYx7NMEvEVdMxRryq8qKaVSSP9c4xFKZANsFq
/+UXBVhvJj7S0k8M7rEtk4MuENdEk9hS4BpUBrxJ1b36y94Yo+4xSondgtzOt8Ui
HOrszvF94xKCrDhtyMygMetgfG8NOUeLXMhGKtCNeADAoM0IxsSJyxYbDOOnha4u
OqljFRkvAgMBAAECggEAQDA5J0qWaC++jwoMpZKkFOymm09X7GK5ei9QJ9apQHoQ
CXEcvhpckJdfEAxU3uxu2AcPjzVbRun/MBRFfwFaHH6EfpzkxInIzxN9/53R/GLK
sG5cvGE2YAWJM+DgRp3tzoUfSf+O6mx8wGUGKZ/RLYkUFtRMcwdZAygtvviQcMj0
Ow7OaftZha2ufLkh+m8BciFYMPJ4qRhAsI6ZYPwzzCZ1aJK5arL8OcfBQgkNrE2m
4XqiYkB6ih7ygxDyw83Aeku7Qkd3aQSNYov2xUmrQmtjbv5hFDky/MqNLuS1quts
dwsi2HrBqRxHYkbW/YJeSnzl8/PZY/B9if95lScskQKBgQD24npwv8L0nGREaF76
0yKlj/v47fRRhL6V1075eW3m9pddRLwPviq1pTl4wy7P5T2TIvJVNp/aM76hNuYR
0qc0m1RtuRvUkWZNaGdJA7wPIInTZyG757DfC1xzlk4ULMhu4o/e6ZMJbQHtf/+9
/QvFBonwA1+ohkC1us+nchOopwKBgQDH+VJH33SDkVCBdNjX47pa1t3itVifPQCC
4lfbD5Dio2FWn1ljx1Bz+7HNjnOSZPFrbAX+tJCSgiOXmGXvVQPecskncacgdQqx
s2Ev9iSEZ3skZEWbNOlVSyjvfjPjSj2pHGHnJehAMrwuVTkw44GuZPMZMjzjiLaG
CppAGayUOQKBgGuZi4mmWf6oRp716idha+FCnqwAcZIxy6qcu0PJo5ec851nvpsR
46VlTGYQk09yduKwwXNYKlU8ZiH3PlCYQd2QsTxdB/8eG7sy/Ij7ArSJFui3MyGl
OhYL6pBJ+t+aIQUK429g13+tLcDoH3sbVUzaq7Waks9tK7jIoYY2FtFTAoGAEEEc
xgjMwLD8GTm1Mn8pM58JdBcHeBIOS5U4TO8L+NAM5myXaHvX90V7I1kz3a9kzDWA
mz50DeSUHgteJcEESn2MCi+7xPCeiOkquipIt5ZttPyNh9saKDuuvSDF+PCc6HUN
JCu5oBY/o4h72JR1gwZjmCW0O05uf6C2/wTXOUkCgYEAzeELZlQjlTBIijRPvqVh
iFlASb79zF+EJmwZZOECtgcA61owgnkTlRaVGf0YDL/Zf9BzEvbSjw1bDLeommQr
gxXu7pcq8Yw+LCrnIw2wBGOf06r6QCXdhbcBw0V87E+Ls+GRTWZ/ZKaq4B/Lld/7
fAlnAxZ/Sj6wVVX+VGX/XNo=
-----END PRIVATE KEY-----
)";

constexpr std::string_view k_public_key = R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAwNp+0JhCyaDaLwWkMhTW
3BuZDN+6eNe7VLV3x9crzQKfiPZX5PMQGfWlCcrkomVxvXqCLiI6bPMaaz0C1+ha
Y8myY/wMpRY1dNYsYCPiZ2eDcnmHjJYdJL6n6d3ZheZZjUdRczkzam2JB5Exr8WK
QXGQ56Dt0Bc87ZmZ6MQgjGMezTBLxFXTMUa8qvKimlUkj/XOMRSmQDbBav/lFwVY
byY+0tJPDO6xLZODLhDXRJPYUuAaVAa8SdW9+sveGKPuMUqJ3YLczrfFIhzq7M7x
feMSgqw4bcjMoDHrYHxvDTlHi1zIRirQjXgAwKDNCMbEicsWGwzjp4WuLjqpYxUZ
LwIDAQAB
-----END PUBLIC KEY-----
)";

constexpr std::string_view k_fingerprint =
    "97:c1:b7:64:c3:bb:d1:1d:a5:84:5d:b3:39:8b:ce:23";
constexpr std::string_view k_fingerprint_sha1 =
    "d2:85:d3:01:29:7e:05:3d:29:39:94:ff:46:24:4b:8d:87:b4:b8:be";
constexpr std::string_view k_fingerprint_sha256 =
    "f3:55:de:8a:ba:13:20:64:13:a7:e6:bc:61:61:7e:14:a4:1b:9f:68:b4:58:86:60:"
    "3f:a9:a1:7f:e1:c2:7d:93";

constexpr std::string_view k_pass_phrase = "Pa$$w0rd!@#";

class Utils_private_key_test : public Shell_core_test_wrapper {};

TEST_F(Utils_private_key_test, load_save_and_read) {
  const auto private_key = Private_key::from_string(k_private_key);
  const auto public_key = private_key.public_key();

  // check that private key is loaded correctly
  {
    EXPECT_EQ(k_private_key, private_key.to_string());
    EXPECT_EQ(k_fingerprint, private_key.fingerprint(Hash::RESTRICTED_MD5));
    EXPECT_EQ(k_fingerprint_sha1,
              private_key.fingerprint(Hash::RESTRICTED_SHA1));
    EXPECT_EQ(k_fingerprint_sha256, private_key.fingerprint(Hash::SHA256));

    EXPECT_EQ(k_public_key, public_key.to_string());
    EXPECT_EQ(k_fingerprint, public_key.fingerprint(Hash::RESTRICTED_MD5));
    EXPECT_EQ(k_fingerprint_sha1,
              public_key.fingerprint(Hash::RESTRICTED_SHA1));
    EXPECT_EQ(k_fingerprint_sha256, public_key.fingerprint(Hash::SHA256));
  }

  const auto private_key_path = Private_key::full_path(".", "test_key");

  // save the private key and ensure the correct access flags are applied
  {
    private_key.save(private_key_path, k_pass_phrase);
    EXPECT_NO_THROW(check_file_access_rights_to_open(private_key_path));
  }

  // save the public key and ensure the correct access flags are applied
  {
    const auto public_key_path = Public_key::full_path(".", "test_key");
    public_key.save(public_key_path);
    EXPECT_NO_THROW(check_file_access_rights_to_open(public_key_path));
  }

  // no starting pass phrase, all inputs are wrong
  {
    std::string k_wrong_pass_phrase = "wrong";

    output_handler.passwords.push_back(
        {shcore::str_format(
             "Please enter the passphrase for private key '%s': ",
             private_key_path.c_str()),
         k_wrong_pass_phrase,
         {}});
    output_handler.passwords.push_back(
        {"Wrong passphrase, please try again: ", k_wrong_pass_phrase, {}});
    output_handler.passwords.push_back(
        {"Wrong passphrase, please try again: ", k_wrong_pass_phrase, {}});
    output_handler.passwords.push_back(
        {"Wrong passphrase, please try again: ", k_wrong_pass_phrase, {}});
    output_handler.passwords.push_back(
        {"Wrong passphrase, please try again: ", k_wrong_pass_phrase, {}});

    EXPECT_THROW(Private_key::from_file(private_key_path), Decrypt_error);
  }

  // with a starting pass phrase, all inputs are wrong
  {
    std::string k_wrong_pass_phrase = "wrong";

    output_handler.passwords.push_back(
        {shcore::str_format(
             "Please enter the passphrase for private key '%s': ",
             private_key_path.c_str()),
         k_wrong_pass_phrase,
         {}});
    output_handler.passwords.push_back(
        {"Wrong passphrase, please try again: ", k_wrong_pass_phrase, {}});
    output_handler.passwords.push_back(
        {"Wrong passphrase, please try again: ", k_wrong_pass_phrase, {}});
    output_handler.passwords.push_back(
        {"Wrong passphrase, please try again: ", k_wrong_pass_phrase, {}});
    output_handler.passwords.push_back(
        {"Wrong passphrase, please try again: ", k_wrong_pass_phrase, {}});

    EXPECT_THROW(Private_key::from_file(private_key_path, k_wrong_pass_phrase),
                 Decrypt_error);
  }

  // no starting pass phrase, but a correct one is provided
  {
    std::string k_wrong_pass_phrase = "wrong";

    output_handler.passwords.push_back(
        {shcore::str_format(
             "Please enter the passphrase for private key '%s': ",
             private_key_path.c_str()),
         k_wrong_pass_phrase,
         {}});
    output_handler.passwords.push_back({"Wrong passphrase, please try again: ",
                                        std::string{k_pass_phrase},
                                        {}});

    const auto pk = Private_key::from_file(private_key_path);
    EXPECT_EQ(k_fingerprint, pk.fingerprint());
  }

  // starting pass phrase is wrong, but a correct one is provided
  {
    std::string k_wrong_pass_phrase = "wrong";

    output_handler.passwords.push_back(
        {shcore::str_format(
             "Please enter the passphrase for private key '%s': ",
             private_key_path.c_str()),
         std::string{k_pass_phrase},
         {}});

    const auto pk =
        Private_key::from_file(private_key_path, k_wrong_pass_phrase);
    EXPECT_EQ(k_fingerprint, pk.fingerprint());
  }

  // pass phrase is correct
  {
    const auto pk = Private_key::from_file(private_key_path, k_pass_phrase);
    EXPECT_EQ(k_fingerprint, pk.fingerprint());
  }

  // load into storage, starting pass phrase is wrong, but a correct one is
  // provided
  {
    std::string k_wrong_pass_phrase = "wrong";

    output_handler.passwords.push_back(
        {shcore::str_format(
             "Please enter the passphrase for private key '%s': ",
             private_key_path.c_str()),
         std::string{k_pass_phrase},
         {}});

    const auto pk = Private_key_storage::instance().from_file(
        private_key_path, k_wrong_pass_phrase);
    EXPECT_EQ(k_fingerprint, pk.fingerprint());
  }

  // load into storage, starting pass phrase is wrong, but file was loaded
  // previously and it's returned without prompting for password
  {
    std::string k_wrong_pass_phrase = "wrong";
    const auto pk = Private_key_storage::instance().from_file(
        private_key_path, k_wrong_pass_phrase);
    EXPECT_EQ(k_fingerprint, pk.fingerprint());
  }
}

}  // namespace
}  // namespace ssl
}  // namespace shcore

# Copyright (c) 2020, 2024 Oracle and/or its affiliates.
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

IF(OPENSSL_INCLUDE_DIR)
  INCLUDE_DIRECTORIES(BEFORE ${OPENSSL_INCLUDE_DIR})
ENDIF()

set(oci_SOURCES
  api_key_credentials_provider.cc
  config_credentials_provider.cc
  data_plane_client.cc
  federation_client.cc
  federation_credentials_provider.cc
  fixed_credentials_provider.cc
  instance_metadata_retriever.cc
  instance_principal_credentials_provider.cc
  oci_bucket.cc
  oci_bucket_config.cc
  oci_bucket_options.cc
  oci_client.cc
  oci_credentials.cc
  oci_credentials_provider.cc
  oci_par.cc
  oci_regions.cc
  oci_setup.cc
  oci_signer.cc
  resource_principal_credentials_provider.cc
  security_token.cc
  security_token_credentials_provider.cc
)

add_convenience_library(oci ${oci_SOURCES})

target_link_libraries(oci utils)

if (WIN32)
  target_link_libraries(oci Crypt32)
endif()

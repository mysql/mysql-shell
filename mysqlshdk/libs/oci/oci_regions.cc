/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/oci/oci_regions.h"

#include <string_view>
#include <unordered_map>

namespace mysqlshdk {
namespace oci {
namespace regions {

namespace {

const std::unordered_map<std::string_view, std::string_view> k_short_names = {
    {"yny", "ap-chuncheon-1"},    {"hyd", "ap-hyderabad-1"},
    {"mel", "ap-melbourne-1"},    {"bom", "ap-mumbai-1"},
    {"kix", "ap-osaka-1"},        {"icn", "ap-seoul-1"},
    {"syd", "ap-sydney-1"},       {"nrt", "ap-tokyo-1"},
    {"yul", "ca-montreal-1"},     {"yyz", "ca-toronto-1"},
    {"ams", "eu-amsterdam-1"},    {"fra", "eu-frankfurt-1"},
    {"zrh", "eu-zurich-1"},       {"jed", "me-jeddah-1"},
    {"dxb", "me-dubai-1"},        {"gru", "sa-saopaulo-1"},
    {"cwl", "uk-cardiff-1"},      {"lhr", "uk-london-1"},
    {"iad", "us-ashburn-1"},      {"phx", "us-phoenix-1"},
    {"sjc", "us-sanjose-1"},      {"vcp", "sa-vinhedo-1"},
    {"scl", "sa-santiago-1"},     {"mtz", "il-jerusalem-1"},
    {"mrs", "eu-marseille-1"},    {"sin", "ap-singapore-1"},
    {"auh", "me-abudhabi-1"},     {"lin", "eu-milan-1"},
    {"arn", "eu-stockholm-1"},    {"jnb", "af-johannesburg-1"},
    {"cdg", "eu-paris-1"},        {"qro", "mx-queretaro-1"},
    {"mad", "eu-madrid-1"},       {"ord", "us-chicago-1"},
    {"mty", "mx-monterrey-1"},    {"aga", "us-saltlake-2"},
    {"bog", "sa-bogota-1"},       {"vap", "sa-valparaiso-1"},
    {"xsp", "ap-singapore-2"},    {"ruh", "me-riyadh-1"},
    {"onm", "ap-delhi-1"},        {"hsg", "ap-batam-1"},
    {"lfi", "us-langley-1"},      {"luf", "us-luke-1"},
    {"ric", "us-gov-ashburn-1"},  {"pia", "us-gov-chicago-1"},
    {"tus", "us-gov-phoenix-1"},  {"ltn", "uk-gov-london-1"},
    {"brs", "uk-gov-cardiff-1"},  {"nja", "ap-chiyoda-1"},
    {"ukb", "ap-ibaraki-1"},      {"mct", "me-dcc-muscat-1"},
    {"wga", "ap-dcc-canberra-1"}, {"bgy", "eu-dcc-milan-1"},
    {"mxp", "eu-dcc-milan-2"},    {"snn", "eu-dcc-dublin-2"},
    {"dtm", "eu-dcc-rating-2"},   {"dus", "eu-dcc-rating-1"},
    {"ork", "eu-dcc-dublin-1"},   {"dac", "ap-dcc-gazipur-1"},
    {"vll", "eu-madrid-2"},       {"str", "eu-frankfurt-2"},
    {"beg", "eu-jovanovac-1"},    {"doh", "me-dcc-doha-1"},
    {"ebb", "us-somerset-1"},     {"ebl", "us-thames-1"},
    {"avz", "eu-dcc-zurich-1"},   {"avf", "eu-crissier-1"},
    {"ahu", "me-abudhabi-3"},     {"rba", "me-alain-1"},
    {"rkt", "me-abudhabi-2"},     {"shj", "me-abudhabi-4"},
    {"dtz", "ap-seoul-2"},        {"dln", "ap-suwon-1"},
    {"bno", "ap-chuncheon-2"},    {"yxj", "us-ashburn-2"},
    {"pgc", "us-newark-1"},       {"jsk", "eu-budapest-1"}};

}  // namespace

std::string_view from_short_name(std::string_view short_name) {
  const auto it = k_short_names.find(short_name);

  if (k_short_names.end() != it) {
    return it->second;
  } else {
    return short_name;
  }
}

}  // namespace regions
}  // namespace oci
}  // namespace mysqlshdk

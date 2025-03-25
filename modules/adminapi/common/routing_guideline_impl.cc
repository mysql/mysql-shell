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

#include "modules/adminapi/common/routing_guideline_impl.h"
#include <sys/types.h>

#include <clocale>
#include <cstdint>
#include <exception>
#include <memory>
#include <numeric>
#include <optional>
#include <regex>
#include <stdexcept>

// from mysql sources
#include "routing_guidelines/routing_guidelines.h"

#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/base_cluster_impl.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/router.h"
#include "modules/adminapi/replica_set/replica_set_impl.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/db/mutable_result.h"
#include "mysqlshdk/libs/utils/version.h"
#include "shellcore/console.h"
#include "utils/document_parser.h"
#include "utils/utils_buffered_input.h"
#include "utils/utils_file.h"
#include "utils/utils_path.h"
#include "utils/utils_string.h"

namespace mysqlsh::dba {

// Guideline versioning:
//
// Guideline versions affect both shell and router.
// Shell can only support versions that the router code supports.
//
// Given a version <a>.<b>
// - increment <b> if expression syntax changes (new var/func added etc)
//    - guideline code in router needs to be updated, but shell just needs a
//    recompile
// - increment <a> if structure of the guideline changes
//    - guideline code needs to be updated in both router and shell
//

// the current (and max.) guideline version we support
static const mysqlshdk::utils::Version k_guideline_version("1.1");

// the min guideline version we support (should be 1.0 forever)
static const mysqlshdk::utils::Version k_min_guideline_version("1.0");

// version to assume if we encounter a guideline without a version field
static const mysqlshdk::utils::Version k_default_guideline_version("1.1");

namespace {
void check_if_shell_supports_guideline_version(
    const mysqlshdk::utils::Version &version) {
  // check if router lib supports this version
  if (version < k_min_guideline_version) {
    throw shcore::Exception("Routing guideline version " + version.get_full() +
                                " + is not supported",
                            SHERR_DBA_UNSUPPORTED_ROUTING_GUIDELINE_VERSION);
  }
  if (version > k_guideline_version) {
    throw shcore::Exception("Routing guideline version " + version.get_full() +
                                " is not supported, upgrade MySQL Shell",
                            SHERR_DBA_UNSUPPORTED_ROUTING_GUIDELINE_VERSION);
  }
}

mysqlshdk::utils::Version check_if_cluster_supports_guideline_version(
    Base_cluster_impl *owner, const mysqlshdk::utils::Version &version) {
  // check if we can use the given version considering what the
  // routers in the cluster support
  auto routers = owner->get_routers();

  // if there are no routers, then anything is ok
  if (routers.empty()) {
    return k_guideline_version;
  }

  // Get the highest guideline version supported
  mysqlshdk::utils::Version max_guideline_version_supported =
      mysqlshdk::utils::Version(0, 0);

  for (const auto &router_md : routers) {
    auto current_version = mysqlshdk::utils::Version(
        router_md.supported_routing_guidelines_version.value_or("0.0"));

    if (current_version > max_guideline_version_supported) {
      max_guideline_version_supported = current_version;
    }
  }

  if (version > max_guideline_version_supported) {
    throw shcore::Exception(
        "Routers in the " +
            to_display_string(owner->get_type(), Display_form::THING) +
            " must be upgraded before Routing guideline can be used",
        SHERR_DBA_ROUTER_UNSUPPORTED_FEATURE);
  }
  // if cluster supports version higher than us, then cap at what we
  // support
  if (max_guideline_version_supported > k_guideline_version) {
    return k_guideline_version;
  }

  return max_guideline_version_supported;
}

shcore::Value parse_destination_selector(const std::string &dest) {
  // format is strategy(destclass[, ...])
  std::regex re(
      R"(^\s*([a-zA-Z-]+)\s*\(\s*(([a-zA-Z0-9_]+|'[^']*'|"[^"]*")\s*(,\s*([a-zA-Z0-9_]+|'[^']*'|"[^"]*"))*)\s*\)\s*$)");
  std::smatch m;

  if (!std::regex_match(dest, m, re) || m.size() < 3) {
    throw std::runtime_error("Invalid syntax for destination selector: " +
                             dest);
  }

  auto is_quote = [](char c) { return c == '\'' || c == '"' || c == '`'; };

  std::string strategy = shcore::str_lower(std::string(m[1]));

  auto classes = shcore::make_array();
  // Split on commas
  for (auto s : shcore::str_split(m[2].str(), ",")) {
    std::string trimmed =
        shcore::str_strip(s);  // trim whitespaces from each class name

    if (!trimmed.empty() && is_quote(trimmed[0])) {  // Check for quotes
      trimmed = shcore::unquote_string(trimmed, trimmed[0]);  // Unquote
    }

    if (trimmed.empty()) {
      throw std::runtime_error("Invalid syntax for destination: " + dest);
    }
    classes->emplace_back(trimmed);  // Add trimmed class to array
  }

  return shcore::Value(shcore::make_dict("strategy", shcore::Value(strategy),
                                         "classes", shcore::Value(classes)));
}

shcore::Array_t parse_route_destinations(const shcore::Array_t &destinations) {
  if (destinations->empty()) return destinations;

  shcore::Array_t result = shcore::make_array();
  uint64_t priority = 0;

  for (const auto &dest : *destinations) {
    // assume raw destination in the serialized format
    if (dest.get_type() == shcore::Map) {
      result->push_back(dest);
      continue;
    }

    if (dest.get_type() != shcore::String) {
      throw std::runtime_error("Invalid destination selector: " +
                               dest.as_string());
    }

    auto parsed_destinations = parse_destination_selector(dest.as_string());

    auto dict = parsed_destinations.as_map();
    dict->set("priority", shcore::Value(priority));

    priority++;

    result->push_back(shcore::Value(dict));
  }

  return result;
}

std::string format_route_destination(const shcore::Dictionary_t &dest) {
  std::string policy = dest->get_string("strategy", "round-robin");
  std::string dests =
      shcore::str_join(*dest->get_array("classes"), ", ",
                       [](const shcore::Value &d) { return d.as_string(); });
  return policy + "(" + dests + ")";
}

std::string format_route_destinations(const shcore::Array_t &dests) {
  // Copy and sort destinations by priority first
  std::vector<shcore::Value> sorted_dests(dests->begin(), dests->end());

  std::sort(sorted_dests.begin(), sorted_dests.end(),
            [](const shcore::Value &a, const shcore::Value &b) {
              return a.as_map()->get_int("priority") <
                     b.as_map()->get_int("priority");
            });

  // Format sorted destinations
  return shcore::str_join(sorted_dests, ", ", [](const shcore::Value &dest) {
    return format_route_destination(dest.as_map());
  });
}
}  // namespace

class Guideline_classifier {
 public:
  explicit Guideline_classifier(const std::string &guideline)
      : m_guideline(
            routing_guidelines::Routing_guidelines_engine::create(guideline)) {}

  std::set<std::string> get_routes_for_destination_class(
      const std::string &destination) const {
    std::set<std::string> routes;

    for (auto &r : m_guideline.get_routes()) {
      for (const auto &destination_group : r.destination_groups) {
        for (const auto &dest : destination_group.destination_classes) {
          if (dest == destination) {
            routes.insert(r.name);
            break;
          }
        }
      }
    }

    return routes;
  }

  std::vector<std::string> classify_destination(
      const routing_guidelines::Server_info &server_info,
      const std::vector<routing_guidelines::Router_info> &router_info_list =
          {}) {
    std::vector<std::string> aggregated_class_names;
    std::set<std::string> unique_class_names;

    if (router_info_list.empty()) {
      // If no Router_info is provided, use a default Router_info based on
      // server_info
      routing_guidelines::Router_info default_router_info;
      default_router_info.local_cluster = server_info.cluster_name;

      routing_guidelines::Routing_guidelines_engine::Destination_classification
          res = m_guideline.classify(server_info, default_router_info);

      // Collect errors and class names
      for (const auto &error : res.errors) {
        std::cerr << error << "\n";
      }

      unique_class_names.insert(res.class_names.begin(), res.class_names.end());
    } else {
      // Use each Router_info in the list
      for (const auto &router_info : router_info_list) {
        routing_guidelines::Routing_guidelines_engine::
            Destination_classification res =
                m_guideline.classify(server_info, router_info);

        // Collect errors and class names
        for (const auto &error : res.errors) {
          std::cerr << error << "\n";
        }

        unique_class_names.insert(res.class_names.begin(),
                                  res.class_names.end());
      }
    }

    // Convert the unique set of class names to a vector
    aggregated_class_names.assign(unique_class_names.begin(),
                                  unique_class_names.end());

    return aggregated_class_names;
  }

 private:
  routing_guidelines::Routing_guidelines_engine m_guideline;
};

void Routing_guideline_impl::add_named_object_to_array(
    const std::string &field, const shcore::Dictionary_t &object, bool replace,
    std::optional<uint64_t> order) {
  if (!m_guideline_doc->has_key(field)) {
    m_guideline_doc->set(field, shcore::Value(shcore::make_array()));
  }

  auto array = m_guideline_doc->get_array(field);
  auto name = object->get_string("name", "");

  if (name.empty()) throw std::runtime_error("name field missing in object");

  for (auto it = array->begin(); it != array->end(); ++it) {
    it->check_type(shcore::Map);
    std::string iname = it->as_map()->get_string("name");
    if (iname == name) {
      if (replace) {
        *it = shcore::Value(object);
        return;
      } else {
        throw std::runtime_error("Duplicate name '" + name + "'");
      }
    }
  }

  // Order default is last. Append to the end if order is out of bounds
  if (!order.has_value() || order.value() > array->size()) {
    array->push_back(shcore::Value(object));
  } else {
    // Insert the object at the specified index
    array->insert(array->begin() + order.value_or(0), shcore::Value(object));
  }
}

shcore::Dictionary_t Routing_guideline_impl::get_named_object(
    const std::string &field, const std::string &name) {
  auto array = m_guideline_doc->get_array(field);
  if (array) {
    for (auto i = array->begin(); i != array->end(); ++i) {
      if (i->as_map()->get_string("name") == name) {
        return i->as_map();
      }
    }
  }
  return nullptr;
}

void Routing_guideline_impl::add_destination(const std::string &name,
                                             const std::string &match,
                                             bool dry_run, bool no_save) {
  // Destinations have the following format:
  //
  //  "destinations": [
  //  {
  //    "name": "d1",
  //    "match": "$.server.memberRole = SECONDARY"
  //  },
  //  {
  //    "name": "d2",
  //    "match": "$.server.memberRole = PRIMARY"
  //  }
  //]

  // Validate name
  dba::validate_name(name, Validation_context::ROUTING_GUIDELINE_DESTINATION);

  // Check if already exists
  if (get_named_object("destinations", name)) {
    throw shcore::Exception(
        shcore::str_format(
            "Destination '%s' already exists in the Routing Guideline",
            name.c_str()),
        SHERR_DBA_ROUTING_GUIDELINE_DUPLICATE_DESTINATION);
  }

  auto dest = shcore::make_dict("name", shcore::Value(name), "match",
                                shcore::Value(auto_escape_tags(match)));

  // Validate it using the Routing Guidelines engine
  routing_guidelines::Routing_guidelines_engine::validate_one_destination(
      shcore::Value(dest).json());

  if (dry_run) {
    current_console()->print_note("dryRun enabled, guideline not updated.");
  } else {
    add_named_object_to_array("destinations", dest);

    if (!no_save) {
      // Unset default_guideline flag and save the changes
      unset_as_default();
      save_guideline();
    }
  }

  current_console()->print_info(
      shcore::str_format("%sDestination '%s' successfully added.",
                         no_save ? "** " : "", name.c_str()));
}

void Routing_guideline_impl::add_route(const std::string &name,
                                       const std::string &match_expr,
                                       const shcore::Array_t &destinations,
                                       bool enabled,
                                       bool connection_sharing_allowed,
                                       std::optional<uint64_t> order,
                                       bool dry_run, bool no_save) {
  // Routes have the following format:
  //
  //   "routes": [
  //  {
  //    "name": "r1",
  //    "enabled": true,
  //    "match": "$.router.port.ro",
  //    "connectionSharingAllowed": true,
  //    "destinations": [
  //    {
  //      "classes": ["d1"],
  //      "strategy": "round-robin",
  //      "priority": 0
  //    },
  //    {
  //      "classes": ["d2"],
  //      "strategy": "round-robin",
  //      "priority": 1
  //    }
  //    ]
  //  }
  //]

  // Validate name
  dba::validate_name(name, Validation_context::ROUTING_GUIDELINE_ROUTE);

  if (get_named_object("routes", name)) {
    throw shcore::Exception(
        shcore::str_format("Route '%s' already exists in the Routing Guideline",
                           name.c_str()),
        SHERR_DBA_ROUTING_GUIDELINE_DUPLICATE_ROUTE);
  }

  auto parsed_destinations = parse_route_destinations(destinations);
  // check destinations
  for (const auto &ds : *parsed_destinations) {
    auto dest_sel = ds.as_map();
    if (dest_sel->count("classes")) {
      for (const auto &c : *dest_sel->get_array("classes")) {
        auto d = get_named_object("destinations", c.as_string());
        if (!d) {
          throw shcore::Exception(
              shcore::str_format("Invalid destination '%s' referenced in route",
                                 c.as_string().c_str()),
              SHERR_DBA_ROUTING_GUIDELINE_INVALID_DESTINATION_IN_ROUTE);
        }
      }
    }
  }

  auto route = shcore::make_dict(
      "name", shcore::Value(name), "match",
      shcore::Value(auto_escape_tags(match_expr)), "destinations",
      shcore::Value(parsed_destinations), "enabled", shcore::Value(enabled),
      "connectionSharingAllowed", shcore::Value(connection_sharing_allowed));

  routing_guidelines::Routing_guidelines_engine::validate_one_route(
      shcore::Value(route).json());

  if (dry_run) {
    mysqlsh::current_console()->print_note(
        "dryRun enabled, guideline not updated.");
  } else {
    add_named_object_to_array("routes", route, false, order);

    if (!no_save) {
      // Unset default_guideline flag and save the changes
      unset_as_default();
      save_guideline();
    }
  }

  current_console()->print_info(shcore::str_format(
      "%sRoute '%s' successfully added.", no_save ? "** " : "", name.c_str()));
}

std::shared_ptr<Guideline_classifier> Routing_guideline_impl::classifier()
    const {
  if (!m_classifier) {
    m_classifier =
        std::make_shared<Guideline_classifier>(shcore::Value(as_json()).json());
  }

  return m_classifier;
}

void Routing_guideline_impl::update_classifier() {
  m_classifier =
      std::make_shared<Guideline_classifier>(shcore::Value(as_json()).json());
}

void Routing_guideline_impl::remove_destination(const std::string &name) {
  // Check if the Destination exists
  if (!get_named_object("destinations", name)) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Destination '%s' not found.", name.c_str()));
  }

  // check if destination is in use
  auto routes = get_routes_for_destination(name);

  if (!routes.empty()) {
    current_console()->print_error(
        "Destination '" + name +
        "' is in use by route(s): " + shcore::str_join(routes, ", "));
    throw shcore::Exception::argument_error(
        "Destination in use by one or more Routes");
  }

  // Remove the destination
  remove_named_object("destinations", name);

  // Unset default_guideline flag and save the changes
  unset_as_default();
  save_guideline();

  current_console()->print_info("Destination successfully removed.");
}

void Routing_guideline_impl::remove_route(const std::string &name) {
  // Check if the Route exists
  if (!get_named_object("routes", name)) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Route '%s' not found.", name.c_str()));
  }

  // Check if the Route is the last one in the guideline
  if (m_guideline_doc->get_array("routes")->size() == 1) {
    current_console()->print_error(shcore::str_format(
        "Route '%s' is the only route in the Guideline and cannot be removed. "
        "A Routing Guideline must contain at least one route.",
        name.c_str()));
    throw shcore::Exception(
        "Cannot remove the last route of a Routing Guideline",
        SHERR_DBA_ROUTING_GUIDELINE_LAST_ROUTE);
  }

  // Remove the route
  remove_named_object("routes", name);

  // Unset default_guideline flag and save the changes
  unset_as_default();
  save_guideline();

  current_console()->print_info("Route successfully removed.");
}

void Routing_guideline_impl::show(
    const shcore::Option_pack_ref<Show_options> &options) const {
  auto console = mysqlsh::current_console();

  shcore::Array_t routes = m_guideline_doc->get_array("routes");
  shcore::Array_t destinations = m_guideline_doc->get_array("destinations");

  // Detect if any $.router.* variables are used in the destinations
  std::regex router_var(R"(\$\.\s*router\.\w+)");

  for (auto &dest : *destinations) {
    const auto &match_expr = dest.as_map()->get_string("match");
    std::smatch match;

    if (std::regex_search(match_expr, match, router_var) &&
        !options->router.has_value()) {
      console->print_error(shcore::str_format(
          "The Routing Guideline contains destinations configured to use "
          "router-specific variables (e.g., '%s'), but the 'router' option "
          "is not set. This may cause potential mismatches with the live "
          "router behavior. Please set the 'router' option to specify the "
          "Router that should be used for evaluation.",
          match.str().c_str()));

      throw shcore::Exception(
          "Option 'router' not set: The Routing Guideline contains "
          "destinations configured to use router-specific variables.",
          SHERR_DBA_ROUTING_GUIDELINE_ROUTER_OPTION_REQUIRED);
    }
  }

  // Print header
  console->print_info(
      shcore::str_format("Routing Guideline: '%s'", m_name.c_str()));
  console->print_info(shcore::str_format(
      "%s: '%s'",
      to_display_string(m_owner->get_type(), Display_form::API_CLASS).c_str(),
      m_owner->get_name().c_str()));
  console->print_info();

  // Classify topology
  auto [classified_destinations, unused_servers] =
      classify_topology(options->router.value_or(""));

  std::optional<Router_metadata> router_md_opt;
  std::unique_ptr<Router_configuration_schema> router_options_parsed;

  if (options->router.has_value()) {
    auto routers_md = m_owner->get_routers();

    auto it = std::find_if(
        routers_md.begin(), routers_md.end(), [&](const auto &router_md) {
          return shcore::str_format("%s::%s", router_md.hostname.c_str(),
                                    router_md.name.c_str()) ==
                 options->router.value();
        });

    if (it == routers_md.end()) {
      throw shcore::Exception(
          shcore::str_format("Router '%s' not found in the metadata.",
                             options->router.value().c_str()),
          SHERR_DBA_ROUTING_GUIDELINE_ROUTER_NOT_FOUND);
    }

    router_md_opt = *it;

    // Parse router options
    auto router_options_md =
        m_owner->get_metadata_storage()->get_router_options(m_owner->get_type(),
                                                            m_owner->get_id());
    router_options_parsed =
        std::make_unique<Router_configuration_schema>(shcore::Value(
            router_options_md.as_map()->get_map(options->router.value())));
  }

  // Helper to resolve endpoint information
  auto resolve_endpoint =
      [&](const std::string &match_expr) -> std::optional<std::string> {
    if (!router_md_opt) {
      return std::nullopt;  // Signal to print "All"
    }

    const auto &router_md = *router_md_opt;

    // Helper to get the bind_port for the "default" endpoints
    auto resolve_port_endpoint =
        [&](const std::optional<std::string> &port) -> std::string {
      if (!port.has_value() || port->empty()) {
        return "N/A";
      }

      auto endpoint_data = router_options_parsed->get_endpoint(*port);

      if (!endpoint_data) return "Unknown Endpoint";

      const auto &[endpoint_name, endpoint_schema] = *endpoint_data;
      std::string port_str;
      endpoint_schema.get_option_value("bind_port", port_str);

      return shcore::str_format("%s (port %s)", endpoint_name.c_str(),
                                port_str.empty() ? "N/A" : port_str.c_str());
    };

    // Resolve "default" endpoints:
    //   - $.router.port.ro -> router_md.ro_port -> bootstrap_ro
    //   - $.router.port.rw -> router_md.rw_port -> bootstrap_rw
    //   - $.router.port.rw_split -> router_md.rw_split_port ->
    //   bootstrap_rw_split
    if (match_expr.find("$.router.port.ro") != std::string::npos) {
      return resolve_port_endpoint(router_md.ro_port);
    } else if (match_expr.find("$.router.port.rw") != std::string::npos) {
      return resolve_port_endpoint(router_md.rw_port);
    } else if (match_expr.find("$.router.port.rw_split") != std::string::npos) {
      return resolve_port_endpoint(router_md.rw_split_port);
    } else if (match_expr.find("$.router.routeName") != std::string::npos) {
      // Get the routeName set in the match expression
      std::string route_name;
      std::regex route_name_regex(
          R"(\$\.\s*router\.routeName\s*=\s*'([^']+)')");
      std::smatch match;

      if (std::regex_search(match_expr, match, route_name_regex)) {
        route_name = match[1].str();
      }

      // Search for the route name in endpoints
      for (const auto &[endpoint_name, endpoint_schema] :
           router_options_parsed->get_all_endpoints()) {
        if (endpoint_name == route_name) {
          std::string port_str;
          endpoint_schema.get_option_value("bind_port", port_str);
          return shcore::str_format(
              "%s (port %s)", endpoint_name.c_str(),
              port_str.empty() ? "N/A" : port_str.c_str());
        }
      }

      // Not found
      return "Unknown Endpoint";
    }

    // No specific match expr: matches all
    return std::nullopt;
  };

  // Print Routes
  std::string base_label = "Routes";
  std::string routes_header =
      options->router.has_value()
          ? shcore::str_format("%s for '%s'", base_label.c_str(),
                               options->router.value().c_str())
          : base_label;
  console->print_info(routes_header);
  console->print_info(std::string(routes_header.length(), '-'));

  for (const auto &route : *routes) {
    auto route_map = route.as_map();
    std::string route_name = route_map->get_string("name");
    std::string match_expr = route_map->get_string("match");

    console->print_info(shcore::str_format("  - %s", route_name.c_str()));
    console->print_info(shcore::str_format("    + Match: \"%s\"",
                                           unescape_tags(match_expr).c_str()));

    // Only resolve endpoints if the 'router' option is provided
    if (options->router.has_value()) {
      auto endpoint_info = resolve_endpoint(match_expr);
      if (endpoint_info.has_value()) {
        console->print_info("    + Endpoints:");
        console->print_info(
            shcore::str_format("      * %s", endpoint_info->c_str()));
      } else {
        console->print_info("    + Endpoints: All");
      }
    }

    console->print_info("    + Destinations:");
    for (const auto &destination : *route_map->get_array("destinations")) {
      auto dest_map = destination.as_map();
      auto classes = dest_map->get_array("classes");

      for (const auto &cls : *classes) {
        std::string class_name = cls.as_string();

        if (auto it = classified_destinations.find(class_name);
            it != classified_destinations.end() && !it->second.empty()) {
          // Join instances and print them in a single line
          std::string instance_list = shcore::str_join(it->second, ", ");
          console->print_info(shcore::str_format(
              "      * %s (%s)", instance_list.c_str(), class_name.c_str()));
        } else {
          // Print None if no instances are available for the class
          console->print_info(
              shcore::str_format("      * None (%s)", class_name.c_str()));
        }
      }
    }
    console->print_info();
  }

  // Print Destination Classes
  console->print_info("Destination Classes");
  console->print_info("-------------------");

  for (const auto &dest : *destinations) {
    auto dest_map = dest.as_map();
    std::string name = dest_map->get_string("name");
    std::string match_expr = dest_map->get_string("match");

    // Print the class name and match expression
    console->print_info(shcore::str_format("  - %s:", name.c_str()));
    console->print_info(shcore::str_format("    + Match: \"%s\"",
                                           unescape_tags(match_expr).c_str()));

    // Print the Instances subsection
    console->print_info("    + Instances:");
    if (auto it = classified_destinations.find(name);
        it != classified_destinations.end()) {
      for (const auto &server : it->second) {
        console->print_info(shcore::str_format("      * %s", server.c_str()));
      }
    } else {
      console->print_info("      * None");
    }

    console->print_info();  // Ensure a newline after each class
  }

  // Print Unreferenced Servers
  console->print_info("Unreferenced servers");
  console->print_info("--------------------");

  if (unused_servers.empty()) {
    console->print_info("  - None");
  } else {
    for (const auto &server : unused_servers) {
      console->print_info(shcore::str_format("  - %s", server.c_str()));
    }
  }
}

void Routing_guideline_impl::add_default_destinations(Cluster_type type) {
  switch (type) {
    case Cluster_type::GROUP_REPLICATION:
      // routes and destinations for a plain cluster must work the same way if
      // the cluster is turned into a clusterset
      add_destination("Primary", "$.server.memberRole = PRIMARY", false, true);
      add_destination("Secondary", "$.server.memberRole = SECONDARY", false,
                      true);
      add_destination("ReadReplica", "$.server.memberRole = READ_REPLICA",
                      false, true);
      break;
    case Cluster_type::REPLICATED_CLUSTER:
      add_destination("Primary",
                      "$.server.memberRole = PRIMARY AND ($.server.clusterRole "
                      "= PRIMARY OR $.server.clusterRole = UNDEFINED)",
                      false, true);
      add_destination(
          "PrimaryClusterSecondary",
          "$.server.memberRole = SECONDARY AND ($.server.clusterRole = PRIMARY "
          "OR $.server.clusterRole = UNDEFINED)",
          false, true);
      add_destination("PrimaryClusterReadReplica",
                      "$.server.memberRole = READ_REPLICA AND "
                      "($.server.clusterRole = PRIMARY OR $.server.clusterRole "
                      "= UNDEFINED)",
                      false, true);
      break;
    case Cluster_type::ASYNC_REPLICATION:
      add_destination("Primary", "$.server.memberRole = PRIMARY", false, true);
      add_destination("Secondary", "$.server.memberRole = SECONDARY", false,
                      true);
      break;
    default:
      break;
  }
}

void Routing_guideline_impl::add_default_routes(
    Cluster_type type, const Read_only_targets Read_only_targets) {
  switch (type) {
    case Cluster_type::GROUP_REPLICATION: {
      // Add the "rw" route which is the same in all cases.
      add_route("rw", "$.session.targetPort = $.router.port.rw",
                shcore::make_array("round-robin(Primary)"), true, true,
                std::nullopt, false, true);
      // Handle each value of "read_only_targets" accordingly
      switch (Read_only_targets) {
        // Empty = unexisting = default = secondaries
        case Read_only_targets::NONE:
        case Read_only_targets::SECONDARIES: {
          add_route("ro", "$.session.targetPort = $.router.port.ro",
                    shcore::make_array("round-robin(Secondary)",
                                       "round-robin(Primary)"),
                    true, true, std::nullopt, false, true);
          break;
        }
        case Read_only_targets::READ_REPLICAS: {
          add_route("ro", "$.session.targetPort = $.router.port.ro",
                    shcore::make_array("round-robin(ReadReplica)",
                                       "round-robin(Primary)"),
                    true, true, std::nullopt, false, true);
          break;
        }
        case Read_only_targets::ALL: {
          add_route("ro", "$.session.targetPort = $.router.port.ro",
                    shcore::make_array("round-robin(Secondary, ReadReplica)",
                                       "round-robin(Primary)"),
                    true, true, std::nullopt, false, true);
          break;
        }
      }
      break;
    }
    case Cluster_type::REPLICATED_CLUSTER: {
      // Add the "rw" route which is the same in all cases.
      add_route("rw", "$.session.targetPort = $.router.port.rw",
                shcore::make_array("round-robin(Primary)"), true, true,
                std::nullopt, false, true);

      add_route("ro", "$.session.targetPort = $.router.port.ro",
                shcore::make_array("round-robin(PrimaryClusterSecondary)",
                                   "round-robin(Primary)"),
                true, true, std::nullopt, false, true);
      break;
    }
    case Cluster_type::ASYNC_REPLICATION: {
      add_route("rw", "$.session.targetPort = $.router.port.rw",
                shcore::make_array("round-robin(Primary)"), true, true,
                std::nullopt, false, true);

      add_route(
          "ro", "$.session.targetPort = $.router.port.ro",
          shcore::make_array("round-robin(Secondary)", "round-robin(Primary)"),
          true, true, std::nullopt, false, true);
      break;
    }
    default:
      break;
  }
}

std::set<std::string> Routing_guideline_impl::get_routes_for_destination(
    const std::string &destination_class) const {
  return classifier()->get_routes_for_destination_class(destination_class);
}

bool Routing_guideline_impl::remove_named_object(const std::string &field,
                                                 const std::string &name) {
  auto array = m_guideline_doc->get_array(field);

  if (array) {
    for (auto i = array->begin(); i != array->end(); ++i) {
      if (i->as_map()->get_string("name") == name) {
        array->erase(i);
        return true;
      }
    }
  }

  return false;
}

namespace {

/*
  struct RGUIDELINES_EXPORT Server_info {
    std::string label;         //!< hostname:port as in the metadata
    std::string address;       //!< address of the server
    uint16_t port{0};          //!< MySQL port number
    uint16_t port_x{0};        //!< X protocol port number
    std::string uuid;          //!< @@server_uuid of the server
    uint32_t version{0};       //!< server version in format 80401 for 8.4.1
    std::string member_role;   //!< PRIMARY, SECONDARY or READ_REPLICA, as
                               //!< reported by GR, empty string if not defined
    std::map<std::string, std::string, std::less<>>
        tags;                  //!< user defined tags stored in the cluster
                               //!< metadata for that Server instance
    std::string cluster_name;  //!< name of the cluster the server belongs to
    std::string
        cluster_set_name;      //!< name of the ClusterSet the server belongs to
    std::string cluster_role;  //!< PRIMARY or REPLICA depending on the role of
                               //!< the cluster in the ClusterSet, empty string
                               //!< if not defined
    bool cluster_is_invalidated;  //!< Cluster containing this server is
                                  //!< invalidated
  };
*/

// Helper to populate common Server_info fields
void populate_common_server_info(
    routing_guidelines::Server_info &info, const Instance_metadata &member,
    const std::string &endpoint,
    const std::shared_ptr<Instance> &instance = nullptr) {
  info.label = member.label;
  info.address = member.address;

  // Extract port from the endpoint
  info.port = std::stoi(shcore::str_split(endpoint, ":")[1]);
  info.uuid = member.uuid;

  // Set tags, if available
  if (member.tags) {
    for (const auto &tag : *member.tags) {
      info.tags[tag.first] =
          Routing_guideline_impl::serialize_value(tag.second);
    }
  }

  // Populate version if instance is available
  if (instance) {
    auto version = instance->get_version();
    info.version = version.numeric();
  }
}

std::vector<routing_guidelines::Server_info> get_all_members_server_info(
    Cluster_impl *cluster, Cluster_set_impl *cluster_set = nullptr) {
  std::vector<routing_guidelines::Server_info> ret;

  std::vector<mysqlshdk::gr::Member> gr_members;

  // Fetch GR members
  if (const auto &server = cluster->get_cluster_server(); server) {
    try {
      gr_members = mysqlshdk::gr::get_members(*server);
    } catch (const std::runtime_error &e) {
      log_info("Unable to get Group Replication membership info: '%s'",
               e.what());
    }
  }

  for (const auto &member : cluster->get_all_instances()) {
    routing_guidelines::Server_info info;
    std::shared_ptr<Instance> instance;

    auto m = std::find_if(gr_members.begin(), gr_members.end(),
                          [&member](const mysqlshdk::gr::Member &gr_member) {
                            return gr_member.uuid == member.uuid;
                          });

    // GR member, get the version from P_S
    if (m != gr_members.end() &&
        member.instance_type == Instance_type::GROUP_MEMBER) {
      mysqlshdk::utils::Version member_version =
          mysqlshdk::utils::Version(m->version);

      info.version = member_version.numeric();
      info.member_role = to_string(m->role);
    } else if (member.instance_type == Instance_type::READ_REPLICA) {
      // Read-Replica
      try {
        instance = cluster->get_session_to_cluster_instance(member.endpoint);
      } catch (const std::exception &) {
        log_info("Unable to connect to '%s'", member.endpoint.c_str());
        continue;
      }

      info.member_role = "READ_REPLICA";
    }

    // Populate common fields
    populate_common_server_info(info, member, member.endpoint, instance);

    // Set cluster-specific information
    info.cluster_name = cluster->get_name();
    if (cluster_set) {
      info.cluster_set_name = cluster_set->get_name();
      info.cluster_role = cluster->is_primary_cluster() ? "PRIMARY" : "REPLICA";
      info.cluster_is_invalidated = cluster->is_invalidated();
    } else {
      info.cluster_is_invalidated = false;
      info.cluster_role = "UNDEFINED";
    }

    ret.push_back(std::move(info));
  }

  return ret;
}

std::vector<routing_guidelines::Server_info> get_all_members_server_info(
    Cluster_set_impl *cluster_set) {
  std::vector<routing_guidelines::Server_info> ret;

  for (const auto &c : cluster_set->get_clusters()) {
    auto cluster = cluster_set->get_cluster_object(c, true);

    auto cluster_members_server_info =
        get_all_members_server_info(cluster.get(), cluster_set);

    ret.insert(ret.end(), cluster_members_server_info.begin(),
               cluster_members_server_info.end());
  }

  return ret;
}

std::vector<routing_guidelines::Server_info> get_all_members_server_info(
    Replica_set_impl *replicaset) {
  std::vector<routing_guidelines::Server_info> ret;

  auto primary = replicaset->get_primary_master();

  for (const auto &member : replicaset->get_instances_from_metadata()) {
    routing_guidelines::Server_info info;
    std::shared_ptr<Instance> instance;

    info.member_role =
        (member.uuid == primary->get_uuid()) ? "PRIMARY" : "SECONDARY";

    // Populate version
    try {
      instance = replicaset->get_session_to_cluster_instance(member.endpoint);
    } catch (const std::exception &) {
      log_info("Unable to connect to '%s'", member.endpoint.c_str());
      continue;
    }

    // Populate common fields
    populate_common_server_info(info, member, member.endpoint, instance);

    // Set ReplicaSet-specific information
    info.cluster_name = replicaset->get_name();

    ret.push_back(std::move(info));
  }

  return ret;
}

}  // namespace

std::pair<std::map<std::string, std::set<std::string>>,
          std::vector<std::string>>
Routing_guideline_impl::classify_topology(const std::string &router) const {
  std::map<std::string, std::set<std::string>> destinations;
  std::vector<std::string> unreferenced_servers;

  auto classify_destinations =
      [&](const std::vector<routing_guidelines::Server_info> &instances,
          const std::vector<routing_guidelines::Router_info>
              &router_info_list) {
        for (const auto &instance : instances) {
          // Classify the instance using all Router_info objects
          auto dest_names =
              classifier()->classify_destination(instance, router_info_list);

          // Add destinations to the destinations map
          for (const auto &d : dest_names) {
            destinations[d].insert(instance.address);
          }

          // If no destinations were matched, add to unreferenced_servers
          if (dest_names.empty()) {
            unreferenced_servers.push_back(instance.address);
          }
        }
      };

  // Build the Router_info object if 'router' is not empty
  /*
    struct ROUTING_GUIDELINES_EXPORT Router_info {
      // std::less<> is used to allow heterogeneous lookup since key type is
      // std::string
      using tags_t = std::map<std::string, std::string, std::less<>>;

      // port numbers configured for the named port configuration
      uint16_t port_ro{0};
      uint16_t port_rw{0};
      uint16_t port_rw_split{0};

      std::string local_cluster;  //!< name of the local cluster
      std::string hostname;       //!< hostname where router is running
      std::string bind_address;   //!< address on which router is listening
      tags_t tags;  //!< an object containing user defined tags stored in
                   //!< the cluster metadata for that Router instance
      std::string route_name;  //!< name of the plugin which handles the
                               //!< connection
    };
  */
  std::vector<routing_guidelines::Router_info> router_info_list;

  // Helper to populate Router_info
  auto populate_router_info =
      [&](routing_guidelines::Router_info &info, const Router_metadata &md,
          const std::string &endpoint_name,
          const Router_configuration_schema &schema, bool include_ports) {
        info.hostname = md.hostname;
        info.local_cluster = md.local_cluster.value_or("");
        if (include_ports) {
          info.port_ro = md.ro_port.has_value() ? std::stoul(*md.ro_port) : 0;
          info.port_rw = md.rw_port.has_value() ? std::stoul(*md.rw_port) : 0;
          info.port_rw_split =
              md.rw_split_port.has_value() ? std::stoul(*md.rw_split_port) : 0;
        }
        info.route_name = endpoint_name;
        schema.get_option_value("bind_address", info.bind_address);
        schema.get_option_value("tags", info.tags);
      };

  if (!router.empty()) {
    auto routers_md = m_owner->get_routers();

    // Lookup for 'router'
    auto it = std::find_if(
        routers_md.begin(), routers_md.end(), [&](const auto &router_md) {
          return shcore::str_format("%s::%s", router_md.hostname.c_str(),
                                    router_md.name.c_str()) == router;
        });

    // Not found
    if (it == routers_md.end()) {
      throw shcore::Exception(
          shcore::str_format(
              "Router '%s' is not registered in the %s", router.c_str(),
              to_display_string(m_owner->get_type(), Display_form::THING)
                  .c_str()),
          SHERR_DBA_ROUTING_GUIDELINE_ROUTER_NOT_FOUND);
    }

    const auto &router_md = *it;

    // Check if the router supports routing guidelines
    if (!router_md.supported_routing_guidelines_version.has_value()) {
      throw shcore::Exception(
          shcore::str_format("Router '%s' does not support Routing Guidelines",
                             router.c_str()),
          SHERR_DBA_ROUTER_UNSUPPORTED_FEATURE);
    }

    auto router_options_md =
        m_owner->get_metadata_storage()->get_router_options(m_owner->get_type(),
                                                            m_owner->get_id());
    auto router_options_parsed = Router_configuration_schema(
        shcore::Value(router_options_md.as_map()->get_map(router)));

    // Handle specific endpoints (RO, RW, RW_Split)
    auto handle_specific_port = [&](const std::string &port) {
      auto endpoint_data = router_options_parsed.get_endpoint(port);
      if (!endpoint_data) {
        mysqlsh::current_console()->print_warning(shcore::str_format(
            "Endpoint for port '%s' not found.", port.c_str()));
        return;
      }

      routing_guidelines::Router_info router_info;
      const auto &[endpoint_name, endpoint_schema] = *endpoint_data;

      populate_router_info(router_info, router_md, endpoint_name,
                           endpoint_schema, true);
      router_info_list.push_back(std::move(router_info));
    };

    // Add Router_info for the default ports: ro, rw, rw_split
    if (router_md.ro_port) {
      handle_specific_port(*router_md.ro_port);
    } else {
      mysqlsh::current_console()->print_warning(
          "Unable to get the listening port for classic protocol read-only "
          "connections (port.ro). This may cause potential mismatches with "
          "the live router behavior");
    }

    if (router_md.rw_port) {
      handle_specific_port(*router_md.rw_port);
    } else {
      mysqlsh::current_console()->print_warning(
          "Unable to get the listening port for classic protocol read-write "
          "connections (port.rw). This may cause potential mismatches with "
          "the live router behavior");
    }

    if (router_md.rw_split_port) {
      handle_specific_port(*router_md.rw_split_port);
    } else {
      mysqlsh::current_console()->print_warning(
          "Unable to get the listening port for classic protocol Read-Write "
          "splitting connections(port.rw_split). This may cause potential "
          "mismatches with the live router behavior");
    }

    // Handle remaining endpoints without setting ports
    for (const auto &[endpoint_name, endpoint_schema] :
         router_options_parsed.get_all_endpoints()) {
      if (std::any_of(router_info_list.begin(), router_info_list.end(),
                      [&](const routing_guidelines::Router_info &info) {
                        return info.route_name == endpoint_name;
                      })) {
        continue;
      }

      routing_guidelines::Router_info router_info;
      populate_router_info(router_info, router_md, endpoint_name,
                           endpoint_schema, false);
      router_info_list.push_back(std::move(router_info));
    }
  }

  // Classify destinations based on the owner type
  if (auto cluster = std::dynamic_pointer_cast<Cluster_impl>(m_owner)) {
    classify_destinations(get_all_members_server_info(cluster.get()),
                          router_info_list);
  } else if (auto cluster_set =
                 std::dynamic_pointer_cast<Cluster_set_impl>(m_owner)) {
    classify_destinations(get_all_members_server_info(cluster_set.get()),
                          router_info_list);
  } else if (auto replicaset =
                 std::dynamic_pointer_cast<Replica_set_impl>(m_owner)) {
    classify_destinations(get_all_members_server_info(replicaset.get()),
                          router_info_list);
  } else {
    throw std::logic_error("internal error");
  }

  return {destinations, unreferenced_servers};
}

std::string Routing_guideline_impl::serialize_value(
    const shcore::Value &value) {
  switch (value.get_type()) {
    case shcore::Bool:
    case shcore::Float:
    case shcore::UInteger:
    case shcore::Integer:
      return value.as_string();  // Convert numeric and boolean to string

    case shcore::String:
      return shcore::quote_string(value.get_string(), '\"');  // Escape strings

    case shcore::Array: {
      std::string array_string = "[";
      const auto &array = value.as_array();

      for (size_t i = 0; i < array->size(); ++i) {
        if (i > 0) array_string += ", ";

        array_string +=
            serialize_value(array->at(i));  // Recursively serialize elements
      }

      array_string += "]";

      return array_string;
    }

    case shcore::Map: {
      std::string map_string = "{";
      const auto &map = value.as_map();
      size_t count = 0;

      for (const auto &pair : *map) {
        if (count > 0) map_string += ", ";

        map_string +=
            shcore::quote_string(pair.first, '\"') + ": " +
            serialize_value(pair.second);  // Recursively serialize values

        ++count;
      }

      map_string += "}";
      return map_string;
    }

    case shcore::Null:
      return "null";

    case shcore::Undefined:
    case shcore::Function:
    case shcore::Binary:
    case shcore::Object:
    default:
      throw std::logic_error("Unable to parse document: unsupported JSON type");
  }
}

std::string Routing_guideline_impl::auto_escape_tags(
    const std::string &expression) const {
  // Auto-escape key-value types for the match expression, if version is 1.0.
  // Otherwise, keep the value unchanged
  if (m_version > k_min_guideline_version) {
    return expression;
  }

  // regex to match $.router.tags.* or $.server.tags.* with a value
  std::regex tag_regex(
      R"((\$\.(router|server)\.tags\.[a-zA-Z0-9_]+\s*=\s*)'([^']+)')");

  // Replace the string
  std::string result = std::regex_replace(expression, tag_regex, R"($1'"$3"')");

  return result;
}

std::string Routing_guideline_impl::unescape_tags(
    const std::string &expression) const {
  // Unescape only if version is 1.0.
  if (m_version > k_min_guideline_version) {
    return expression;
  }

  // Regex to match escaped double quotes within tag values
  std::regex unescape_regex(
      R"((\$\.(router|server)\.tags\.\w+\s*=\s*)'\"([^\"]+)\"')");

  // Replace the string by removing the extra quotes
  return std::regex_replace(expression, unescape_regex, R"($1'$3')");
}

void Routing_guideline_impl::ensure_unique_or_reuse(
    const std::shared_ptr<Base_cluster_impl> &base_topology, bool force) {
  std::string guideline_name = get_name();
  // Check if the guideline exists in the topology
  bool guideline_exists = base_topology->has_guideline(guideline_name);

  if (!guideline_exists) return;  // force is irrelevant

  if (!force) {
    throw shcore::Exception::argument_error(shcore::str_format(
        "A Routing Guideline with the name '%s' already exists",
        guideline_name.c_str()));
  } else {
    // Replacing the guideline: keep the original id from the owner
    set_id(base_topology->get_routing_guideline(base_topology, guideline_name)
               ->get_id());
  }
}

Routing_guideline_impl::Routing_guideline_impl(
    const std::shared_ptr<Base_cluster_impl> &owner, const std::string &name)
    : m_owner(owner),
      m_name(name),
      m_version(k_default_guideline_version),
      m_guideline_doc(
          shcore::make_dict("version", shcore::Value(m_version.get_full()),
                            "name", shcore::Value(m_name), "destinations",
                            shcore::Value(shcore::make_array()), "routes",
                            shcore::Value(shcore::make_array()))) {}

std::shared_ptr<Routing_guideline_impl> Routing_guideline_impl::create(
    const std::shared_ptr<Base_cluster_impl> &owner, const std::string &name,
    shcore::Dictionary_t json,
    const shcore::Option_pack_ref<Create_routing_guideline_options> &options) {
  auto console = mysqlsh::current_console();

  // Validate the options
  {
    // 'force' can only be used to override the current guideline with one
    // passed via 'json'
    if (options->get_force() && !json) {
      throw shcore::Exception(
          "The 'json' option is required when 'force' is enabled",
          SHERR_DBA_ROUTING_GUIDELINE_MISSING_GUIDELINE_OPTION);
    }

    // Validate the name
    if (!json) {
      dba::validate_name(name, Validation_context::ROUTING_GUIDELINE);
    }
  }

  auto guideline_impl = std::make_shared<Routing_guideline_impl>(owner, name);

  // Check if a Guideline with that name already exists. Skip if forced is
  // used and keep the same it if needed
  guideline_impl->ensure_unique_or_reuse(owner, options->get_force());

  if (json) {
    guideline_impl->parse(json);
  } else {
    console->print_info("Creating Default Routing Guideline...");

    // Set as default guideline.
    if (!options->get_force()) guideline_impl->set_as_default();

    auto topology_type = owner->get_type();
    auto read_only_targets = owner->get_read_only_targets_option();

    console->print_info();
    console->print_info("* Adding default destinations...");
    guideline_impl->add_default_destinations(topology_type);

    console->print_info();
    console->print_info("* Adding default routes...");
    guideline_impl->add_default_routes(topology_type, read_only_targets);
  }

  // Store the guideline in the topology and print informative messages
  owner->store_routing_guideline(guideline_impl);

  console->print_info();
  console->print_info(shcore::str_format(
      "Routing Guideline '%s' successfully created.", name.c_str()));
  console->print_info();

  return guideline_impl;
}

std::shared_ptr<Routing_guideline_impl> Routing_guideline_impl::load(
    const std::shared_ptr<Base_cluster_impl> &owner, const std::string &name) {
  Routing_guideline_metadata guideline_md =
      owner->get_metadata_storage()->get_routing_guideline(
          owner->get_type(), owner->get_id(), name);

  auto rg = std::make_shared<Routing_guideline_impl>(owner, name);
  rg->m_id = guideline_md.id;

  if (guideline_md.is_default_guideline) rg->set_as_default();

  rg->parse(shcore::Value::parse(guideline_md.guideline).as_map());

  return rg;
}

std::shared_ptr<Routing_guideline_impl> Routing_guideline_impl::load(
    const std::shared_ptr<Base_cluster_impl> &owner,
    const Routing_guideline_metadata &guideline) {
  auto rg = std::make_shared<Routing_guideline_impl>(owner, guideline.name);

  rg->m_id = guideline.id;
  rg->m_name = guideline.name;

  if (guideline.is_default_guideline) rg->set_as_default();

  rg->parse(shcore::Value::parse(guideline.guideline).as_map());

  return rg;
}

std::shared_ptr<Routing_guideline_impl> Routing_guideline_impl::import(
    const std::shared_ptr<Base_cluster_impl> &owner,
    const std::string &file_path,
    const shcore::Option_pack_ref<Import_routing_guideline_options> &options) {
  auto console = mysqlsh::current_console();

  auto read_json_file = [&](const std::string &full_path) {
    shcore::Buffered_input input{};
    input.open(full_path);
    std::string document;

    try {
      shcore::Document_reader_options opts;
      shcore::Json_reader reader(&input, opts);
      reader.parse_bom();

      size_t docs_number = 0;

      while (!reader.eof()) {
        ++docs_number;
        std::string jd = reader.next();

        if (jd.empty()) {
          throw shcore::Exception::runtime_error(shcore::str_format(
              "'%s' is not a valid file or contains no valid documents.",
              full_path.c_str()));
        }

        if (docs_number > 1) {
          throw shcore::Exception::runtime_error(shcore::str_format(
              "'%s' contains multiple documents, which is not supported.",
              full_path.c_str()));
        }

        document = std::move(jd);
      }

      if (!docs_number) {
        throw shcore::Exception::runtime_error(
            shcore::str_format("'%s' is empty.", full_path.c_str()));
      }
    } catch (std::exception &e) {
      throw shcore::Exception::runtime_error(shcore::str_format(
          "Invalid Routing Guideline document: %s", e.what()));
    }

    return document;
  };

  // Check if the path is not empty
  if (file_path.empty()) {
    throw shcore::Exception::argument_error(
        "The input file path cannot be empty");
  }

  // Expand the user path
  std::string full_path = shcore::path::expand_user(file_path);

  // Load and parse the file
  auto parsed_document = read_json_file(full_path);

  // Instantiate a Routing_guideline_impl object for the parsed doc
  auto guideline_impl = std::make_shared<Routing_guideline_impl>(owner);
  guideline_impl->parse(shcore::Value::parse(parsed_document).as_map());

  // Handle options: 'force' or 'rename' (mutually exclusive)
  if (!options->rename.has_value()) {
    // Check if a Guideline with that name already exists. Skip if 'force' is
    // used and keep the same if needed
    guideline_impl->ensure_unique_or_reuse(owner, options->get_force());
  } else {
    // Rename the guideline but first check if a guideline with the new name
    // already exists
    const auto &new_name = *options->rename;

    if (owner->has_guideline(new_name)) {
      throw shcore::Exception::argument_error(shcore::str_format(
          "A Routing Guideline with the name '%s' already exists",
          new_name.c_str()));
    }

    // Rename the parsed guideline
    guideline_impl->set_name(new_name);
  }

  // Store the guideline in the topology and print informative messages
  owner->store_routing_guideline(guideline_impl);

  console->print_info();
  console->print_info(
      shcore::str_format("Routing Guideline '%s' successfully imported.",
                         guideline_impl->get_name().c_str()));
  console->print_info();

  return guideline_impl;
}

shcore::Dictionary_t Routing_guideline_impl::as_json() const {
  return m_guideline_doc;
}

std::unique_ptr<mysqlshdk::db::IResult>
Routing_guideline_impl::get_destinations() const {
  using mysqlshdk::db::Mutable_result;

  static std::vector<mysqlshdk::db::Column> columns(
      {Mutable_result::make_column("destination", mysqlshdk::db::Type::String),
       Mutable_result::make_column("match", mysqlshdk::db::Type::String)});

  auto result = std::make_unique<Mutable_result>(columns);

  if (auto dests = m_guideline_doc->get_array("destinations")) {
    for (auto &r : *dests) {
      auto &row = result->add_row();

      row.set_field(0, r.as_map()->get_string("name"));
      row.set_field(1, unescape_tags(r.as_map()->get_string("match")));
    }
  }

  return result;
}

std::unique_ptr<mysqlshdk::db::IResult> Routing_guideline_impl::get_routes()
    const {
  using mysqlshdk::db::Mutable_result;
  using mysqlshdk::db::Type;

  static std::vector<mysqlshdk::db::Column> columns(
      {Mutable_result::make_column("name"),
       Mutable_result::make_column("enabled", Type::Integer),
       Mutable_result::make_column("shareable", Type::Integer),
       Mutable_result::make_column("match"),
       Mutable_result::make_column("destinations"),
       Mutable_result::make_column("order", Type::Integer)});

  auto result = std::make_unique<Mutable_result>(columns);

  if (auto routes = m_guideline_doc->get_array("routes")) {
    int order = 0;

    for (auto &r : *routes) {
      auto &row = result->add_row();

      row.set_row_values(
          r.as_map()->get_string("name"), r.as_map()->get_int("enabled"),
          r.as_map()->get_int("connectionSharingAllowed"),
          unescape_tags(r.as_map()->get_string("match")),
          format_route_destinations(r.as_map()->get_array("destinations")),
          order);

      order++;
    }
  }

  return result;
}

void Routing_guideline_impl::parse(const shcore::Dictionary_t &json,
                                   bool dry_run) {
  // ensure no funny stuff in the dict
  auto guideline = shcore::Value::parse(shcore::Value(json).json()).as_map();

  // ensure name isn't overwritten
  if (!m_name.empty()) {
    guideline->set("name", shcore::Value(m_name));
  } else if (guideline->has_key("name")) {
    m_name = guideline->get_string("name");
  }

  dba::validate_name(m_name, Validation_context::ROUTING_GUIDELINE);

  if (guideline->has_key("version")) {
    try {
      m_version = mysqlshdk::utils::Version(guideline->get_string("version"));
    } catch (...) {
      throw shcore::Exception(
          shcore::str_format("Invalid routing guideline version '%s'",
                             guideline->at("version").descr().c_str()),
          SHERR_DBA_ROUTING_GUIDELINE_INVALID_VERSION);
    }

    // Check if the version includes patch
    if (m_version.has_patch() || !m_version.get_extra().empty()) {
      throw shcore::Exception(
          shcore::str_format("Invalid routing guideline version format '%s': "
                             "expected format is 'x.y'.",
                             guideline->at("version").descr().c_str()),
          SHERR_DBA_ROUTING_GUIDELINE_INVALID_VERSION);
    }
  } else {
    m_version = k_default_guideline_version;
    guideline->set("version", shcore::Value(m_version.get_full()));
  }

  // check if we can handle this version
  check_if_shell_supports_guideline_version(m_version);

  auto max_version =
      check_if_cluster_supports_guideline_version(m_owner.get(), m_version);

  // NOTE: cluster supports higher version, so upgrade here if/when new
  // version is added

  // validate
  try {
    (void)Guideline_classifier(shcore::Value(guideline).json());

    // Validate names across routes and destinations
    for (const auto &dest : *guideline->get_array("destinations")) {
      dba::validate_name(dest.as_map()->get_string("name"),
                         Validation_context::ROUTING_GUIDELINE_DESTINATION);
    }

    for (const auto &route : *guideline->get_array("routes")) {
      dba::validate_name(route.as_map()->get_string("name"),
                         Validation_context::ROUTING_GUIDELINE_ROUTE);
    }
  } catch (...) {
    throw std::runtime_error("Invalid guideline document: " +
                             format_active_exception());
  }

  if (!dry_run) m_guideline_doc = guideline;
}

void Routing_guideline_impl::save_guideline() {
  Routing_guideline_metadata guideline;
  guideline.name = get_name();
  guideline.guideline = shcore::Value(as_json()).json();
  guideline.is_default_guideline = is_default();

  if (m_owner->get_type() == Cluster_type::REPLICATED_CLUSTER) {
    guideline.clusterset_id = m_owner->get_id();
  } else {
    guideline.cluster_id = m_owner->get_id();
  }

  // get the highest guideline version that we can use in the cluster
  auto version = check_if_cluster_supports_guideline_version(
      m_owner.get(), k_min_guideline_version);
  auto metadata = m_owner->get_metadata_storage();

  // 1st time save
  if (m_id.empty()) {
    m_id = metadata->create_routing_guideline(guideline);
  } else {
    guideline.id = m_id;
    metadata->update_routing_guideline(guideline);
  }

  // Update the classifier
  update_classifier();
}

void Routing_guideline_impl::upgrade_routing_guideline_to_clusterset(
    const Cluster_set_id &cluster_set_id) {
  Routing_guideline_metadata guideline;
  auto console = mysqlsh::current_console();

  // Keep the id and name
  guideline.id = m_id;
  guideline.name = get_name();

  // Set the ClusterSet id and clear the Cluster_id
  guideline.clusterset_id = cluster_set_id;
  guideline.cluster_id = "";

  if (is_default()) {
    console->print_info();
    console->print_info(
        shcore::str_format("* Upgrading Default Routing Guideline '%s'...",
                           guideline.name.c_str()));
    // Remove all destinations and routes
    m_guideline_doc->erase("destinations");
    console->print_info("** Default destinations successfully removed...");

    m_guideline_doc->erase("routes");
    console->print_info("** Default routes successfully removed...");

    // Add ClusterSet default destinations and routes
    add_default_destinations(Cluster_type::REPLICATED_CLUSTER);
    add_default_routes(Cluster_type::REPLICATED_CLUSTER);

    guideline.is_default_guideline = true;
  } else {
    console->print_info();
    console->print_warning(shcore::str_format(
        "The Routing Guideline '%s' may be tailored for a standalone Cluster "
        "deployment and might not be suitable for use with a ClusterSet. "
        "Please review and adjust this guideline as necessary after the "
        "upgrade.",
        guideline.name.c_str()));
  }

  // Check if there any Routers bootstrapped on the Cluster to warn the user
  // about the need to re-bootstrap, otherwise, Router's won't recognize the
  // topology and behave as if the Cluster is standalone.
  if (!m_owner->get_metadata_storage()
           ->get_clusterset_routers(cluster_set_id)
           .empty()) {
    console->print_info();
    console->print_warning(
        "Detected Routers that were bootstrapped before the ClusterSet was "
        "created. Please re-bootstrap the Routers to ensure the ClusterSet "
        "is recognized and the configurations are updated. Otherwise, "
        "Routers will operate as if the Clusters were standalone.");
  }

  // Update the document
  guideline.guideline = shcore::Value(as_json()).json();

  // Save in the metadata
  m_owner->get_metadata_storage()->update_routing_guideline(guideline);
}

void Routing_guideline_impl::downgrade_routing_guideline_to_cluster(
    const Cluster_id &cluster_id) {
  Routing_guideline_metadata guideline;
  auto console = mysqlsh::current_console();

  // Keep the id and name
  guideline.id = m_id;
  guideline.name = get_name();

  // Clear the clusterset_id and set the cluster_id
  guideline.clusterset_id = "";
  guideline.cluster_id = cluster_id;

  // Update the document
  guideline.guideline = shcore::Value(as_json()).json();

  // Save in the metadata
  m_owner->get_metadata_storage()->update_routing_guideline(guideline);
}

void Routing_guideline_impl::validate_or_throw_if_invalid() {
  // Check if the Cluster belongs to a ClusterSet, if so and we reached here it
  // means the object was created before the Cluster was upgraded to a
  // ClusterSet
  if (auto cluster = std::dynamic_pointer_cast<Cluster_impl>(m_owner)) {
    if (cluster->is_cluster_set_member()) {
      m_is_invalidated = true;
    }
  }

  if (!m_is_invalidated) return;

  current_console()->print_error(
      "The Routing Guideline object was created when the Cluster was not part "
      "of a ClusterSet. Since the Cluster now belongs to a ClusterSet, this "
      "object is invalid. Please obtain a new Routing Guideline object using "
      "<ClusterSet>.<<<getRoutingGuideline()>>>.");

  throw shcore::Exception::runtime_error("Invalid Routing Guideline object");
}

void Routing_guideline_impl::rename(const std::string &name) {
  std::string original_name = m_name;
  // Validate the given name first
  dba::validate_name(name, Validation_context::ROUTING_GUIDELINE);

  // Check if the name is not already in use by other guideline
  if (m_owner->has_guideline(name)) {
    throw shcore::Exception::argument_error(shcore::str_format(
        "A Routing Guideline with the name '%s' already exists.",
        name.c_str()));
  }

  set_name(name);

  save_guideline();

  mysqlsh::current_console()->print_info(
      shcore::str_format("Successfully renamed Routing Guideline '%s' to '%s'",
                         original_name.c_str(), name.c_str()));
}

void Routing_guideline_impl::set_destination_option(
    const std::string &destination_name, const std::string &option,
    const shcore::Value &value) {
  // Check if the destination exists
  if (!get_named_object("destinations", destination_name)) {
    throw shcore::Exception(shcore::str_format("Destination '%s' not found",
                                               destination_name.c_str()),
                            SHERR_DBA_ROUTING_GUIDELINE_DESTINATION_NOT_FOUND);
  }

  // Validate the accepted options: only 'match' supported
  if (std::find(k_routing_guideline_set_destination_options.begin(),
                k_routing_guideline_set_destination_options.end(),
                option) == k_routing_guideline_set_destination_options.end()) {
    // Create a comma-separated list of the accepted values
    std::string accepted_values = std::accumulate(
        std::next(k_routing_guideline_set_destination_options.begin()),
        k_routing_guideline_set_destination_options.end(),
        std::string(k_routing_guideline_set_destination_options[0]),
        [](const std::string &a, const std::string_view b) {
          return a + ", " + std::string(b);
        });

    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value for destination option, accepted values: %s",
        accepted_values.c_str()));
  }

  // Validate the value type
  if (value.get_type() != shcore::Value_type::String) {
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value for destination option '%s', value is "
        "expected to be a string.",
        k_routing_guideline_set_option_match));
  }

  // Validate the value
  auto dest =
      shcore::make_dict("name", shcore::Value(destination_name), "match",
                        shcore::Value(auto_escape_tags(value.as_string())));

  routing_guidelines::Routing_guidelines_engine::validate_one_destination(
      shcore::Value(dest).json());

  // Replace it in the guideline
  add_named_object_to_array("destinations", dest, true);

  // Unset default_guideline flag and save the changes
  unset_as_default();
  save_guideline();

  current_console()->print_info(shcore::str_format(
      "Destination '%s' successfully updated.", destination_name.c_str()));
}

void Routing_guideline_impl::set_route_option(const std::string &route_name,
                                              const std::string &option,
                                              const shcore::Value &value) {
  // Check if the route exists
  if (!get_named_object("routes", route_name)) {
    throw shcore::Exception(
        shcore::str_format("Route '%s' not found", route_name.c_str()),
        SHERR_DBA_ROUTING_GUIDELINE_ROUTE_NOT_FOUND);
  }

  // Validate the accepted options: match, destinations, enabled, priority
  if (std::find(k_routing_guideline_set_route_options.begin(),
                k_routing_guideline_set_route_options.end(),
                option) == k_routing_guideline_set_route_options.end()) {
    // Create a comma-separated list of the accepted values
    std::string accepted_values = std::accumulate(
        std::next(k_routing_guideline_set_route_options.begin()),
        k_routing_guideline_set_route_options.end(),
        std::string(k_routing_guideline_set_route_options[0]),
        [](const std::string &a, const std::string_view b) {
          return a + ", " + std::string(b);
        });

    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value for route option, accepted values: %s",
        accepted_values.c_str()));
  }

  // Validate the value type for the option
  {
    // Define expected types for each option
    const std::map<std::string, shcore::Value_type> option_types = {
        {k_routing_guideline_set_option_match, shcore::Value_type::String},
        {k_routing_guideline_set_route_option_destinations,
         shcore::Value_type::Array},
        {k_routing_guideline_set_route_option_enabled,
         shcore::Value_type::Bool},
        {k_routing_guideline_set_route_option_connectionsharingallowed,
         shcore::Value_type::Bool},
        {k_routing_guideline_set_route_option_order,
         shcore::Value_type::Integer}};

    auto expected_type = option_types.find(option);
    if (expected_type != option_types.end() &&
        value.get_type() != expected_type->second) {
      throw shcore::Exception::argument_error(shcore::str_format(
          "Invalid value for route option '%s', value is "
          "expected to be %s.",
          option.c_str(), type_description(expected_type->second).c_str()));
    }
  }

  // Get the current values of the route
  std::shared_ptr<shcore::Value::Map_type> route_map;

  if (auto routes = m_guideline_doc->get_array("routes")) {
    for (auto &r : *routes) {
      if (r.as_map()->get_string("name") == route_name) {
        // Create a deep copy of the map
        route_map = std::make_shared<shcore::Value::Map_type>(*r.as_map());
        break;
      }
    }
  } else {
    // Shouldn't happen
    throw std::logic_error("internal error");
  }

  // Retrieve the current option value
  if (option != k_routing_guideline_set_route_option_order) {
    shcore::Value current_value = route_map->at(option);

    // Check if the current value matches the new value
    if (current_value == value) {
      current_console()->print_info("No changes detected, skipping update.");
      return;
    }
  }

  // Destinations must be checked separately because validate_one_route() does
  // not check if the destination class is configured, it does not have this
  // information. This must be done at document-level
  shcore::Value parsed_value = value;

  if (option == k_routing_guideline_set_route_option_destinations) {
    auto route_destinations = parse_route_destinations(value.as_array());

    for (const auto &ds : *route_destinations) {
      auto dest_sel = ds.as_map();
      if (dest_sel->count("classes")) {
        for (const auto &c : *dest_sel->get_array("classes")) {
          auto d = get_named_object("destinations", c.as_string());
          if (!d) {
            throw std::runtime_error("Invalid destination '" + c.as_string() +
                                     "' referenced in route '" + route_name +
                                     "'");
          }
        }
      }
    }

    parsed_value = shcore::Value(route_destinations);
  }

  // Validate and update route property with the new value
  if (option != k_routing_guideline_set_route_option_order) {
    // Auto-escape key-value types for the match expression, if version 1.0
    if (option == k_routing_guideline_set_option_match) {
      parsed_value = shcore::Value(auto_escape_tags(parsed_value.as_string()));
    }

    route_map->set(option, parsed_value);

    routing_guidelines::Routing_guidelines_engine::validate_one_route(
        shcore::Value(route_map).json());

    // Replace it in the guideline
    add_named_object_to_array("routes", route_map, true);
  } else {
    int new_priority = parsed_value.as_int();
    // Retrieve and sort routes based on updated priority
    auto routes_array = m_guideline_doc->get_array("routes");

    // Remove the route from its current position
    routes_array->erase(
        std::remove_if(routes_array->begin(), routes_array->end(),
                       [&](const shcore::Value &r) {
                         return r.as_map()->get_string("name") == route_name;
                       }),
        routes_array->end());

    // Insert the route at the new priority position
    if (static_cast<size_t>(new_priority) >= routes_array->size()) {
      routes_array->push_back(shcore::Value(route_map));  // Append to the end
    } else {
      routes_array->insert(routes_array->begin() + new_priority,
                           shcore::Value(route_map));
    }

    // Update m_guideline_doc with the reordered routes
    m_guideline_doc->set("routes", shcore::Value(routes_array));
  }

  // Unset default_guideline flag and save the changes
  unset_as_default();
  save_guideline();

  current_console()->print_info(shcore::str_format(
      "Route '%s' successfully updated.", route_name.c_str()));
}

std::shared_ptr<Routing_guideline_impl> Routing_guideline_impl::copy(
    const std::string &name) {
  // Validate name
  dba::validate_name(name, Validation_context::ROUTING_GUIDELINE);

  // Check if a Guideline with that name already exists
  if (m_owner->has_guideline(name)) {
    throw shcore::Exception::argument_error(shcore::str_format(
        "A Routing Guideline with the name '%s' already exists.",
        name.c_str()));
  }

  // Duplicate the guideline
  auto copy = std::make_shared<Routing_guideline_impl>(m_owner, name);

  // Deep copy the guideline doc
  shcore::Dictionary_t guideline_copy = shcore::make_dict();

  for (const auto &[key, value] : *m_guideline_doc) {
    // Update the name in the deep-copied dictionary
    if (key == "name") {
      guideline_copy->set(key, shcore::Value(name));
      continue;
    }

    guideline_copy->set(key, value);
  }

  // Assign the deep-copied doc to the copy
  copy->m_guideline_doc = guideline_copy;

  // Save it to the metadata
  copy->save_guideline();

  current_console()->print_info(shcore::str_format(
      "Routing Guideline '%s' successfully duplicated with new name '%s'.",
      m_name.c_str(), name.c_str()));

  return copy;
}

void Routing_guideline_impl::export_to_file(const std::string &file_path) {
  // Check if the file path is empty
  if (file_path.empty()) {
    throw shcore::Exception::argument_error(
        "The output file path cannot be empty.");
  }

  // Check if the file already exists
  if (shcore::is_file(file_path)) {
    throw shcore::Exception::argument_error(shcore::str_format(
        "The file '%s' already exists. Please provide a different file path.",
        file_path.c_str()));
  }

  // Write the guideline to file
  if (!shcore::create_file(file_path, shcore::Value(as_json()).json(true))) {
    throw std::runtime_error("Failed to open " + file_path + " for writing.");
  }

  current_console()->print_info(shcore::str_format(
      "Routing Guideline '%s' successfully exported to '%s'.", m_name.c_str(),
      file_path.c_str()));
}

}  // namespace mysqlsh::dba

/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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

#include <climits>
#include <string>

#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/gtid_validations.h"
#include "mysqlshdk/include/scripting/shexcept.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/textui/textui.h"

namespace mysqlsh {
namespace dba {

namespace {
enum Prompt_type {
  None,
  Clone_incremental_abort,
  Clone_abort,
  Incremental_abort
};

void msg_recovery_possible_and_safe(
    Cluster_type cluster_type, Member_recovery_method recovery_method,
    mysqlshdk::mysql::IInstance *target_instance, bool clone_only = false) {
  auto console = current_console();

  if (recovery_method == Member_recovery_method::AUTO) {
    console->print_info(
        "The safest and most convenient way to provision a new instance is "
        "through automatic clone provisioning, which will completely overwrite "
        "the state of '" +
        target_instance->descr() +
        "' with a physical snapshot from an existing " + thing(cluster_type) +
        " member. To use this method by default, set the 'recoveryMethod' "
        "option to 'clone'.");

    if (!clone_only) {
      console->print_info();

      switch (cluster_type) {
        case Cluster_type::GROUP_REPLICATION:
          console->print_info(
              "The incremental state recovery may be safely used if you are "
              "sure all updates ever executed in the cluster were done with "
              "GTIDs enabled, there are no purged transactions and the new "
              "instance contains the same GTID set as the cluster or a subset "
              "of it. To use this method by default, set the 'recoveryMethod' "
              "option to 'incremental'.");
          break;

        case Cluster_type::ASYNC_REPLICATION:
          console->print_warning(
              "It should be safe to rely on replication to incrementally "
              "recover the state of the new instance if you are sure all "
              "updates ever executed in the replicaset were done with GTIDs "
              "enabled, there are no purged transactions and the new instance "
              "contains the same GTID set as the replicaset or a subset of it. "
              "To use this method by default, set the 'recoveryMethod' option "
              "to 'incremental'.");
          break;

        case Cluster_type::REPLICATED_CLUSTER:
          console->print_warning(
              "It should be safe to rely on replication to incrementally "
              "recover the state of the new Replica Cluster if you are sure "
              "all updates ever executed in the ClusterSet were done with "
              "GTIDs enabled, there are no purged transactions and the "
              "instance used to create the new Replica Cluster contains the "
              "same GTID set as the ClusterSet or a subset of it. To use this "
              "method by default, set the 'recoveryMethod' option to "
              "'incremental'.");
          break;

        default:
          throw std::logic_error("Internal error");
      }
    }
  }
}

void validate_clone_recovery(bool clone_disabled) {
  auto console = current_console();
  std::string error;

  // User explicitly selected clone, so just do it.
  // Check for whether clone is supported was already done in Clone_options
  if (clone_disabled) {
    error =
        "Cannot use recoveryMethod=clone option because the disableClone "
        "option was set for the cluster.";
  }

  if (!error.empty()) {
    throw shcore::Exception::runtime_error(error);
  }
}

void validate_incremental_recovery(Member_op_action op_action,
                                   bool recovery_possible, bool recovery_safe) {
  std::string error;

  if ((op_action == Member_op_action::REJOIN_INSTANCE && !recovery_safe) ||
      !recovery_possible) {
    error =
        "Cannot use recoveryMethod=incremental option because the GTID "
        "state is not compatible or cannot be recovered.";
  }

  if (!error.empty()) {
    throw shcore::Exception::runtime_error(error);
  }
}

Prompt_type validate_auto_recovery(Cluster_type cluster_type,
                                   Member_op_action op_action,
                                   bool recovery_possible, bool recovery_safe,
                                   bool clone_supported, bool gtid_set_diverged,
                                   bool interactive, bool clone_disabled) {
  auto console = current_console();

  Prompt_type prompt = None;

  std::string clone_available_since =
      "Built-in clone support is available starting with MySQL 8.0.17 and is "
      "the recommended method for provisioning instances.";

  std::string target_clone_or_provisioned_add =
      "The target instance must be either cloned or fully provisioned before "
      "it can be added to the target " +
      thing(cluster_type) + ".";

  std::string target_clone_or_provisioned_rejoin =
      "The target instance must be either cloned or fully re-provisioned "
      "before it can be re-added to the target " +
      thing(cluster_type) + ".";

  if (recovery_possible) {
    if (!recovery_safe) {
      if (clone_supported && !clone_disabled) {
        if (op_action == Member_op_action::ADD_INSTANCE) {
          prompt = Clone_incremental_abort;
        } else {
          prompt = Clone_abort;
        }
      } else {
        if (op_action == Member_op_action::ADD_INSTANCE) {
          prompt = Incremental_abort;
        } else {
          console->print_error(target_clone_or_provisioned_rejoin);

          if (!clone_supported) {
            console->print_info(clone_available_since);
          }

          throw shcore::Exception("Instance provisioning required",
                                  SHERR_DBA_DATA_RECOVERY_NOT_POSSIBLE);
        }
      }
    }
  } else {  // recovery not possible, can only do clone
    if (op_action == Member_op_action::ADD_INSTANCE) {
      if (clone_supported && !clone_disabled && interactive) {
        prompt = Clone_abort;
      } else {
        console->print_error(target_clone_or_provisioned_add);

        if (!clone_supported) {
          console->print_info(clone_available_since);
        }

        throw shcore::Exception("Instance provisioning required",
                                SHERR_DBA_DATA_RECOVERY_NOT_POSSIBLE);
      }
    } else if (op_action == Member_op_action::REJOIN_INSTANCE) {
      if (clone_supported && !clone_disabled) {
        if (!gtid_set_diverged) {
          prompt = Clone_abort;
        } else {
          console->print_error(target_clone_or_provisioned_rejoin);

          throw shcore::Exception("Instance provisioning required",
                                  SHERR_DBA_DATA_RECOVERY_NOT_POSSIBLE);
        }
      } else {
        console->print_error(target_clone_or_provisioned_rejoin);

        if (!clone_supported) {
          console->print_info(clone_available_since);
        }

        throw shcore::Exception("Instance provisioning required",
                                SHERR_DBA_DATA_RECOVERY_NOT_POSSIBLE);
      }
    }
  }

  if (prompt != None) {
    if (!interactive) {
      if (clone_supported && !clone_disabled) {
        throw shcore::Exception::runtime_error(
            "'recoveryMethod' option must be set to 'clone' or 'incremental'");
      } else {
        throw shcore::Exception::runtime_error(
            "'recoveryMethod' option must be set to 'incremental'");
      }
    }
  }

  return prompt;
}
}  // namespace

void check_gtid_consistency_and_recoverability(
    Cluster_type cluster_type, mysqlshdk::mysql::IInstance *donor_instance,
    mysqlshdk::mysql::IInstance *target_instance,
    const std::function<bool(mysqlshdk::mysql::IInstance *)> &check_recoverable,
    Member_recovery_method recovery_method, bool gtid_set_is_complete,
    bool *out_recovery_possible, bool *out_recovery_safe,
    bool *gtid_set_diverged) {
  assert(out_recovery_possible);
  assert(out_recovery_safe);
  assert(gtid_set_diverged);

  auto console = current_console();

  std::string errant_gtid_set, msg;
  *gtid_set_diverged = false;

  // Get the gtid state in regards to the cluster_session
  mysqlshdk::mysql::Replica_gtid_state state =
      mysqlshdk::mysql::check_replica_gtid_state(
          *donor_instance, *target_instance, nullptr, &errant_gtid_set);

  switch (state) {
    case mysqlshdk::mysql::Replica_gtid_state::NEW:
      console->println();
      if (gtid_set_is_complete) {
        msg = "The target instance '" + target_instance->descr() +
              "' has not been pre-provisioned (GTID set is empty), but the " +
              thing(cluster_type) + " was configured to assume that ";

        if (cluster_type == Cluster_type::GROUP_REPLICATION) {
          msg +=
              "incremental state recovery can correctly provision it in this "
              "case.";
        } else {
          msg +=
              "replication can completely recover the state of new instances.";
        }

        console->print_note(msg);

        *out_recovery_possible = true;
        *out_recovery_safe = true;
      } else {
        msg = "The target instance '" + target_instance->descr() +
              "' has not been pre-provisioned (GTID set is empty). The Shell "
              "is unable to decide whether ";

        if (cluster_type == Cluster_type::GROUP_REPLICATION) {
          msg += "incremental state recovery can correctly provision it.";
        } else {
          msg += "replication can completely recover its state.";
        }

        console->print_note(msg);

        *out_recovery_possible = true;
        *out_recovery_safe = false;
      }

      msg_recovery_possible_and_safe(cluster_type, recovery_method,
                                     target_instance);

      console->print_info();
      break;

    case mysqlshdk::mysql::Replica_gtid_state::DIVERGED:
      console->println();
      console->print_warning(
          "A GTID set check of the MySQL instance at '" +
          target_instance->descr() +
          "' determined that it contains transactions that do not originate "
          "from the " +
          thing(cluster_type) +
          ", which must be discarded before it can join the " +
          thing(cluster_type) + ".");

      console->print_info();
      console->print_info(target_instance->descr() +
                          " has the following errant GTIDs that do not exist "
                          "in the " +
                          thing(cluster_type) + ":\n" + errant_gtid_set);
      console->print_info();

      console->print_warning(
          "Discarding these extra GTID events can either be done manually "
          "or by completely overwriting the state of " +
          target_instance->descr() +
          " with a physical snapshot from an existing " + thing(cluster_type) +
          " member. To use this method by default, set the 'recoveryMethod' "
          "option to 'clone'.\n\n"
          "Having extra GTID events is not expected, and it is recommended to "
          "investigate this further and ensure that the data can be removed "
          "prior to choosing the clone recovery method.");

      *out_recovery_possible = false;
      *out_recovery_safe = false;
      *gtid_set_diverged = true;
      break;

    case mysqlshdk::mysql::Replica_gtid_state::IRRECOVERABLE:
      if (!check_recoverable(target_instance)) {
        console->print_note("A GTID set check of the MySQL instance at '" +
                            target_instance->descr() +
                            "' determined that it is missing transactions that "
                            "were purged from all " +
                            thing(cluster_type) + " members.");

        *out_recovery_possible = false;

        // Recovery should be safe, regardless if based on incremental or clone
        // recovery as long as the target instance does not have an empty
        // GTID-set. In that case, we do not know the instance's state so we
        // cannot assume recovery is safe. BUG#30884590: ADDING AN INSTANCE WITH
        // COMPATIBLE GTID SET SHOULDN'T PROMPT FOR CLONE
        if (out_recovery_safe) {
          auto slave_gtid_set = get_executed_gtid_set(*target_instance);
          if (slave_gtid_set.empty()) {
            msg = "The target instance '" + target_instance->descr() +
                  "' has not been pre-provisioned (GTID set is empty). The "
                  "Shell is unable to decide whether clone based recovery is "
                  "safe to use.";

            console->print_note(msg);

            *out_recovery_safe = false;

            msg_recovery_possible_and_safe(cluster_type, recovery_method,
                                           target_instance, true);

            console->print_info();
          } else {
            *out_recovery_safe = true;
          }
        }
      } else {
        *out_recovery_possible = true;
        *out_recovery_safe = true;
      }
      break;

    case mysqlshdk::mysql::Replica_gtid_state::RECOVERABLE:
    case mysqlshdk::mysql::Replica_gtid_state::IDENTICAL:
      // This is the only case where it's safe to assume that the instance
      // can be safely recovered through binlog.
      *out_recovery_possible = true;
      *out_recovery_safe = true;

      // TODO(alfredo) - this doesn't seem necessary?
      msg_recovery_possible_and_safe(cluster_type, recovery_method,
                                     target_instance);

      console->print_info();
      break;

    default:
      throw std::logic_error("Internal error");
  }
}

Member_recovery_method validate_instance_recovery(
    Cluster_type cluster_type, Member_op_action op_action,
    mysqlshdk::mysql::IInstance *donor_instance,
    mysqlshdk::mysql::IInstance *target_instance,
    const std::function<bool(mysqlshdk::mysql::IInstance *)> &check_recoverable,
    Member_recovery_method opt_recovery_method, bool gtid_set_is_complete,
    bool interactive, bool clone_disabled) {
  auto console = mysqlsh::current_console();

  Member_recovery_method recovery_method = Member_recovery_method::INCREMENTAL;

  bool recovery_possible;
  bool recovery_safe;
  bool gtid_set_diverged;

  bool clone_supported = mysqlshdk::mysql::is_clone_available(*target_instance);

  check_gtid_consistency_and_recoverability(
      cluster_type, donor_instance, target_instance, check_recoverable,
      opt_recovery_method, gtid_set_is_complete, &recovery_possible,
      &recovery_safe, &gtid_set_diverged);

  if (opt_recovery_method == Member_recovery_method::CLONE) {
    validate_clone_recovery(clone_disabled);

    console->print_info(
        "Clone based recovery selected through the recoveryMethod option");
    console->print_info();

    recovery_method = Member_recovery_method::CLONE;

  } else if (opt_recovery_method == Member_recovery_method::INCREMENTAL) {
    validate_incremental_recovery(op_action, recovery_possible, recovery_safe);

    console->print_info(
        "Incremental state recovery selected through the "
        "recoveryMethod option");
    console->print_info();

    recovery_method = Member_recovery_method::INCREMENTAL;
  } else {
    // When recovery is safe we do not need to prompt. If possible,
    // incremental recovery should be used, otherwise clone. BUG#30884590:
    // ADDING AN INSTANCE WITH COMPATIBLE GTID SET SHOULDN'T PROMPT FOR
    // CLONE
    if (recovery_safe) {
      if (recovery_possible) {
        mysqlshdk::utils::nullable<int64_t> current_threshold;

        // Even though we detected that incremental distributed recovery is
        // safely usable, it might not be used if
        // group_replication_clone_threshold was set to 1 - in InnoDB
        // cluster only
        if (cluster_type == Cluster_type::GROUP_REPLICATION) {
          current_threshold = target_instance->get_sysvar_int(
              "group_replication_clone_threshold");
        }

        if ((cluster_type == Cluster_type::GROUP_REPLICATION) &&
            (!current_threshold.is_null() && *current_threshold != LLONG_MAX)) {
          console->print_note(
              "Incremental state recovery was determined to be "
              "safely usable, however, group_replication_clone_threshold has "
              "been set to " +
              std::to_string(*current_threshold) +
              ", which may cause Group Replication to use clone based "
              "recovery.");
        } else {
          console->print_info(
              "Incremental state recovery was selected because it seems to be "
              "safely usable.");
          console->print_info();
          recovery_method = Member_recovery_method::INCREMENTAL;
        }
      } else {
        if (!clone_disabled && clone_supported) {
          console->print_info(
              "Clone based recovery was selected because it seems to be safely "
              "usable.");
          console->print_info();
          recovery_method = Member_recovery_method::CLONE;
        }
      }
    } else {
      Prompt_type prompt = validate_auto_recovery(
          cluster_type, op_action, recovery_possible, recovery_safe,
          clone_supported, gtid_set_diverged, interactive, clone_disabled);

      console->print_info();

      switch (prompt) {
        case Clone_incremental_abort:
          switch (console->confirm("Please select a recovery method",
                                   mysqlsh::Prompt_answer::YES, "&Clone",
                                   "&Incremental recovery", "&Abort")) {
            case mysqlsh::Prompt_answer::YES:
              recovery_method = Member_recovery_method::CLONE;
              break;
            case mysqlsh::Prompt_answer::NO:
              recovery_method = Member_recovery_method::INCREMENTAL;
              break;
            case mysqlsh::Prompt_answer::ALT:
            case mysqlsh::Prompt_answer::NONE:
              throw shcore::cancelled("Cancelled");
          }
          break;

        case Clone_abort:
          switch (console->confirm("Please select a recovery method",
                                   mysqlsh::Prompt_answer::NO, "&Clone",
                                   "&Abort")) {
            case mysqlsh::Prompt_answer::YES:
              recovery_method = Member_recovery_method::CLONE;
              break;
            default:
              throw shcore::cancelled("Cancelled");
          }
          break;

        case Incremental_abort:
          switch (console->confirm("Please select a recovery method",
                                   mysqlsh::Prompt_answer::YES,
                                   "&Incremental recovery", "&Abort")) {
            case mysqlsh::Prompt_answer::YES:
              recovery_method = Member_recovery_method::INCREMENTAL;
              break;
            default:
              throw shcore::cancelled("Cancelled");
          }
          break;

        case None:
          break;
      }
    }
  }

  return recovery_method;
}

}  // namespace dba
}  // namespace mysqlsh

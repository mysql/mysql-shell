/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include <algorithm>

#include "modules/adminapi/common/clone_progress.h"
#include "mysqlshdk/include/shellcore/console.h"

namespace mysqlsh {
namespace dba {

Clone_progress::Clone_progress(Recovery_progress_style style)
    : m_current_stage(-1), m_style(style) {}

Clone_progress::~Clone_progress() {
  if (m_progress) {
    m_progress->end();
    m_progress.reset();
  }
}

void Clone_progress::on_restart() {
  if (m_progress) {
    m_progress->end();
    m_progress.reset();

    m_current_stage = 4;
  }
}

void Clone_progress::on_error() {
  if (m_progress) {
    m_progress->end();
    m_progress.reset();
  }
}

void Clone_progress::update(const mysqlshdk::mysql::Clone_status &status) {
  int current_stage = status.current_stage();

  if (current_stage >= 0 && current_stage < 7) {
    const mysqlshdk::mysql::Clone_status::Stage_info &stage =
        status.stages[current_stage];
    log_debug2(
        "Clone state=%s, elapsed=%s, errno=%i, error=%s, stage=(%i, %s, "
        "state=%s, elapsed=%s, begin_time=%s)",
        status.state.c_str(), std::to_string(status.seconds_elapsed).c_str(),
        status.error_n, status.error.c_str(), current_stage,
        stage.stage.c_str(), stage.state.c_str(),
        std::to_string(stage.seconds_elapsed).c_str(),
        status.begin_time.c_str());
  }

  if (m_style != Recovery_progress_style::NOWAIT &&
      m_style != Recovery_progress_style::NOINFO) {
    if (current_stage < 1) {
      // before data transfer
      update_stage(status, 0, 0);
    } else if (current_stage < 4) {
      // data transfer
      update_transfer(status);
    } else {
      if (m_current_stage < 4) {
        update_transfer(status);  // flush end of data transfer part
      }

      // restart and the rest
      update_stage(status, 4, 6);
    }
  }

  if (status.state == mysqlshdk::mysql::k_CLONE_STATE_FAILED) {
    auto console = mysqlsh::current_console();

    console->print_error("The clone process has failed: " + status.error +
                         " (" + std::to_string(status.error_n) + ")");
    throw std::runtime_error(status.error);
  }
}

void Clone_progress::update_stage(const mysqlshdk::mysql::Clone_status &status,
                                  int first_stage, int last_stage) {
  // stage can be in 3 general states:
  // - Not Started
  // - In Progress
  // - Completed/Failed
  //
  // In fancy progress mode, we start showing the stage once the previous
  // stage switches to Completed/Failed.
  // In plain mode, we only show the stage when Completed/Failed.
  auto console = mysqlsh::current_console();

  int current_stage = status.current_stage();

  if (m_style == Recovery_progress_style::PROGRESSBAR) {
    // dynamic text
    if (current_stage > m_current_stage) {
      // clear the last seen step
      if (m_current_stage >= 0 && m_spinner) {
        m_spinner->done(status.stages[m_current_stage].state);
        m_spinner.reset();
      }

      // add items between the last seen step and the new step
      for (int i = m_current_stage + 1;
           i < std::min(last_stage + 1, current_stage); i++) {
        if (status.stages[i].state == mysqlshdk::mysql::k_CLONE_STATE_SUCCESS ||
            status.stages[i].state == mysqlshdk::mysql::k_CLONE_STATE_FAILED)
          console->print_info("** Stage " + status.stages[i].stage + ": " +
                              status.stages[i].state);
      }

      if (current_stage <= last_stage && current_stage >= first_stage) {
        // add current step
        m_current_stage = current_stage;
        if (m_current_stage < static_cast<int>(status.stages.size())) {
          m_spinner.reset(new mysqlshdk::textui::Spinny_stick(
              "** Stage " + status.stages[current_stage].stage + ":"));
          m_spinner->update();
        }
      }
    } else if (m_current_stage == current_stage) {
      if (m_spinner) m_spinner->update();
    }
  } else if (m_style == Recovery_progress_style::TEXTUAL) {
    // static text
    for (int i = m_current_stage + 1;
         i < std::min(last_stage + 1, current_stage); i++) {
      console->print_info("** Stage " + status.stages[i].stage + ": " +
                          status.stages[i].state);
      m_current_stage = i;
    }
  }
}

void Clone_progress::update_transfer(
    const mysqlshdk::mysql::Clone_status &status) {
  int current_stage = status.current_stage();

  if (m_style == Recovery_progress_style::TEXTUAL) {
    update_stage(status, 1, 3);
  } else {
    // fancy progress
    if (!m_progress) {
      // flush previous steps, if needed
      update_stage(status, 0, 0);

      m_progress.reset(new mysqlshdk::textui::Progress_vt100(3));
      m_progress->set_label("** Clone Transfer");
      m_progress->set_total(-1);  // disable main progressbar
      m_progress->set_secondary_label(
          0, mysqlshdk::mysql::k_CLONE_STAGE_FILE_COPY);
      m_progress->set_secondary_right_label(
          0, mysqlshdk::mysql::k_CLONE_STATE_NONE);
      m_progress->set_secondary_label(
          1, mysqlshdk::mysql::k_CLONE_STAGE_PAGE_COPY);
      m_progress->set_secondary_right_label(
          1, mysqlshdk::mysql::k_CLONE_STATE_NONE);
      m_progress->set_secondary_label(
          2, mysqlshdk::mysql::k_CLONE_STAGE_REDO_COPY);
      m_progress->set_secondary_right_label(
          2, mysqlshdk::mysql::k_CLONE_STATE_NONE);

      m_progress->start();
    }

    // bool success = true;
    for (int i = 1; i <= 3; i++) {
      const auto &stage = status.stages[i];

      if (stage.state == mysqlshdk::mysql::k_CLONE_STATE_SUCCESS) {
        m_progress->set_secondary_total(i - 1, 1);
        m_progress->set_secondary_current(i - 1, 1);
      } else {
        m_progress->set_secondary_total(i - 1, stage.work_estimated);
        m_progress->set_secondary_current(i - 1, stage.work_completed);
      }

      m_progress->set_secondary_right_label(i - 1, stage.state);

      // if (stage.state != mysqlshdk::mysql::k_CLONE_STATE_SUCCESS)
      //  success = false;
    }

    m_progress->update();
    if (current_stage > 3) {
      m_progress->end();
      m_progress.reset();
    }
    m_current_stage = current_stage;
  }
}

}  // namespace dba
}  // namespace mysqlsh

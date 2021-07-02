/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_PROGRESS_THREAD_H_
#define MODULES_UTIL_DUMP_PROGRESS_THREAD_H_

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/textui/text_progress.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"

namespace mysqlsh {
namespace dump {

/**
 * Displays progress of various stages of a complete process using a separate
 * thread. The current_console() object is set up, so that any operation which
 * writes to the console does not interfere with the progress reporting.
 */
class Progress_thread final {
 public:
  class Stage;

  /**
   * Represents a duration in time.
   */
  class Duration final {
   public:
    Duration(const Duration &) = delete;
    Duration(Duration &&) = delete;

    Duration &operator=(const Duration &) = delete;
    Duration &operator=(Duration &&) = delete;

    ~Duration() = default;

    /**
     * Returns date and time when time measurement has started, formatted:
     *   %Y-%m-%d %T.
     *
     * @returns formatted start time
     */
    const std::string &started_at() const;

    /**
     * Returns date and time when time measurement has finished, formatted:
     *   %Y-%m-%d %T.
     *
     * @returns formatted finish time
     *
     * @throws std::logic_error if time measurement is not finished yet
     */
    const std::string &finished_at() const;

    /**
     * Returns duration in seconds.
     *
     * @returns duration in seconds
     *
     * @throws std::logic_error if time measurement is not finished yet
     */
    double seconds() const;

    /**
     * Formats duration in seconds to: HH:MM:SSs.
     *
     * @returns formatted duration
     *
     * @throws std::logic_error if time measurement is not finished yet
     */
    std::string to_string() const;

   private:
    friend class Progress_thread;
    friend class Progress_thread::Stage;

    Duration() = default;

    void start();

    void finish();

    void validate() const;

    mysqlshdk::utils::Profile_timer m_timer;
    std::string m_start;
    std::string m_finish;
  };

  /**
   * Represents a stage of execution.
   */
  class Stage {
   public:
    Stage(const Stage &) = delete;
    Stage(Stage &&) = delete;

    Stage &operator=(const Stage &) = delete;
    Stage &operator=(Stage &&) = delete;

    virtual ~Stage() = default;

    /**
     * Finishes execution of this stage.
     *
     * @param wait If true, it will wait for the display loop to end.
     */
    void finish(bool wait = true);

    /**
     * Provides duration of this stage.
     *
     * @return duration of this stage
     */
    const Duration &duration() const { return m_duration; }

    /**
     * Provides description of this stage.
     *
     * @return description of this stage
     */
    const std::string &description() const { return m_description; }

   protected:
    /**
     * Initializes a stage with the given description.
     *
     * @param description The description of a stage.
     * @param show_progress Whether the progress should be displayed.
     */
    Stage(const std::string &description, bool show_progress);

    /**
     * Write the progress to the console.
     */
    virtual void draw() = 0;

    /**
     * Called when a stage begins to be displayed.
     */
    virtual void on_display_started();

    /**
     * Called when a stage finishes to be displayed.
     */
    virtual void on_display_done();

    /**
     * Called when displaying a stage has been terminated, and it is i.e. unsafe
     * to call configured callbacks.
     */
    virtual void on_display_terminated();

    /**
     * Called when the stage should update its progress.
     */
    virtual void on_update();

   private:
    friend class Progress_thread;

    void start();

    void display();

    void wait_for_finish();

    void wait_for_display_done();

    void terminate();

    // information about the stage
    Duration m_duration;
    std::string m_description;
    bool m_show_progress;

    // display-related variables
    std::mutex m_finished_mutex;
    std::condition_variable m_finished_cv;
    volatile bool m_finished = false;

    std::mutex m_display_done_mutex;
    std::condition_variable m_display_done_cv;
    volatile bool m_display_done = false;

    volatile bool m_terminated = false;
  };

  /**
   * Configuration of a stage which displays a spinner along with numeric
   * progress: current / total.
   */
  struct Progress_config {
    /**
     * The current number of items, required.
     */
    std::function<uint64_t()> current;
    /**
     * The total number of items, required.
     */
    std::function<uint64_t()> total;
    /**
     * Whether the total number of items is already known, optional. Used i.e.
     * when progress is displayed while the total number of items is still being
     * computed. If not given, the total is assumed to be known.
     *
     * If this function returns false, the total number if prefixed with a
     * tilde.
     */
    std::function<bool()> is_total_known;
  };

  /**
   * Configuration of a stage which displays a throughput information.
   */
  struct Throughput_config {
    /**
     * The full name of the items being processed.
     */
    const char *items_full = "bytes";
    /**
     * The abbreviated name of the items being processed.
     */
    const char *items_abbrev = "B";
    /**
     * The singular name of the items being processed.
     */
    const char *item_singular = "B";
    /**
     * The plural name of the items being processed.
     */
    const char *item_plural = "B";
    /**
     * Whether there should be a space between the unit and the item name.
     */
    bool space_before_item = true;
    /**
     * Whether the total value is approximated. If true, total is prefixed with
     * a tilde.
     */
    bool total_is_approx = false;
    /**
     * The initial number of items, optional. If not given, it is assumed to be
     * equal to 0.
     */
    std::function<uint64_t()> initial;
    /**
     * The current number of items, required.
     */
    std::function<uint64_t()> current;
    /**
     * The total number of items, required.
     */
    std::function<uint64_t()> total;
    /**
     * The text to be displayed in the left label, optional.
     */
    std::function<std::string()> left_label;
    /**
     * The text to be displayed in the right label, optional.
     */
    std::function<std::string()> right_label;
    /**
     * Called when display is started, optional.
     */
    std::function<void()> on_display_started;
  };

  Progress_thread() = delete;

  /**
   * Initialized the thread which is going to display information about stages
   * and their progress.
   *
   * @param description The description of the operation.
   * @param show_progress If false, the progress is not going to be displayed,
   *        and will be replaced with static messages printed to the console
   *        when a stage starts and finishes.
   */
  Progress_thread(const std::string &description, bool show_progress);

  Progress_thread(const Progress_thread &) = delete;
  Progress_thread(Progress_thread &&) = delete;

  Progress_thread &operator=(const Progress_thread &) = delete;
  Progress_thread &operator=(Progress_thread &&) = delete;

  ~Progress_thread();

  /**
   * Starts handling of the registered stages. Each of them is going to be
   * displayed one by one, after the previous one finishes execution. If not
   * called explicitly, will be implicitly called by start_stage().
   *
   * Duration measurement starts after this call.
   */
  void start();

  /**
   * Starts a stage which displays a spinner. Needs to be manually finished.
   *
   * @param description The description of the new stage.
   *
   * @returns stage which was created.
   */
  Stage *start_stage(const std::string &description);

  /**
   * Starts a stage which displays a spinner and a numeric progress. Stage will
   * be automatically finished.
   *
   * @param description The description of the new stage.
   * @param config The configuration of the new stage.
   *
   * @returns stage which was created.
   */
  Stage *start_stage(const std::string &description, Progress_config &&config);

  /**
   * Starts a stage which displays a throughput information. Needs to be
   * manually finished.
   *
   * @param description The description of the new stage.
   * @param config The configuration of the new stage.
   *
   * @returns stage which was created.
   */
  Stage *start_stage(const std::string &description,
                     Throughput_config &&config);

  /**
   * Finishes any pending stages and waits for their completion.
   *
   * @throws any exception reported by the stages
   */
  void finish();

  /**
   * Interrupts execution of all stages.
   */
  void terminate();

  /**
   * Provides duration of all the registered stages.
   *
   * @return duration of all stages
   */
  const Duration &duration() const { return m_total_duration; }

 private:
  template <class T, class... Args>
  Stage *start_stage(const std::string &description, Args &&... args);

  void shutdown();

  void emergency_shutdown();

  void wait_for_thread();

  void rethrow();

  void kill_thread();

  // configuration
  std::string m_description;
  bool m_show_progress = false;

  // console
  std::unique_ptr<Scoped_console> m_console;

  // progress reporting
  Duration m_total_duration;
  std::vector<std::unique_ptr<Stage>> m_stages;
  shcore::Synchronized_queue<Stage *> m_schedule;
  std::unique_ptr<std::thread> m_progress_thread;
  std::exception_ptr m_exception;
  volatile bool m_interrupt = false;

  // animated progress
  std::unique_ptr<mysqlshdk::textui::Base_progress> m_progress;
  std::recursive_mutex m_progress_mutex;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_PROGRESS_THREAD_H_

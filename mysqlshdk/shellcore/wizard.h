/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_SHELLCORE_WIZARD_H_
#define MYSQLSHDK_SHELLCORE_WIZARD_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/include/scripting/lang_base.h"
#include "mysqlshdk/include/shellcore/console.h"

namespace shcore {
namespace wizard {

constexpr const char K_DONE[] = "K_DONE";
constexpr const char K_NEXT[] = "K_NEXT";
constexpr const char K_YES[] = "K_YES";
constexpr const char K_NO[] = "K_NO";

enum class Prompt_type { PROMPT, CONFIRM, SELECTION };

using Prompt_answer = mysqlsh::Prompt_answer;

/**
 * This class provides a standard framework to interactively request
 * information from the user.
 *
 * It is possible to register a series of steps including:
 * - Open questions.
 * - Confirmation questions.
 * - Selection from a list of options.
 *
 * It is also possible to define different paths to be followed
 * based on the user answers through links.
 *
 * Other supported features include:
 * - Validation callbacks per each step.
 * - Post processing callbacks per step.
 * - Dynamically change step linkage to update the sequence of steps on the go.
 *
 * Each registered step is associated to an ID and the information
 * provided by the user for that step will be stored internally in a
 * dictionary in the form of: id=answer.
 */
class Wizard {
 public:
  using Step_leave_callback =
      std::function<void(const std::string &value, Wizard *wizard)>;

 private:
  /**
   * Base class holding the elements associated to a step.
   */
  struct Wizard_step {
    explicit Wizard_step(const Wizard_step &other) = default;

    Wizard_step(const std::string &i, Prompt_type t) : id(i), type(t) {}

    virtual ~Wizard_step() {}

    bool has_link(const std::string &link) {
      return links.find(link) != links.end();
    }

    void on_leave(Wizard *w) {
      if (on_leave_cb && !w->is_cancelled()) on_leave_cb((*w)[id], w);
    }

    void ensure_link(const std::string &link) {
      if (!has_link(link))
        throw std::logic_error("Missing link '" + link + "' for step '" + id +
                               "'");
    }

    virtual void verify() = 0;
    virtual std::string execute() = 0;

    std::string id;
    Prompt_type type;
    std::string text;
    std::vector<std::string> details;
    mysqlsh::IConsole::Validator validate_cb = nullptr;
    Step_leave_callback on_leave_cb = nullptr;
    std::map<std::string, std::string> links;
  };

  /**
   * Template class that will provide the correct method definitions
   * to define the different elements of a Step.
   *
   * This is required so the classes for the different step types can
   * define it's elements by chaining calls to these methods.
   */
  template <typename T>
  struct Wizard_common : public Wizard_step {
    Wizard_common(const Wizard_common &other) = default;
    Wizard_common(const std::string &i, Prompt_type t) : Wizard_step(i, t) {}
    virtual ~Wizard_common() {}

    T &prompt(const std::string &p) {
      text = p;
      return *reinterpret_cast<T *>(this);
    }

    T &description(const std::vector<std::string> &d) {
      details = d;
      return *reinterpret_cast<T *>(this);
    }

    T &validator(mysqlsh::IConsole::Validator &&v) {
      validate_cb = v;
      return *reinterpret_cast<T *>(this);
    }

    T &on_leave(Step_leave_callback &&cb) {
      on_leave_cb = cb;
      return *reinterpret_cast<T *>(this);
    }

    /**
     * Links the next step to be executed after the current step
     * if the user response equals the indicated trigger
     *
     * @param trigger:
     * - K_NEXT: to define the next step on a "Next" action
     * - K_YES: to define the next step on a "Yes" answer
     * - K_NO: to define the next step on a "No" answer
     * - Any Other String: to define the next step i.e. on specific user
     *   answers.
     *
     * @param target the id of the next step or K_DONE to indicate the
     * wizard ends.
     */
    T &link(const std::string &trigger, const std::string &target) {
      if (!has_link(trigger))
        links[trigger] = target;
      else
        throw std::logic_error("Link '" + trigger + "' for step '" + id +
                               "' is already defined.");

      return *reinterpret_cast<T *>(this);
    }

    T &unlink(const std::string &trigger) {
      if (has_link(trigger))
        links.erase(trigger);
      else
        throw std::logic_error("Link '" + trigger + "' for step '" + id +
                               "' does not exist.");

      return *reinterpret_cast<T *>(this);
    }
  };

  /**
   * Defines an open question prompt.
   *
   * A link on K_NEXT must be defined for this step.
   */
  struct Wizard_prompt : public Wizard_common<Wizard_prompt> {
    explicit Wizard_prompt(const std::string &id_)
        : Wizard_common(id_, Prompt_type::PROMPT) {}

    explicit Wizard_prompt(const Wizard_prompt &other) = default;

    virtual ~Wizard_prompt() {}

    Wizard_prompt &default_answer(const std::string &d) {
      answer = d;
      return *this;
    }

    Wizard_prompt &hide_input(bool h) {
      password = h;
      return *this;
    }

    void verify() override;
    std::string execute() override;

    bool password = false;
    std::string answer;
  };

  /**
   * Defines a confirmation prompt (Yes/No) question.
   *
   * The K_YES and K_NO links must be defined for this prompt
   * or K_NEXT in case one of them is missing.
   */
  struct Wizard_confirm : public Wizard_common<Wizard_confirm> {
    explicit Wizard_confirm(const std::string &id_)
        : Wizard_common(id_, Prompt_type::CONFIRM) {}
    explicit Wizard_confirm(const Wizard_confirm &other) = default;
    virtual ~Wizard_confirm() {}

    Wizard_confirm &default_answer(Prompt_answer d) {
      def_answer = d;
      return *this;
    }

    void verify() override;
    std::string execute() override;

    Prompt_answer def_answer = Prompt_answer::NO;
  };

  /**
   * Defines a selection prompt.
   *
   * Links for every possible answer must be defined unless the
   * link for K_NEXT is defined.
   */
  struct Wizard_select : public Wizard_common<Wizard_select> {
    explicit Wizard_select(const std::string &id_)
        : Wizard_common(id_, Prompt_type::SELECTION) {}
    explicit Wizard_select(const Wizard_select &other) = default;
    virtual ~Wizard_select() {}

    Wizard_select &default_option(size_t o) {
      def_option = o;
      return *this;
    }

    Wizard_select &items(const std::vector<std::string> &i) {
      item_list = i;
      return *this;
    }

    Wizard_select &allow_custom(bool c) {
      custom = c;
      return *this;
    }

    void verify() override;
    std::string execute() override;

    size_t def_option = 0;
    std::vector<std::string> item_list;
    bool custom = false;
  };

 public:
  Wizard() : m_cancelled(false) {}

  /**
   * This function handles the wizard execution, after
   * completion, if the wizard is not cancelled, all the
   * gathered data will be available in the wizard.
   */
  bool execute(const std::string &id);

  // Step registering functions.
  Wizard_prompt &add_prompt(const std::string &id);
  Wizard_confirm &add_confirm(const std::string &id);
  Wizard_select &add_select(const std::string &id);

  bool has_step(const std::string &id);

  /**
   * Dynamically changes the links for a wizard step:
   *
   * @param step: the step id for which a link will be updated
   * @param trigger: the link that will be updated
   * @param target: the id of the new target step or K_DONE
   */
  void relink(const std::string &step, const std::string &trigger,
              const std::string &target);

  /**
   * Verifies if the wizard has a specific data element.
   */
  bool has(const std::string &key) { return m_data.find(key) != m_data.end(); }

  /**
   * Removes a data element from the wizard if exists
   */
  bool remove(const std::string &key) {
    if (m_data.find(key) != m_data.end()) {
      m_data.erase(key);
      return true;
    }

    return false;
  }

  /**
   * Sets a new data element in the wizard, note that there's no
   * restriction that the key should be a valid step, that's on purpose
   */
  inline void set(const std::string &key, const std::string &value) {
    m_data[key] = value;
  }

  /**
   * Retrieves the value of a data element from the wizard
   */
  inline std::string get(const std::string &key) { return m_data[key]; }

  std::string &operator[](const std::string &key) { return m_data[key]; }

  inline void cancel() { m_cancelled = true; }
  inline bool is_cancelled() const { return m_cancelled; }
  void reset();

 protected:
  std::map<std::string, std::unique_ptr<Wizard_step>> m_steps;
  std::map<std::string, std::string> m_data;

  bool m_cancelled;

  // Auxiliary functions for consistency check
  void verify(const std::string &id);
  void verify(const std::string &id, std::vector<std::string> *data);

  // Auxiliary function for step registration
  Wizard_step *add_step(const std::string &id, Prompt_type type);

#ifdef FRIEND_TEST
  FRIEND_TEST(Wizard_test, step_addition);
  FRIEND_TEST(Wizard_test, prompt_step);
  FRIEND_TEST(Wizard_test, confirm_step);
  FRIEND_TEST(Wizard_test, select_step);
#endif
};
}  // namespace wizard

}  // namespace shcore

#endif  // MYSQLSHDK_SHELLCORE_WIZARD_H_

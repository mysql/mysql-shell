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

#include "mysqlshdk/libs/ssh/ssh_session.h"
#include <fcntl.h>
#include <functional>
#include <sstream>
#include <vector>
#include "mysqlshdk/include/scripting/shexcept.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/shellcore/credential_manager.h"
#ifdef _MSC_VER
#include "Shlobj.h"
#endif  // _MSC_VER

namespace mysqlshdk {
namespace ssh {
namespace {
int libssh_auth_callback(const char *prompt, char *buf, size_t len, int echo,
                         int UNUSED(verify), void *userdata) {
  std::string return_value;
  if (echo == 1) {
    if (mysqlsh::current_console()->prompt(prompt, &return_value) !=
        shcore::Prompt_result::Ok)
      return -1;
  } else {
    auto session = static_cast<Ssh_session *>(userdata);
    auto &options = session->get_options();

    if (strcmp(prompt, "Passphrase for private key:") == 0 &&
        options.has_keyfile_password()) {
      return_value = options.get_key_file_password();
    } else if (mysqlsh::current_console()->prompt_password(
                   prompt, &return_value) != shcore::Prompt_result::Ok) {
      return -1;
    }
  }
  strncpy(buf, return_value.data(),
          return_value.size() > len ? len : return_value.size());

  shcore::clear_buffer(&return_value);
  return 0;
}
}  // namespace

Ssh_session::Ssh_session()
    : m_session{std::make_unique<::ssh::Session>()},
      m_is_connected(false),
      m_interactive(true) {
  init_libssh();

  memset(&m_ssh_callbacks, 0, sizeof(m_ssh_callbacks));
  m_ssh_callbacks.userdata = this;
  m_ssh_callbacks.auth_function = libssh_auth_callback;
  ssh_callbacks_init(&m_ssh_callbacks);
  auto rc = ssh_set_callbacks(m_session->getCSession(), &m_ssh_callbacks);

  if (rc == SSH_ERROR) throw std::runtime_error(m_session->getError());
}

Ssh_session::~Ssh_session() {}

std::tuple<Ssh_return_type, std::string> Ssh_session::connect(
    const Ssh_connection_options &config) {
  if (is_connected()) {
    throw std::logic_error(
        "Unable to connect already connected SSHSession, please disconnect "
        "first.");
  }

  // auto lock = lock_session();
  m_options = config;
  m_interactive = m_options.interactive();
  // We need to set the host before reading the config, otherwise we will get
  // error. This will be of course overridden by optionsParseconfig
  try {
    m_session->setOption(SSH_OPTIONS_HOST, m_options.get_host().c_str());
  } catch (::ssh::SshException &exc) {
    log_error(
        "SSH: session: Error setting remote host in ssh session option: %s",
        exc.getError().c_str());
    return std::make_tuple(Ssh_return_type::INVALID_AUTH_DATA, exc.getError());
  }

  // The default timeout of 10 seconds is set here, if different value is
  // needed, it should be set in the config file
  try {
    m_session->setOption(SSH_OPTIONS_TIMEOUT,
                         static_cast<long>(m_options.get_connection_timeout()));
  } catch (::ssh::SshException &exc) {
    log_error(
        "SSH: session: Error setting connection timeout in ssh session option: "
        "%s",
        exc.getError().c_str());
    return std::make_tuple(Ssh_return_type::INVALID_AUTH_DATA, exc.getError());
  }

  // We've already loaded the config  earlier
  // but it wasn't full data, here we do this again, overwriting what we
  // already have from the user and leave the rest like ciphers etc

  try {
    m_session->optionsParseConfig(m_options.has_config_file()
                                      ? m_options.get_config_file().c_str()
                                      : nullptr);

    Ssh_config_reader reader;

    reader.read(&m_config, m_options.get_host(),
                m_options.has_config_file() ? m_options.get_config_file() : "");
  } catch (::ssh::SshException &exc) {
    if (m_options.has_config_file()) {
      log_error("SSH: session: Unable to parse config file: %s\nError was: %s ",
                m_options.get_config_file().c_str(), exc.getError().c_str());
    } else {
      log_error(
          "SSH: session: Unable to parse default config file\nError was: %s ",
          exc.getError().c_str());
    }
  }

  // The option setter does't indicate what option failed to set, so we have to
  // wrap each call individually to be able to report exactly what failed.
  if (m_options.has_user()) {
    try {
      m_session->setOption(SSH_OPTIONS_USER, m_options.get_user().c_str());
    } catch (::ssh::SshException &exc) {
      log_error(
          "SSH: session: Error setting user name in ssh session option: %s",
          exc.getError().c_str());
      return std::make_tuple(Ssh_return_type::INVALID_AUTH_DATA,
                             exc.getError());
    }
  }

  if (m_options.has_key_file()) {
    try {
      m_session->setOption(SSH_OPTIONS_IDENTITY,
                           m_options.get_key_file().c_str());
    } catch (::ssh::SshException &exc) {
      log_error(
          "SSH: session: Error setting identity file in ssh session option: %s",
          exc.getError().c_str());
      return std::make_tuple(Ssh_return_type::INVALID_AUTH_DATA,
                             exc.getError());
    }
  }

  if (m_options.has_port()) {
    try {
      m_session->setOption(SSH_OPTIONS_PORT,
                           static_cast<long>(m_options.get_port()));
    } catch (::ssh::SshException &exc) {
      log_error(
          "SSH: session: Error setting remote port in ssh session option: %s",
          exc.getError().c_str());
      return std::make_tuple(Ssh_return_type::INVALID_AUTH_DATA,
                             exc.getError());
    }
  }

  m_options.dump_config();

  try {
    m_session->connect();
  } catch (::ssh::SshException &exc) {
    log_error(
        "SSH: session: Unable to establish SSH connection: %s:%d\nError was: "
        "%s",
        m_options.get_host().c_str(), m_options.get_port(),
        exc.getError().c_str());
    return std::make_tuple(Ssh_return_type::CONNECTION_FAILURE, exc.getError());
  }

  std::string fingerprint;
  int ret_val = verify_known_host(m_options, &fingerprint);
  switch (ret_val) {
    case SSH_SERVER_FILE_NOT_FOUND:
    case SSH_SERVER_NOT_KNOWN:
      return std::make_tuple(
          ret_val == SSH_SERVER_FILE_NOT_FOUND
              ? Ssh_return_type::FINGERPRINT_UNKNOWN_AUTH_FILE_MISSING
              : Ssh_return_type::FINGERPRINT_UNKNOWN,
          fingerprint);
    case SSH_SERVER_KNOWN_CHANGED:
      return std::make_tuple(Ssh_return_type::FINGERPRINT_CHANGED, fingerprint);
    case SSH_SERVER_FOUND_OTHER:
      return std::make_tuple(Ssh_return_type::FINGERPRINT_MISMATCH,
                             fingerprint);
    default:
      break;
  }

  try {
    authenticate_user(m_options);
  } catch (Ssh_auth_exception &sxc) {
    log_error("SSH: session: User authentication failed.");
    return std::make_tuple(Ssh_return_type::INVALID_AUTH_DATA,
                           std::string(sxc.what()));
  }

  m_is_connected = true;

  m_time_created = std::chrono::system_clock::now();

  return std::make_tuple(Ssh_return_type::CONNECTED, "");
}

void Ssh_session::disconnect() {
  log_debug2("SSH: session: disconnect");

  if (m_session != nullptr) {
    if (m_is_connected) m_session->disconnect();
    m_session = std::make_unique<::ssh::Session>();
  }
  // Now we should release the lock.
  m_is_connected = false;
}

bool Ssh_session::is_connected() const {
  return m_is_connected && ssh_is_connected(m_session->getCSession());
}

const Ssh_connection_options &Ssh_session::config() const { return m_options; }

void Ssh_session::update_local_port(int port) {
  m_options.set_local_port(port);
}

bool Ssh_session::open_channel(::ssh::Channel *chann) {
  int rc = SSH_ERROR;
  std::size_t i = 0;
  while (i < m_options.get_connection_timeout()) {
    rc = ssh_channel_open_session(chann->getCChannel());
    // We can't rely on rc == 0 as there's possibility it will return 0 even
    // that the channel is closed. Because of this, we have to use isOpen()
    // and try few times
    if (rc == SSH_AGAIN || !chann->isOpen()) {
      log_debug3(
          "SSH: session: Unable to open channel, wait a moment and retry.");
      i++;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    } else if (rc == SSH_ERROR) {
      log_error("SSH: session: Unable to open channel: %s",
                ssh_get_error(chann->getCSession()));
      return false;
    } else {
      log_debug("SSH: session: Channel successfully opened");
      return true;
    }
  }
  return false;
}

void Ssh_session::clean_connect() {
  if (!ssh_is_connected(m_session->getCSession())) {
    disconnect();
    connect(m_options);
  }
}

const char *Ssh_session::get_ssh_error() {
  if (m_session) {
    return m_session->getError();
  }
  return nullptr;
}

ssh_session Ssh_session::get_csession() {
  if (m_session) {
    return m_session->getCSession();
  }
  throw std::logic_error(
      "Trying to access SSH session which should be available but it's not");
}

std::unique_ptr<::ssh::Channel> Ssh_session::create_channel() {
  return std::make_unique<::ssh::Channel>(*(m_session));
}

int Ssh_session::verify_known_host(const ssh::Ssh_connection_options &config,
                                   std::string *fingerprint) {
  std::unique_ptr<unsigned char, void (*)(unsigned char *)> hash(
      nullptr, [](unsigned char *v) {
        if (v != nullptr) ssh_clean_pubkey_hash(&v);
      });
  ssh_key srv_pub_key;
  int rc = 0;
  std::size_t hlen = 0;
  errno = 0;
  rc = ssh_get_server_publickey(m_session->getCSession(), &srv_pub_key);
  if (rc < 0)
    throw Ssh_tunnel_exception("Can't get server pubkey " + get_error());

  unsigned char *hash_ptr = nullptr;
  errno = 0;
  rc = ssh_get_publickey_hash(srv_pub_key, SSH_PUBLICKEY_HASH_SHA256, &hash_ptr,
                              &hlen);
  ssh_key_free(srv_pub_key);
  if (rc < 0)
    throw Ssh_tunnel_exception("Can't calculate pubkey hash " + get_error());

  hash.reset(hash_ptr);

  std::unique_ptr<char, void (*)(char *)> hexa(ssh_get_hexa(hash.get(), hlen),
                                               [](char *ptr) { free(ptr); });
  *fingerprint = hexa.get();
  int ret_val = m_session->isServerKnown();
  switch (ret_val) {
    case SSH_SERVER_FILE_NOT_FOUND:
    case SSH_SERVER_NOT_KNOWN: {
      if (config.get_fingerprint().empty()) {
        return ret_val;
      } else {
        if (config.get_fingerprint() == hexa.get()) {
          m_session->writeKnownhost();
          return SSH_SERVER_KNOWN_OK;
        } else {
          return ret_val;
        }
      }
    }
    case SSH_SERVER_KNOWN_OK:
    case SSH_SERVER_KNOWN_CHANGED:
    case SSH_SERVER_FOUND_OTHER:
      return ret_val;
    case SSH_SERVER_ERROR:
      throw Ssh_tunnel_exception(m_session->getError());
  }

  return SSH_SERVER_KNOWN_OK;
}

namespace {
Ssh_auth_return try_auth(const std::function<Ssh_auth_return()> &fn) {
  Ssh_auth_return ret;
  int attempt = 0;
  do {
    ret = fn();
    if (ret != Ssh_auth_return::AUTH_DENIED) break;
    attempt++;
  } while (attempt < 3);
  return ret;
}
}  // namespace

void Ssh_session::authenticate_user(const ssh::Ssh_connection_options &config) {
  try {
    if (m_session->userauthNone() == SSH_AUTH_SUCCESS) {
      log_debug("SSH: session: Server accepts \"none\" auth method");
      return;
    }
    log_warning(
        "SSH: session: Failed authenticating with the \"none\" authentication "
        "method");
  } catch (::ssh::SshException &) {
    throw Ssh_tunnel_exception(ssh_get_error(m_session->getCSession()));
  }

  auto config_copy = config;
  int auth_list = m_session->getAuthList();

  ssh_set_blocking(m_session->getCSession(), 1);
  shcore::Scoped_callback scope(
      [this] { ssh_set_blocking(m_session->getCSession(), 0); });

  Ssh_auth_return ret_val = Ssh_auth_return::AUTH_NONE;

  if (auth_list == 0)
    throw std::runtime_error(
        "Unable to authenticate, no known authentication methods.");

  if (ret_val != Ssh_auth_return::AUTH_SUCCESS &&
      (auth_list & SSH_AUTH_METHOD_PUBLICKEY) &&
      m_config.auth_methods.is_set(Ssh_auth_methods::PUBKEY_AUTH)) {
    if (m_options.has_key_file()) {
      if (config_copy.get_key_encrypted()) {
        ret_val =
            try_auth([this, &config_copy]() { return auth_key(&config_copy); });
      } else {
        ret_val = auth_key(&config_copy);
      }
      if (ret_val != Ssh_auth_return::AUTH_SUCCESS) {
        log_warning(
            "SSH: session: Failed authenticating using the provided identity "
            "file");
      }

    } else {
      ret_val = auth_auto_pubkey();
      if (ret_val != Ssh_auth_return::AUTH_SUCCESS) {
        log_warning(
            "SSH: session: Failed authenticating using the default identity "
            "files");
      }
    }
  }

  if (ret_val != Ssh_auth_return::AUTH_SUCCESS &&
      (auth_list & SSH_AUTH_METHOD_PASSWORD) &&
      m_config.auth_methods.is_set(Ssh_auth_methods::PASSWORD_AUTH)) {
    ret_val = try_auth(
        [this, &config_copy]() { return auth_password(&config_copy); });
    if (ret_val != Ssh_auth_return::AUTH_SUCCESS) {
      log_warning("SSH: session: Failed authenticating using user/password");
    }
  }

  if (ret_val != Ssh_auth_return::AUTH_SUCCESS &&
      (auth_list & SSH_AUTH_METHOD_INTERACTIVE) &&
      m_config.auth_methods.is_set(Ssh_auth_methods::KBDINT_AUTH)) {
    if (!m_interactive)
      throw std::runtime_error(
          "Interactive auth mode is disabled when in disabled wizards mode.");
    ret_val = auth_interactive();
    if (ret_val != Ssh_auth_return::AUTH_SUCCESS) {
      log_warning(
          "SSH: session: Failed authenticating using interactive method");
    }
  }

  if (ret_val != Ssh_auth_return::AUTH_SUCCESS) {
    if (ret_val == Ssh_auth_return::AUTH_DENIED)
      throw Ssh_auth_exception("Access denied");
    else if (ret_val != Ssh_auth_return::AUTH_NONE)
      throw std::runtime_error(
          "Unable to authenticate, all known authentication methods failed.");
  }
}

namespace {
int call_auth_in_a_loop(::ssh::Session *session,
                        const std::function<int()> &c) {
  try {
    int ret = 0;
    do {
      ret = c();

      if (ret == SSH_AUTH_AGAIN) {
        // There's a bug in libssh that it returns SSH_AUTH_AGAIN
        // in blocking mode even while it should return SSH_DENIED;
        // This has to be removed, once libssh bug will be fixed.
        if (!ssh_is_connected(session->getCSession()))
          throw std::runtime_error("Unable to authenticate, " +
                                   std::string(session->getError()));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    } while (ret == SSH_AUTH_AGAIN);
    return ret;
  } catch (::ssh::SshException &sxc) {
    throw Ssh_auth_exception("Authentication failed: " + sxc.getError());
  }
}
}  // namespace

Ssh_auth_return Ssh_session::auth_agent(
    const ssh::Ssh_connection_options &config) {
  log_debug3("SSH trying auth_agent");
#ifdef _WIN32
  throw std::logic_error(
      "Ssh-agent functionality is not supported on this OS.");
#else
  return handle_auth_return(
      call_auth_in_a_loop(m_session.get(), [this, config]() {
        return ssh_userauth_agent(m_session->getCSession(), nullptr);
      }));
#endif
}

Ssh_auth_return Ssh_session::auth_password(
    ssh::Ssh_connection_options *config) {
  Ssh_auth_return ret_val = ssh::Ssh_auth_return::AUTH_NONE;
  log_debug3("SSH trying auth_pass");
  // here we need to take a pw from the user
  bool from_credential_manager = false;
  if (!config->has_password()) {
    bool ret = shcore::Credential_manager::get().get_password(config);
    if (ret && config->has_password()) from_credential_manager = true;

    if (!config->has_password()) {
      std::string passphrase;
      if (m_interactive) {
        if (mysqlsh::current_console()->prompt_password(
                "Please provide the password for " + config->as_uri() + ": ",
                &passphrase) == shcore::Prompt_result::Cancel)
          throw shcore::cancelled("User cancelled");
      }
      config->set_password(passphrase);
      shcore::clear_buffer(&passphrase);
    }
  }
  if (config->has_password()) {
    ret_val = handle_auth_return(
        call_auth_in_a_loop(m_session.get(), [this, config]() {
          return m_session->userauthPassword(config->get_password().c_str());
        }));

    if (ret_val == Ssh_auth_return::AUTH_DENIED) {
      bool had_password = config->has_password();
      config->clear_password();
      std::string error_msg =
          "SSH Access denied, connection to \"" +
          config->as_uri(mysqlshdk::db::uri::formats::user_transport()) +
          "\" could not be established.";

      if (from_credential_manager) {
        shcore::Credential_manager::get().remove_password(*config);
        error_msg.append(" Invalid password has been erased.");
      }
      mysqlsh::current_console()->print_error(error_msg);

      // We throw exception in case it was given as a part of uri
      // so the outer loop won't try to Auth using credential store.
      if (had_password) throw Ssh_auth_exception("Access denied");
    } else {
      if (!from_credential_manager)
        shcore::Credential_manager::get().save_password(*config);
    }
  } else {
    throw Ssh_auth_exception("Access denied");
  }
  return ret_val;
}

Ssh_auth_return Ssh_session::auth_auto_pubkey() {
  log_debug3("SSH trying auth_auto");

  return handle_auth_return(call_auth_in_a_loop(m_session.get(), [this]() {
    return m_session->userauthPublickeyAuto();
  }));
}

Ssh_auth_return Ssh_session::auth_key(ssh::Ssh_connection_options *config) {
  log_debug3("SSH trying auth_key");
  ssh_key priv_key;
  if (!shcore::is_file(config->get_key_file()))
    throw std::runtime_error("The key file does not exist.");

  std::string keypass;
  bool got_password = false;
  bool from_manager = false;
  if (config->get_key_encrypted()) {
    if (config->has_keyfile_password()) {
      keypass = config->get_key_file_password();
    } else {
      try {
        got_password = shcore::Credential_manager::get().get_credential(
            config->key_file_uri(), &keypass);
        from_manager = true;
      } catch (shcore::Exception &re) {
        mysqlsh::current_console()->print_error(
            "Unable to get credentials for " + config->key_file_uri() +
            " error was " + re.what());
      }

      if (!got_password && m_interactive &&
          (mysqlsh::current_console()->prompt_password(
               "Please provide key passphrase for " + config->get_key_file() +
                   ": ",
               &keypass) == shcore::Prompt_result::Cancel)) {
        throw shcore::cancelled("User cancelled");
      }
    }
  }

  auto ret = ssh_pki_import_privkey_file(config->get_key_file().c_str(),
                                         keypass.c_str(), libssh_auth_callback,
                                         nullptr, &priv_key);

  if (ret == SSH_OK) {
    shcore::Scoped_callback scope([&priv_key] { ssh_key_free(priv_key); });
    auto auth_info = call_auth_in_a_loop(m_session.get(), [this, priv_key]() {
      return m_session->userauthPublickey(priv_key);
    });

    if ((auth_info == SSH_AUTH_SUCCESS || auth_info == SSH_AUTH_PARTIAL) &&
        !keypass.empty()) {
      config->set_key_file_password(keypass);
      try {
        if (!got_password &&
            shcore::Credential_manager::get().should_save_password(
                config->key_file_uri()))
          shcore::Credential_manager::get().store_credential(
              config->key_file_uri(), keypass);
      } catch (shcore::Exception &re) {
        mysqlsh::current_console()->print_error(
            "An error occurred while storing the passphrase for the SSH "
            "identity file: " +
            std::string(re.what()));
      }
    }

    return handle_auth_return(auth_info);
  } else {
    log_error("SSH: session: %s", m_session->getError());
    // else is SSH_EOF according to docs it means, missing keyfile or
    // permission denied either way we move on

    if (shcore::is_file(config->get_key_file())) {
      if (from_manager && got_password) {
        shcore::Credential_manager::get().delete_credential(
            config->key_file_uri());
        mysqlsh::current_console()->print_error(
            "SSH Access denied, connection to \"" +
            config->as_uri(mysqlshdk::db::uri::formats::user_transport()) +
            "\" could not be established. Invalid password has been erased.");
      }
      return Ssh_auth_return::AUTH_DENIED;
    } else {
      throw std::invalid_argument("The file " + config->get_key_file() +
                                  " doesn't exist.");
    }
  }
}
namespace {
shcore::Prompt_result prompt_user(const std::string &prompt, std::string *reply,
                                  bool echo) {
  return echo ? mysqlsh::current_console()->prompt(prompt, reply)
              : mysqlsh::current_console()->prompt_password(prompt, reply);
}
}  // namespace

Ssh_auth_return Ssh_session::auth_interactive() {
  log_debug3("SSH trying auth_interactive");
  int ret_auth = 0;
  Ssh_auth_return ret_val = Ssh_auth_return::AUTH_NONE;
  while ((ret_auth = m_session->userauthKbdint(nullptr, nullptr)) ==
         SSH_AUTH_INFO) {
    for (auto i = 0; i < m_session->userauthKbdintGetNPrompts(); i++) {
      char echo;
      auto prompt =
          ssh_userauth_kbdint_getprompt(m_session->getCSession(), i, &echo);
      if (prompt == nullptr) {
        throw Ssh_auth_exception("Unknown authentication error: " +
                                 std::string(m_session->getError()));
      }

      std::string reply;
      if (prompt_user(prompt, &reply, echo) == shcore::Prompt_result::Cancel) {
        throw shcore::cancelled("User cancelled");
      }

      shcore::on_leave_scope cleanup([&reply]() {
        if (!reply.empty()) shcore::clear_buffer(&reply);
      });
      try {
        if (m_session->userauthKbdintSetAnswer(i, reply.c_str()) == 0) {
          ret_val = Ssh_auth_return::AUTH_SUCCESS;
        } else {
          throw Ssh_auth_exception("An error occurred: " +
                                   std::string(m_session->getError()));
        }
      } catch (::ssh::SshException &e) {
        throw Ssh_auth_exception(e.getError());
      }
    }
  }
  return ret_val;
}

Ssh_auth_return Ssh_session::handle_auth_return(int auth) {
  switch (auth) {
    case SSH_AUTH_DENIED:
      return Ssh_auth_return::AUTH_DENIED;
    case SSH_AUTH_SUCCESS:
      return Ssh_auth_return::AUTH_SUCCESS;
    case SSH_AUTH_PARTIAL:
      return Ssh_auth_return::AUTH_PARTIAL;
    case SSH_AUTH_INFO:
      return Ssh_auth_return::AUTH_INFO;
    default:
      if (!ssh_is_connected(m_session->getCSession()))
        throw std::runtime_error("Unable to authenticate, " +
                                 std::string(m_session->getError()));

      throw Ssh_auth_exception("Unknown authentication error (" +
                               std::to_string(auth) + ").");
  }
}
}  // namespace ssh
}  // namespace mysqlshdk

/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef _NGS_ASIO_OPTIONS_H_
#define _NGS_ASIO_OPTIONS_H_

#include <string>
#include <vector>
#include <boost/make_shared.hpp>


namespace ngs
{

  class Options_session
  {
  public:
    virtual ~Options_session() {};

    virtual bool supports_tls() = 0;
    virtual bool active_tls() = 0;

    virtual std::string ssl_cipher() = 0;
    virtual std::vector<std::string> ssl_cipher_list() = 0;

    virtual std::string ssl_version() = 0;

    virtual long ssl_verify_depth() = 0;
    virtual long ssl_verify_mode() = 0;
    virtual long ssl_sessions_reused() = 0;
  };

  class Options_context
  {
  public:
    virtual ~Options_context() {};

    virtual long ssl_ctx_verify_depth() = 0;
    virtual long ssl_ctx_verify_mode() = 0;

    virtual std::string ssl_server_not_after() = 0;
    virtual std::string ssl_server_not_before() = 0;

    virtual long ssl_sess_accept_good() = 0;
    virtual long ssl_sess_accept() = 0;
    virtual long ssl_accept_renegotiates() = 0;

    virtual std::string ssl_session_cache_mode() = 0;

    virtual long ssl_session_cache_hits() = 0;
    virtual long ssl_session_cache_misses() = 0;
    virtual long ssl_session_cache_overflows() = 0;
    virtual long ssl_session_cache_size() = 0;
    virtual long ssl_session_cache_timeouts() = 0;
    virtual long ssl_used_session_cache_entries() = 0;
  };

  typedef boost::shared_ptr<Options_session> Options_session_ptr;
  typedef boost::shared_ptr<Options_context> Options_context_ptr;

} // namespace ngs

#endif // _NGS_ASIO_OPTIONS_H_

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

#ifndef _NGS_ASIO_CONNECTION_RAW_FACTORY_H_
#define _NGS_ASIO_CONNECTION_RAW_FACTORY_H_

#include "myasio/connection_factory.h"
#include "myasio/connection_raw.h"


namespace yaSSL
{

class SSL_CTX;

}  // namespace yaSSL

namespace ngs
{

  class Connection_raw_factory: public Connection_factory
  {
  public:
    virtual Connection_unique_ptr create_connection(boost::asio::io_service &io_service)
    {
      return Connection_unique_ptr(new Connection_raw<boost::asio::ip::tcp::socket>(io_service));
    }

    virtual Options_context_ptr create_ssl_context_options()
    {
      return Options_context_ptr();
    }
  };

} // namespace ngs

#endif // _NGS_ASIO_CONNECTION_RAW_FACTORY_H_

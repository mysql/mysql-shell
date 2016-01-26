/*
 * Copyright (c) 2015, 2016 Oracle and/or its affiliates. All rights reserved.
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

#ifndef _NGS_ASIO_CONNECTION_H_
#define _NGS_ASIO_CONNECTION_H_

#include "myasio/types.h"
#include "myasio/options.h"

#include <boost/function.hpp>
#include "ngs/memory.h"


namespace ngs
{

class IConnection;

typedef boost::shared_ptr<IConnection>      IConnection_ptr;
typedef Memory_new<IConnection>::Unique_ptr IConnection_unique_ptr;

class IConnection
{
public:
  virtual ~IConnection() {};

  virtual Endpoint    get_remote_endpoint() const = 0;
  virtual int         get_socket_id() = 0;
  virtual IOptions_session_ptr options() = 0;

  virtual void async_connect(const Endpoint &endpoint, const On_asio_status_callback &on_connect_callback, const On_asio_status_callback &on_read_callback) = 0;
  virtual void async_accept(boost::asio::ip::tcp::acceptor &acceptor, const On_asio_status_callback &on_accept_callback, const On_asio_status_callback &on_read_callback) = 0;
  virtual void async_write(const Const_buffer_sequence &data, const On_asio_data_callback &on_write_callback) = 0;
  virtual void async_read(const Mutable_buffer_sequence &data, const On_asio_data_callback &on_read_callback) = 0;
  virtual void async_activate_tls(const On_asio_status_callback on_status) = 0;

  virtual IConnection_ptr get_lowest_layer() = 0;

  virtual void post(const boost::function<void ()> &calee) = 0;
  virtual bool thread_in_connection_strand() = 0;

  virtual void shutdown(boost::asio::socket_base::shutdown_type how_to_shutdown, boost::system::error_code &ec) = 0;
  virtual void cancel() = 0;
  virtual void close() = 0;
};

} // namespace ngs

#endif // _NGS_ASIO_CONNECTION_H_

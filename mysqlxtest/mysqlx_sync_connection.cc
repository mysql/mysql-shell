/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db


#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include "myasio/connection_dynamic_tls.h"
#include "myasio/connection_factory_openssl.h"
#include "myasio/connection_factory_yassl.h"
#include "myasio/connection_factory_raw.h"
#include "mysqlx_sync_connection.h"
#include "mysql.h"

namespace mysqlx
{


using namespace boost::system;
using namespace boost::asio::ip;
using namespace ngs;


namespace details
{


class Callback_executor
{
public:
  Callback_executor(boost::asio::io_service &service)
  : m_num_of_bytes(0U),
    m_error(boost::system::errc::make_error_code(boost::system::errc::io_error)),
    m_service(service)
  {}

  virtual ~Callback_executor() {}

  error_code wait()
  {
    bool executed;

    return wait(executed);
  }

  error_code wait(bool &executed)
  {
    m_service.reset();
    executed = m_service.run() > 0;

    return m_error;
  }

  std::size_t get_number_of_bytes()
  {
    return m_num_of_bytes;
  }

  void connect(ngs::IConnection_ptr async_connection, const Endpoint &endpoint)
  {
    preproces(async_connection);
    async_connection->async_connect(endpoint, get_status_callback(), On_asio_status_callback());
  }

  void accept(ngs::IConnection_ptr async_connection, boost::asio::ip::tcp::acceptor &acceptor)
  {
    preproces(async_connection);
    async_connection->async_accept(acceptor, get_status_callback(), On_asio_status_callback());
  }

  void write(ngs::IConnection_ptr async_connection, const Const_buffer_sequence &data)
  {
    preproces(async_connection);
    async_connection->async_write(data, get_data_callback());
  }

  void read(ngs::IConnection_ptr async_connection, const Mutable_buffer_sequence &data)
  {
    preproces(async_connection);
    async_connection->async_read(data, get_data_callback());
  }

  void activate_tls(ngs::IConnection_ptr async_connection)
  {
    preproces(async_connection);
    async_connection->async_activate_tls(get_status_callback());
  }

protected:
  virtual void preproces(ngs::IConnection_ptr async_connection) {}

  On_asio_status_callback get_status_callback()
  {
    return boost::bind(&Callback_executor::callback, this, boost::asio::placeholders::error, 0);
  }


  On_asio_data_callback get_data_callback()
  {
    return boost::bind(&Callback_executor::callback, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
  }


  virtual void callback(const error_code &ec, std::size_t num_of_bytes)
  {
    m_error = ec;
    m_num_of_bytes += num_of_bytes;
  }

  std::size_t m_num_of_bytes;
  error_code  m_error;
  boost::asio::io_service &m_service;
};


class Callback_executor_with_timeout : public Callback_executor
{
public:
  Callback_executor_with_timeout(boost::asio::io_service &io_service,
                                 const std::size_t timeout_in_miliseconds)
  : Callback_executor(io_service), m_deadline(io_service),
    m_timeout(timeout_in_miliseconds),
    m_is_expired(false)
  {}

  bool is_expired() const { return m_is_expired; }

private:
  virtual void preproces(ngs::IConnection_ptr async_connection)
  {
    m_is_expired = false;
    m_deadline.expires_from_now(boost::posix_time::milliseconds(m_timeout));
    m_deadline.async_wait(boost::bind(&Callback_executor_with_timeout::timeout_callback, this, _1, async_connection));
  }


  void timeout_callback(const error_code &ec, ngs::IConnection_ptr async_connection)
  {
    if (ec == boost::asio::error::operation_aborted)
      return;
    async_connection->close();
    m_is_expired = true;
  }


  void callback(const error_code &ec, std::size_t num_of_bytes)
  {
    Callback_executor::callback(ec, num_of_bytes);
    m_deadline.cancel();
  }


  boost::asio::deadline_timer m_deadline;
  const std::size_t m_timeout;
  bool m_is_expired;
};

Callback_executor *get_callback_executor(boost::asio::io_service &service, const std::size_t timeout)
{
  return timeout ?
      new Callback_executor_with_timeout(service, timeout) :
      new Callback_executor(service);
}

typedef Memory_new<Callback_executor>::Unique_ptr Callback_executor_ptr;

} // namespace details


Mysqlx_sync_connection::Mysqlx_sync_connection(boost::asio::io_service &service, const char *ssl_key,
                                               const char *ssl_ca, const char *ssl_ca_path,
                                               const char *ssl_cert, const char *ssl_cipher,
                                               const char *ssl_crl, const char *ssl_crl_path,
                                               const char *ssl_tls_version, int ssl_mode,
                                               const std::size_t timeout)
: m_service(service), m_timeout(timeout)
{
  m_async_factory    = get_async_connection_factory(ssl_key, ssl_ca, ssl_ca_path, ssl_cert, ssl_cipher, ssl_crl, ssl_crl_path, ssl_tls_version, ssl_mode);

  ngs::IConnection_unique_ptr async_connection = m_async_factory->create_connection(m_service);

  m_async_connection.reset(async_connection.release());
}

ngs::Connection_factory_ptr Mysqlx_sync_connection::get_async_connection_factory(const char *ssl_key, const char *ssl_ca,
  const char *ssl_ca_path, const char *ssl_cert, const char *ssl_cipher, const char *ssl_crl, const char *ssl_crl_path,
  const char *ssl_tls_version, int ssl_mode)
{
  const bool is_client = true;

  // If mode is any of PREFERRED (default), REQUIRED, VERIFY_CA, VERIFY_IDENTITY
  if (ssl_mode >= SSL_MODE_PREFERRED) {
#if !defined(DISABLE_SSL_ON_XPLUGIN)
    ngs::Connection_factory_ptr factory;

    ssl_key      = ssl_key      ? ssl_key      : "";
    ssl_ca       = ssl_ca       ? ssl_ca       : "";
    ssl_ca_path  = ssl_ca_path  ? ssl_ca_path  : "";
    ssl_cert     = ssl_cert     ? ssl_cert     : "";
    ssl_cipher   = ssl_cipher   ? ssl_cipher   : "";
    ssl_crl      = ssl_crl      ? ssl_crl      : "";
    ssl_crl_path = ssl_crl_path ? ssl_crl_path : "";
    if (!ssl_tls_version)
      ssl_tls_version = "";


#if !defined(HAVE_YASSL)
    factory = boost::make_shared<ngs::Connection_openssl_factory>(ssl_key, ssl_cert, ssl_ca, ssl_ca_path,
                                                                  ssl_cipher, ssl_crl, ssl_crl_path, 
                                                                  ssl_tls_version, ssl_mode, is_client);

#else
    factory = boost::make_shared<ngs::Connection_yassl_factory>(ssl_key, ssl_cert, ssl_ca, ssl_ca_path,
                                                                ssl_cipher, ssl_crl, ssl_crl_path, 
                                                                ssl_tls_version, ssl_mode, is_client);

#endif // HAVE_YASSL

     return factory;

#endif // !defined(DISABLE_SSL_ON_XPLUGIN)
  }

  return boost::make_shared<ngs::Connection_raw_factory>();
}


error_code Mysqlx_sync_connection::connect(const Endpoint &ep)
{
  details::Callback_executor_ptr executor(details::get_callback_executor(m_service, m_timeout));
  executor->connect(m_async_connection, ep);
  return executor->wait();
}


error_code Mysqlx_sync_connection::accept(const Endpoint &ep)
{
  boost::asio::ip::tcp::acceptor acceptor(m_service, ep);
  details::Callback_executor_ptr executor(details::get_callback_executor(m_service, m_timeout));
  executor->accept(m_async_connection, acceptor);
  return executor->wait();
}


error_code Mysqlx_sync_connection::activate_tls()
{
  details::Callback_executor_ptr executor(details::get_callback_executor(m_service, m_timeout));
  executor->activate_tls(m_async_connection);
  return executor->wait();
}


error_code Mysqlx_sync_connection::shutdown(boost::asio::socket_base::shutdown_type how_to_shutdown)
{
  error_code result;
  m_async_connection->shutdown(how_to_shutdown, result);
  return result;
}


error_code Mysqlx_sync_connection::write(const void *data, const std::size_t data_length)
{
  details::Callback_executor_ptr executor(details::get_callback_executor(m_service, m_timeout));
  Const_buffer_sequence buffers;

  for (;;)
  {
    buffers.push_back(boost::asio::buffer((char*)data + executor->get_number_of_bytes(),
                                    data_length - executor->get_number_of_bytes()));

    executor->write(m_async_connection, buffers);
    error_code err = executor->wait();
    if (err)
      return err;
    if (executor->get_number_of_bytes() < data_length)
    {
      buffers.clear();
    }
    else
      return err;
  }
}

error_code Mysqlx_sync_connection::read(void *data, const std::size_t data_length)
{
  error_code error;
  details::Callback_executor_ptr executor(details::get_callback_executor(m_service, m_timeout));

  while(!error && data_length != executor->get_number_of_bytes())
  {
    Mutable_buffer_sequence buffers;
    std::size_t in_buffer = executor->get_number_of_bytes();
    buffers.push_back(boost::asio::buffer((char*)data + in_buffer,  data_length - in_buffer));
    executor->read(m_async_connection, buffers);
    error = executor->wait();
  }

  return error;
}


error_code Mysqlx_sync_connection::read_with_timeout(void *data, std::size_t &data_length, const std::size_t deadline_miliseconds)
{
  error_code error;
  details::Callback_executor_with_timeout executor(m_service, deadline_miliseconds);

  while(!error && data_length != executor.get_number_of_bytes())
  {
    Mutable_buffer_sequence buffers;
    std::size_t in_buffer = executor.get_number_of_bytes();
    buffers.push_back(boost::asio::buffer((char*)data + in_buffer,  data_length - in_buffer));
    executor.read(m_async_connection, buffers);
    error = executor.wait();
  }
  if (executor.is_expired())
    data_length = 0;
  return error;
}


void Mysqlx_sync_connection::close()
{
  m_async_connection->close();
}

bool Mysqlx_sync_connection::supports_ssl()
{
  return m_async_connection->options()->supports_tls();;
}


bool Mysqlx_sync_connection::is_set(const char *string)
{
  return string ? strlen(string) > 0 : false;
}

} // namespace mysqlx

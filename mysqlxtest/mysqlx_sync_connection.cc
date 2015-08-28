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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db


#include "mysqlx_sync_connection.h"

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include "myasio/connection_dynamic_tls.h"
#include "myasio/connection_factory_openssl.h"
#include "myasio/connection_factory_yassl.h"
#include "myasio/connection_factory_raw.h"

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
  {
  }

  virtual ~Callback_executor() {}

  On_asio_status_callback get_status_callback()
  {
    return boost::bind(&Callback_executor::callback, this, boost::asio::placeholders::error, 0);
  }

  On_asio_data_callback get_data_callback()
  {
    return boost::bind(&Callback_executor::callback, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
  }

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

protected:
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
  Callback_executor_with_timeout(boost::asio::io_service &io_service, ngs::Connection_ptr &sync_connection, const std::size_t timeout_in_miliseconds, void *data_ptr, const std::size_t expected_data_size)
  : Callback_executor(io_service), m_async_connection(sync_connection), m_deadline(io_service), m_is_expired(false), m_expected_data_size(expected_data_size), m_data_ptr(data_ptr)
  {
    Mutable_buffer_sequence result;

    result.push_back(boost::asio::buffer(m_data_ptr, m_expected_data_size));

    m_deadline.expires_from_now(boost::posix_time::milliseconds(timeout_in_miliseconds));
    m_deadline.async_wait(boost::bind(&Callback_executor_with_timeout::deadline_timeout, this, _1));
    m_async_connection->async_read(result, this->get_data_callback());
  }

  bool is_timeout()
  {
    return m_is_expired;
  }

private:
  void deadline_timeout(const error_code &ec)
  {
    if (!ec)
    {
      m_is_expired = true;
      m_async_connection->close();
    }
  }

  void callback(const error_code &ec, std::size_t num_of_bytes)
  {
    Callback_executor::callback(ec, num_of_bytes);

    const bool is_io_finished = m_num_of_bytes == m_expected_data_size;

    if (ec || is_io_finished)
    {
      m_deadline.cancel();
      return;
    }

    Mutable_buffer_sequence result;

    result.push_back(boost::asio::buffer(m_data_ptr, m_expected_data_size));

    m_async_connection->async_read(result, this->get_data_callback());
  }

  ngs::Connection_ptr         &m_async_connection;
  boost::asio::deadline_timer  m_deadline;
  bool                         m_is_expired;
  const std::size_t            m_expected_data_size;
  void*                        m_data_ptr;
};

} // namespace details


Mysqlx_sync_connection::Mysqlx_sync_connection(boost::asio::io_service &service, const char *ssl_key, const char *ssl_ca,
    const char *ssl_ca_path, const char *ssl_cert, const char *ssl_cipher)
: m_service(service)
{
  m_async_factory    = get_async_connection_factory(ssl_key, ssl_ca, ssl_ca_path, ssl_cert, ssl_cipher);

  ngs::Connection_unique_ptr async_connection = m_async_factory->create_connection(m_service);

  m_async_connection.reset(async_connection.release());
}

ngs::Connection_factory_ptr Mysqlx_sync_connection::get_async_connection_factory(const char *ssl_key, const char *ssl_ca,
    const char *ssl_ca_path, const char *ssl_cert, const char *ssl_cipher)
{
  const bool is_client = true;

  if (is_set(ssl_key) || is_set(ssl_ca) || is_set(ssl_ca_path) || is_set(ssl_cert) || is_set(ssl_cipher))
  {
#if !defined(DISABLE_SSL_ON_XPLUGIN)
    ngs::Connection_factory_ptr factory;

    ssl_key      = ssl_key      ? ssl_key      : "";
    ssl_ca       = ssl_ca       ? ssl_ca       : "";
    ssl_ca_path  = ssl_ca_path  ? ssl_ca_path  : "";
    ssl_cert     = ssl_cert     ? ssl_cert     : "";
    ssl_cipher   = ssl_cipher   ? ssl_cipher   : "";


#if !defined(HAVE_YASSL)
    factory = boost::make_shared<ngs::Connection_openssl_factory>(ssl_key, ssl_cert, ssl_ca, ssl_ca_path,
                                                                  ssl_cipher, "", "", is_client);

#else
    factory = boost::make_shared<ngs::Connection_yassl_factory>(ssl_key, ssl_cert, ssl_ca, ssl_ca_path,
                                                                ssl_cipher, "", "", is_client);

#endif // HAVE_YASSL

     return factory;

#endif // !defined(DISABLE_SSL_ON_XPLUGIN)
  }

  return boost::make_shared<ngs::Connection_raw_factory>();
}


error_code Mysqlx_sync_connection::connect(const Endpoint &ep)
{
  details::Callback_executor     executor(m_service);

  m_async_connection->async_connect(ep, executor.get_status_callback(), On_asio_status_callback());

  return executor.wait();
}


error_code Mysqlx_sync_connection::accept(const Endpoint &ep)
{
  boost::asio::ip::tcp::acceptor acceptor(m_service, ep);
  details::Callback_executor     executor(m_service);

  m_async_connection->async_accept(acceptor, executor.get_status_callback(), On_asio_status_callback());

  return executor.wait();
}


error_code Mysqlx_sync_connection::activate_tls()
{
  details::Callback_executor executor(m_service);

  m_async_connection->async_activate_tls(executor.get_status_callback());

  return executor.wait();
}


error_code Mysqlx_sync_connection::shutdown(boost::asio::socket_base::shutdown_type how_to_shutdown)
{
  error_code result;

  m_async_connection->shutdown(how_to_shutdown, result);

  return result;
}


error_code Mysqlx_sync_connection::write(const void *data, const std::size_t data_length)
{
  details::Callback_executor executor(m_service);
  Const_buffer_sequence      buffers;

  for (;;)
  {
    buffers.push_back(boost::asio::buffer((char*)data + executor.get_number_of_bytes(),
                                    data_length - executor.get_number_of_bytes()));

    m_async_connection->async_write(buffers, executor.get_data_callback());
    error_code err = executor.wait();
    if (err)
      return err;
    if (executor.get_number_of_bytes() < data_length)
    {
      buffers.clear();
    }
    else
      return err;
  }
}

error_code Mysqlx_sync_connection::read(void *data, const std::size_t data_length)
{
  details::Callback_executor executor(m_service);
  error_code                 error;

  while(!error && data_length != executor.get_number_of_bytes())
  {
    Mutable_buffer_sequence buffers;
    std::size_t in_buffer = executor.get_number_of_bytes();

    buffers.push_back(boost::asio::buffer((char*)data + in_buffer,  data_length - in_buffer));

    m_async_connection->async_read(buffers, executor.get_data_callback());

    error = executor.wait();
  }

  return error;
}


error_code Mysqlx_sync_connection::read_with_timeout(void *data, std::size_t &data_length, const std::size_t deadline_miliseconds)
{
  details::Callback_executor_with_timeout executor(m_service, m_async_connection, deadline_miliseconds, data, data_length);

  error_code error = executor.wait();

  if (executor.is_timeout())
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
  if (string)
    return strlen(string) > 0;

  return false;
}


} // namespace mysqlx

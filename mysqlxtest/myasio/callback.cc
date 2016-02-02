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


#include <boost/bind.hpp>


#include "myasio/types.h"
#include "myasio/callback.h"


#define LOG_DOMAIN "ngs.protocol"
#include "ngs/log.h"


namespace ngs
{


void Callback_direct::call_status_function(On_asio_status_callback &function, const boost::system::error_code &error)
{
  if (function)
  {
    On_asio_status_callback calee = function;
    function.clear();

    calee(error);
  }
}


void Callback_direct::call_data_function(On_asio_data_callback &function, const boost::system::error_code &error, const int received_data)
{
  if (!function)
  {
    log_error("Callback already called error:%i, received_data:%i", (int)(bool)error, received_data);
    return;
  }

  On_asio_data_callback calee = function;
  function.clear();

  calee(error, received_data);
}


void Callback_direct::call_data_function(On_asio_data_callback &function, const int received_data)
{
  call_data_function(function, boost::system::error_code(), received_data);
}


Callback_post::Callback_post(const Function_post &post)
: m_post(post)
{
}


void Callback_post::call_status_function(On_asio_status_callback &function, const boost::system::error_code &error)
{
  if (function)
  {
    On_asio_status_callback calee = function;
    function.clear();

    m_post(boost::bind(calee, error));
  }
}


void Callback_post::call_data_function(On_asio_data_callback &function, const boost::system::error_code &error, const int received_data)
{
  if (!function)
  {
    log_error("Callback already called error:%i, received_data:%i", (int)(bool)error, received_data);
    return;
  }

  On_asio_data_callback calee = function;
  function.clear();

  m_post(boost::bind(calee, error, received_data));
}


void Callback_post::call_data_function(On_asio_data_callback &function, const int received_data)
{
  call_data_function(function, boost::system::error_code(), received_data);
}


}  // namespace ngs


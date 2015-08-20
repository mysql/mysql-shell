/* Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */


#ifndef _NGS_ASIO_TYPES_SSL_
#define _NGS_ASIO_TYPES_SSL_


#include <boost/function.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>
#include <vector>


namespace ngs
{


typedef boost::asio::ip::tcp::endpoint Endpoint;
typedef boost::asio::ip::tcp::socket::lowest_layer_type  Asio_socket;
typedef boost::asio::ip::tcp::socket::native_handle_type Native_socket;
typedef boost::function<void (const boost::system::error_code &)> On_asio_status_callback;
typedef boost::function<void (const boost::system::error_code &, std::size_t)> On_asio_data_callback;
typedef std::vector<boost::asio::mutable_buffer> Mutable_buffer_sequence;
typedef std::vector<boost::asio::const_buffer>   Const_buffer_sequence;


struct const_buffer_with_callback
{
  Const_buffer_sequence buffer;
  On_asio_data_callback on_sdu_send_user_callback;
  std::size_t           write_user_data_size;
};

#define BUFFER_PAGE_SIZE 4096

// A 4KB aligned buffer to be used for reading data from sockets.
// Multiple buffers can be combined into a single boost::buffer, which is
// all filled at once by the boost async read method
struct Buffer_page
{
  char *data;
  uint32_t capacity;
  uint32_t length;

  Buffer_page()
  {
    capacity = BUFFER_PAGE_SIZE;
    data = new char[capacity];
    length = 0;
  }

  Buffer_page(uint32_t pcapacity)
  {
    //assert(psize > 8) 8 is absolute min size
    capacity = pcapacity;
    data = new char[capacity];
    length = 0;
  }

  ~Buffer_page()
  {
    delete []data;
  }

  uint32_t get_free_bytes()
  {
    return capacity - length;
  }

  uint8_t* get_free_ptr()
  {
    return (uint8_t*)data + length;
  }
};

} // namespace ngs

#endif // _NGS_ASIO_TYPES_SSL_

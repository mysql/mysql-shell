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


#ifndef _MYSQLX_DATETIME_H_
#define _MYSQLX_DATETIME_H_

#include <string>
#include <stdint.h>
#include <ostream>

namespace mysqlx
{
  class Time
  {
  public:
    Time() : m_hour(0), m_minutes(0), m_seconds(0),
    m_useconds(0), m_valid(false)
    {}

    Time(uint8_t hour_,
         uint8_t minutes_,
         uint8_t seconds_,
         uint32_t useconds_ = 0)
    : m_hour(hour_), m_minutes(minutes_), m_seconds(seconds_),
    m_useconds(useconds_), m_valid(true)
    {
      if (hour_ > 23 || minutes_ > 59 || seconds_ > 59 || useconds_ >= 1000000)
        m_valid = false;
    }

    explicit Time(const std::string &s)
    : m_useconds(0), m_valid(false)
    {
      uint32_t word;
      if (s.size() >= 3 && s.size() <= 6)
      {
        const unsigned char *data = reinterpret_cast<const unsigned char*>(s.data());

        // 5 bits for hour (0-23) / 6 bits for mins (0-59) / 6 bits for secs (0-59)
        word = (uint32_t)data[0] | (uint32_t)data[1] << 8 | (uint32_t)data[2] << 16;
        m_hour = word & 0x1f;
        m_minutes = (word >> 5) & 0x3f;
        m_seconds = (word >> 11) & 0x3f;

        // microsecond part is a varint
        switch (s.size() - 3)
        {
          case 3:
            m_useconds = ((uint32_t)data[5] & 0x7F) << 14;
          case 2:
            m_useconds = m_useconds | ((uint32_t)data[4] & 0x7F) << 7;
          case 1:
            m_useconds = m_useconds | ((uint32_t)data[3] & 0x7F);
            break;
          case 0:
            break;
        }
        m_valid = true;
      }
    }

    std::string to_bytes() const
    {
      if (!m_valid)
        return "";

      std::string s;
      uint32_t word;
      word = (m_hour & 0x1f) | (m_minutes & 0x3f) << 5 | (m_seconds & 0x3f) << 11;
      s.push_back((char)(word & 0xff));
      s.push_back((char)((word >> 8) & 0xff));
      s.push_back((char)((word >> 16) & 0xff));
      if (m_useconds)
      {
        word = m_useconds;
        // encode useconds as a varint
        if (word > 0x7f)
        {
          s.push_back((word & 0x7f) | 128);
          if (word > 0x3fff)
          {
            s.push_back(((word >> 7) & 0x7f) | 128);
            s.push_back((word >> 14) & 0x7f);
          }
          else
            s.push_back(word >> 7);
        }
        else
          s.push_back(word);
      }
      return s;
    }

    bool valid() const
    {
      return m_valid;
    }

    operator bool () const
    {
      return m_valid;
    }

    uint8_t hour() const { return m_hour; }
    uint8_t minutes() const { return m_minutes; }
    uint8_t seconds() const { return m_seconds; }
    uint32_t useconds() const { return m_useconds; }

  private:
    uint8_t m_hour;
    uint8_t m_minutes;
    uint8_t m_seconds;
    uint32_t m_useconds;
    bool m_valid;
  };


  class DateTime
  {
  public:
    explicit DateTime(const std::string &s)
    : m_hour(0), m_minutes(0), m_seconds(0), m_useconds(0), m_valid(false)
    {
      const unsigned char *data = reinterpret_cast<const unsigned char*>(s.data());
      uint32_t word;
      if (s.size() >= 3)
      {
        // date only
        word = (uint32_t)data[0] | (uint32_t)data[1] << 8 | (uint32_t)data[2] << 16;

        // 14bits for year (0-9999) / 4 bits for month (1-12) / 5 bits for day (0-31)
        m_year = word & 0x3fff;
        m_month = (word >> 14) & 0xf;
        m_day = (word >> 18) & 0x1f;

        m_hour = 0xff;
        m_valid = true;

        if (s.size() >= 6 && s.size() <= 9)
        {
          // date and time

          // 5 bits for hour (0-23) / 6 bits for mins (0-59) / 6 bits for secs (0-59)
          word = (uint32_t)data[3] | (uint32_t)data[4] << 8 | (uint32_t)data[5] << 16;
          m_hour = word & 0x1f;
          m_minutes = (word >> 5) & 0x3f;
          m_seconds = (word >> 11) & 0x3f;

          // microsecond part is a varint
          switch (s.size() - 6)
          {
            case 3:
              m_useconds = ((uint32_t)data[8] & 0x7F) << 14;
            case 2:
              m_useconds = m_useconds | ((uint32_t)data[7] & 0x7F) << 7;
            case 1:
              m_useconds = m_useconds | ((uint32_t)data[6] & 0x7F);
              break;
            case 0:
              break;
          }
        }
        else if (s.size() > 3)
          m_valid = false;
      }
    }

    DateTime(uint16_t year_,
             uint8_t month_,
             uint8_t day_,

             uint8_t hour_ = 0xff,
             uint8_t minutes_ = 0,
             uint8_t seconds_ = 0,
             uint32_t useconds_ = 0)
    : m_year(year_), m_month(month_), m_day(day_),
    m_hour(hour_), m_minutes(minutes_), m_seconds(seconds_),
    m_useconds(useconds_), m_valid(true)
    {
      if (year_ > 9999 || month_ > 12 || day_ > 31)
        m_valid = false;
      if (hour_ != 0xff)
      {
        if (hour_ > 23 || minutes_ > 59 || seconds_ > 59 || useconds_ >= 1000000)
          m_valid = false;
      }
    }

    DateTime() {}

    std::string to_bytes() const
    {
      if (!m_valid)
        return "";

      std::string s;
      uint32_t word;
      word = m_year | (uint32_t)(m_month & 0xf) << 14 | (uint32_t)(m_day & 0x1f) << 18;
      s.push_back((char)(word & 0xff));
      s.push_back((char)((word >> 8) & 0xff));
      s.push_back((char)((word >> 16) & 0xff));

      if (m_hour != 0xff)
      {
        word = (uint32_t)(m_hour & 0x1f) | (uint32_t)(m_minutes & 0x3f) << 5 | (uint32_t)(m_seconds & 0x3f) << 11;
        s.push_back((char)(word & 0xff));
        s.push_back((char)((word >> 8) & 0xff));
        s.push_back((char)((word >> 16) & 0xff));
        if (m_useconds)
        {
          word = m_useconds;
          // encode useconds as a varint
          if (word > 0x7f)
          {
            s.push_back((word & 0x7f) | 128);
            if (word > 0x3fff)
            {
              s.push_back(((word >> 7) & 0x7f) | 128);
              s.push_back((word >> 14) & 0x7f);
            }
            else
              s.push_back(word >> 7);
          }
          else
            s.push_back(word);
        }
      }
      return s;
    }

    bool valid() const
    {
      return m_valid;
    }

    operator bool () const
    {
      return m_valid;
    }

    bool has_time() const
    {
      return m_hour != 0xff;
    }

    uint16_t year() const { return m_year; }
    uint8_t month() const { return m_month; }
    uint8_t day() const { return m_day; }

    uint8_t hour() const { return m_hour; }
    uint8_t minutes() const { return m_minutes; }
    uint8_t seconds() const { return m_seconds; }
    uint32_t useconds() const { return m_useconds; }
    Time time() const { return Time(m_hour, m_minutes, m_seconds, m_useconds); }

  private:
    uint16_t m_year;
    uint8_t m_month;
    uint8_t m_day;

    uint8_t m_hour;
    uint8_t m_minutes;
    uint8_t m_seconds;
    uint32_t m_useconds;

    bool m_valid;
  };


  inline std::ostream & operator<<(std::ostream &os, const DateTime &v)
  {
    if (v)
    {
      os << v.year() << "/" << v.month() << "/" << v.day();
      if (v.has_time())
      {
        double us = v.seconds() + v.useconds()/1000000.0;
        os << " " << v.hour() << ":" << v.minutes() << ":" << us;
      }
    }
    return os;
  }

  inline std::ostream & operator<<(std::ostream &os, const Time &v)
  {
    if (v)
    {
      double us = v.seconds() + v.useconds()/1000000.0;
      os << " " << v.hour() << ":" << v.minutes() << ":" << us;
    }
    return os;
  }
}

#endif

//
//  utils_time.cpp
//  mysh
//
//  Created by Juan René Ramírez Monarrez on 1/10/15.
//
//

#include "utils_time.h"
#include <boost/format.hpp>
#include <cmath>

#if defined(WIN32)
#include <time.h>
#else
#include <sys/times.h>
#ifdef _SC_CLK_TCK				// For mit-pthreads
#undef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC (sysconf(_SC_CLK_TCK))
#endif
#endif


unsigned long MySQL_timer::get_time()
{
#if defined(WIN32)
  return clock();
#else
  struct tms tms_tmp;
  return times(&tms_tmp);
#endif
}

unsigned long MySQL_timer::start()
{
  return (_start = get_time());
}

unsigned long MySQL_timer::end()
{
  return (_end = get_time());
}

/**
 Write as many as 52+1 bytes to buff, in the form of a legible duration of time.
 
 len("4294967296 days, 23 hours, 59 minutes, 60.00 seconds")  ->  52
 */

std::string MySQL_timer::format_legacy(unsigned long raw_time, bool part_seconds)
{
  std::string str_duration;
  
  int days = 0;
  int hours = 0;
  int minutes = 0;
  float seconds = 0.0;
  
  MySQL_timer::parse_duration(raw_time, days, hours, minutes, seconds);

  if (days)
    str_duration.append((boost::format("%d %s") % days % (days == 1 ? "day" : "days")).str());
  
  if (days || hours)
    str_duration.append((boost::format("%s%d %s") % (str_duration.empty() ? "" : ", ") % hours % (hours == 1 ? "hour" : "hours")).str());
  
  if (days || hours || minutes)
    str_duration.append((boost::format("%s%d %s") % (str_duration.empty() ? "" : ", ") % minutes % (minutes == 1 ? "minute" : "minute")).str());
  
  if (part_seconds)
    str_duration.append((boost::format("%.2f sec") % seconds).str());
  else
    str_duration.append((boost::format("%d sec") % seconds).str());
  
  return str_duration;
}

void MySQL_timer::parse_duration(int raw_time, int &days, int &hours, int &minutes, float &seconds)
{
  double duration = (double)(raw_time) / CLOCKS_PER_SEC;
  double temp;
  std::string str_duration;

  double minute_seconds = 60.0;
  double hour_seconds = 3600.0;
  double day_seconds = hour_seconds * 24;

  if (duration >= day_seconds)
  {
    days = (int)floor(duration / day_seconds);
    duration -= temp * day_seconds;
  }

  if (duration >= hour_seconds)
  {
    hours = (int)floor(duration / hour_seconds);
    duration -= temp * hour_seconds;
  }

  if (duration >= minute_seconds)
  {
    minutes = (int)floor(duration / minute_seconds);
    duration -= temp * minute_seconds;
  }

  seconds = duration;
}
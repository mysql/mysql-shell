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
  double duration = (double)(raw_time) / CLOCKS_PER_SEC;
  double temp;
  std::string str_duration;
  
  double minute_seconds = 60.0;
  double hour_seconds = 3600.0;
  double day_seconds = hour_seconds * 24;
  
  if (duration >= day_seconds)
  {
    temp = floor(duration/day_seconds);
    duration -= temp * day_seconds;
    
    str_duration.append((boost::format("%d %s") % temp % (temp == 1 ? "day" : "days")).str());
  }
  
  if (duration >= hour_seconds)
  {
    temp = floor(duration/hour_seconds);
    duration -= temp * hour_seconds;
    
    if (!str_duration.empty())
      str_duration.append(", ");
    
    str_duration.append((boost::format("%d %s") % temp % (temp == 1 ? "hour" : "hours")).str());
  }
  
  if (duration >= minute_seconds)
  {
    temp = floor(duration/minute_seconds);
    duration -= temp * minute_seconds;
    
    if (!str_duration.empty())
      str_duration.append(", ");
    
    str_duration.append((boost::format("%d %s") % temp % (temp == 1 ? "minute" : "minute")).str());
  }
  
  if (part_seconds)
    str_duration.append((boost::format("%.2f sec") % duration).str());
  else
    str_duration.append((boost::format("%d sec") % duration).str());
  
  return str_duration;
}
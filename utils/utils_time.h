//
//  utils_time.h
//  mysh
//
//  Created by Juan René Ramírez Monarrez on 1/10/15.
//
//

#ifndef __mysh__utils_time__
#define __mysh__utils_time__

#include <string>

class MySQL_timer
{
public:
  unsigned long get_time();
  unsigned long start();
  unsigned long end();
  std::string format_legacy(bool part_seconds);

private:
  unsigned long _start;
  unsigned long _end;
};

#endif /* defined(__mysh__utils_time__) */

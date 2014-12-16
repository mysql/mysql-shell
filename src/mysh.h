
#ifndef _MYSH_H_
#define _MYSH_H_

#include "shellcore/shell_core.h"

class Interactive_shell
{
public:
  enum Mode
  {
    SQLMode,
    JSMode,
    PyMode
  };

private:
  void process_line(const std::string &line);


private:
  boost::shared_ptr<shcore::Shell_core> _shell;

  std::string _line_buffer;
};


#endif // _MYSH_H_


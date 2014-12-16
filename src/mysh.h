
#ifndef _MYSH_H_
#define _MYSH_H_

#include "shellcore/shellcore.h"

class InteractiveShell
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
  boost::shared_ptr<ShellCore> _shell;

  std::string _line_buffer;
};


#endif // _MYSH_H_


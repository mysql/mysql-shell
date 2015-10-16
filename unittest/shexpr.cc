
#include <iostream>
#include <string>
#include <vector>

#include "../mysqlxtest/common/expr_parser.h"

#include "mysqlx_expr.pb.h"
#include <google/protobuf/text_format.h>


int main(int argc, char **argv)
{
  if (argc != 3)
  {
    printf("%s tab|col <expr>\n", argv[0]);
    return 1;
  }

  const char *type = argv[1];
  const char *expr = argv[2];

  try
  {
    mysqlx::Expr_parser p(expr, strcmp(type, "col") == 0);

    Mysqlx::Expr::Expr *expr = p.expr();
    std::string out;
    google::protobuf::TextFormat::PrintToString(*expr, &out);
    delete expr;

    std::cout << "OK\n";
    std::cout << out << "\n";
  }
  catch (mysqlx::Parser_error &e)
  {
    std::cout << "ERROR\n";
    std::cout << e.what() << "\n";
    // must exit with 0
  }
  catch (std::exception &e)
  {
    std::cout << "UNEXPECTED ERROR\n";
    std::cout << e.what() << "\n";
    return 1;
  }

  return 0;
}

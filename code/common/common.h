# include <iostream>
# include <fstream>
# include <sstream>
# include <vector>
# include <map>
# include <utility>

# define NIL(type) (type)0
# define COMMON_DEBUG 1
# define _setNULL(arg, type) \
  arg = NIL(type)

using namespace std;

void common_error(string);
void common_message(string);

string getStrFromInt(int);

#include <ixlib_js_internals.hh>
#include <ixlib_exbase.hh>
#include <ixlib_javascript.hh>

#include <fstream>

using namespace ixion;
using namespace ixion::javascript;

IXLIB_JS_DECLARE_FUNCTION(write)
{
  if (parameters.size() != 1) {
    EXJS_THROWINFO(ECJS_INVALID_NUMBER_OF_ARGUMENTS, "write");
  }
  std::cout << parameters[0]->toString();
  return makeConstant(parameters[0]->toString());
}

int main (int ac, char ** av) {
   interpreter *jsint = new interpreter();
   addStandardLibrary(*jsint);
   ref<value> x = new write();
   jsint->RootScope->addMember("write", x);

   if (ac == 1) {
     std::cerr << "Usage: " << av[0] << "<file+>" << std::endl;
     exit(1);
   }
   for (int i = 1; i < ac; i++) {
     std::ifstream input(av[i]);
     try {
       ref<value> result = jsint->execute(input);
       std::cout << av[i] << " returned " << result->stringify() << std::endl;
     } catch (base_exception &ex) {
       std::cerr << ex.getText() << ex.what() << std::endl;
     }
     input.close();
   }
   delete jsint;
}
   

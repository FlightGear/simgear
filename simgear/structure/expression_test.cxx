#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#ifdef NDEBUG
// Always enable DEBUG mode in test application, otherwise "assert" test
// statements have no effect and don't actually test anything (catch 17 ;-) ).
#undef NDEBUG
#endif

#include <simgear/compiler.h>

#include <iostream>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>

#include <simgear/misc/test_macros.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/SGExpression.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>

using namespace std;    
using namespace simgear;

SGPropertyNode_ptr propertyTree;

void initPropTree()
{
  const char* xml = "<?xml version=\"1.0\"?>"
      "<PropertyList>"
          "<group-a>"
              "<foo>one</foo>"
              "<bar>2</bar>"
              "<zot>99</zot>"
          "</group-a>"
          "<group-b>"
              "<thing-1 type=\"bool\">false</thing-1>"
          "</group-b>"
      "</PropertyList>";
  
  propertyTree = new SGPropertyNode;
  readProperties(xml, strlen(xml), propertyTree.ptr());
}

void testBasic()
{
  
}

void testParse()
{
    initPropTree();
#if 0
    const char* xml = "<?xml version=\"1.0\"?>"
        "<PropertyList>"
            "<expression>"
              "<and>"
              "<greater-than>"
                "<property>/group-a/bar</property>"
                "<value>42</value>"
              "</greater-than>"
              "<less-than>"
                "<property>/group-a/zot</property>"
                "<value>50</value>"
              "</less-than>"
              "</and>"
            "</expression>"
        "</PropertyList>";
#endif
    const char* xml2 = "<?xml version=\"1.0\"?>"
        "<PropertyList>"
              "<sqr>"
                "<max>"
                  "<property>/group-a/bar</property>"
                  "<property>/group-a/zot</property>"
                  "<property>/group-b/thing-1</property>"    
                "</max>"
              "</sqr>"
        "</PropertyList>";
    
    auto desc = std::unique_ptr<SGPropertyNode>(new SGPropertyNode);
    readProperties(xml2, strlen(xml2), desc.get());
    
    SGSharedPtr<SGExpressiond> expr = SGReadDoubleExpression(propertyTree, desc->getChild(0));
    
    std::set<const SGPropertyNode*> deps;
    expr->collectDependentProperties(deps);
    
    SG_CHECK_EQUAL(deps.size(), 3);
    SGPropertyNode* barProp = propertyTree->getNode("group-a/bar");
    SG_VERIFY(deps.find(barProp) != deps.end());

    SG_VERIFY(deps.find(propertyTree->getNode("group-a/zot")) != deps.end());
    SG_VERIFY(deps.find(propertyTree->getNode("group-b/thing-1")) != deps.end());
}

int main(int argc, char* argv[])
{
    sglog().setLogLevels( SG_ALL, SG_INFO );
  
    testBasic();
    testParse();
    
    cout << __FILE__ << ": All tests passed" << endl;
    return EXIT_SUCCESS;
}

#include "strutils.hxx"
#include <iostream>
#include <string>

int main()
{
    std::string utf8_string1 = "Zweibr\u00FCcken";
    //valid UTF-8, convertible to Latin-1
    std::string latin1_string1 = "Zweibr\374cken";
    //Latin-1, not valid UTF-8
    std::string utf8_string2 = "\u600f\U00010143";
    //valid UTF-8, out of range for Latin-1
    
    std::string output_string1u = simgear::strutils::utf8ToLatin1(utf8_string1);
    if (output_string1u.compare(latin1_string1)){
        std::cerr << "Conversion fail: "
        << output_string1u << "!=" << latin1_string1;
        return 1;
    }
    std::string output_string1l = simgear::strutils::utf8ToLatin1(latin1_string1);
    if (output_string1l.compare(latin1_string1)){
        std::cerr << "Non-conversion fail: "
        << output_string1l << "!=" << latin1_string1;
        return 1;
    }
    std::string output_string3 = simgear::strutils::utf8ToLatin1(utf8_string2);
    //we don't check the result of this one as there is no right answer,
    //just make sure it doesn't crash/hang
    return 0;
}

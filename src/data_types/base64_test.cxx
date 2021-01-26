#include "sys.h"
#include "BinaryData.h"
#include "debug.h"
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
  Debug(debug::init());

  std::string s;
  std::getline(std::cin, s, '\n');
  std::cout << "Your string is: '" << s << "'" << std::endl;

  BinaryData bd;

  bd.assign_from_binary(s);
  std::cout << "C escaped representation: " << bd << std::endl;
  // Encode
  std::string base64 = bd.to_base64_string();
  std::cout << "Base64 representation: " << base64 << std::endl;
  // Decode
  bd.assign_from_base64(base64);
  std::cout << "Decoded: '" << bd.to_string() << "'" << std::endl;
}

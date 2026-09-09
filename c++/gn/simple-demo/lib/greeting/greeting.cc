#include "greeting.h"

namespace greeting {

std::string hello(const std::string& name) {
  return "Hello, " + name + "!";
}

}  // namespace greeting

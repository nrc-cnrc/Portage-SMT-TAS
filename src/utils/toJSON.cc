#include "toJSON.h"

namespace Portage {
float to_JSON(const float& o) {
   return o;
}

double to_JSON(const double& o) {
   return o;
}

int to_JSON(const int& o) {
   return o;
}

long to_JSON(const long& o) {
   return o;
}

unsigned int to_JSON(const unsigned int& o) {
   return o;
}

unsigned long to_JSON(const unsigned long& o) {
   return o;
}

const char* to_JSON(const bool& o) {
   return (o ? "true" : "false");
}



string keyJSON(const char* const key) {
   string t(key);
   replaceAll(t, string("\""), string("\\\""));
   return '"' + t + "\":";
}
}

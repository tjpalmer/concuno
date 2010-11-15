#include "context.h"
#include <iostream>

using namespace std;

namespace cuncuno {

Context& Context::global() {
  static Context context;
  return context;
}

void Context::log(const string& message) {
  cout << message << endl;
}

Worker::Worker(const string& n): localContext(0), name(n) {}

Context& Worker::context() {
  return localContext ? *localContext : Context::global();
}

void Worker::log(const string& message) {
  context().log(message);
}

}

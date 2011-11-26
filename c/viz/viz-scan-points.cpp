/*
Renders scan points, including their temporary models, from a concuno log.
*/


#include <concuno.h>
#include <fstream>


bool beginsWith(const std::string& $string, const std::string& prefix);


using namespace concuno;
using namespace std;


int main(int argc, char** argv) {
  if (argc < 2) {
    throw Error(Buf() << "Usage: " << argv[0] << " <log-file-name>");
  }
  string line;
  fstream logIn(argv[1]);
  while (logIn) {
    getline(logIn, line);
    // TODO Get a regex engine?
    if (beginsWith(line, "Expanding on ")) {
      // TODO Parse.
      cout << line << endl;
    } else if (beginsWith(line, "scanByPointScore/each:")) {
      // TODO Parse.
      cout << line << endl;
    }
  }
  return EXIT_SUCCESS;
}


bool beginsWith(const std::string& $string, const std::string& prefix) {
  // TODO Something more efficient! Also something respecting non-null-term!
  return !$string.find(prefix.c_str());
}

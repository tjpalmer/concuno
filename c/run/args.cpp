#include "args.h"


namespace concuno {namespace run {


Args::Args(int argc, char** argv) {
  parse(argc, argv);
}


void Args::parse(int argc, char** argv) {
  if (argc < 4) {
    throw Error(
      Buf() << "Usage: " << argv[0] <<
        " <features-file> <labels-file> <label-id>"
    );
  }
  featuresFile = argv[1];
  labelsFile = argv[2];
  label = argv[3];
}


}}

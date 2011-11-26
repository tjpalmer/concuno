#ifndef concuno_run_args_h
#define concuno_run_args_h


#include <concuno.h>


namespace concuno {namespace run {


/**
 * Results of parsing command-line arguments, or in other words, the settings
 * for the concuno run.
 */
struct Args {

  /**
   * Convenience for parsing at construction.
   */
  Args(int argc, char** argv);

  void parse(int argc, char** argv);

  /**
   * The file to use for loading bags and entity feature vectors.
   */
  std::string featuresFile;

  /**
   * The label to use for the run.
   */
  std::string label;

  /**
   * The file to use for which bags are marked by which labels.
   */
  std::string labelsFile;

};


}}


#endif

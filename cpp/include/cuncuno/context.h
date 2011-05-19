#ifndef cuncuno_context_h
#define cuncuno_context_h

#include "export.h"
#include <string>

namespace cuncuno {

/**
 * Provides access to logging and system parameters.
 *
 * TODO Provide a name for each context?
 */
struct Context {

  /**
   * There's a default global context for cases without an override.
   */
  static Context& global();

  /**
   * TODO Invent a separate logger type, and defer to it. Or something.
   */
  void log(const std::string& message);

};

/**
 * A base class for objects that do much calculation. It provides convenient
 * access to a customizable context and a name for the worker.
 *
 * Note that if this Worker type is too heavyweight for some needs, you can
 * still access the global context or provide some other access to a local one
 * that works for the needs at hand.
 */
struct Worker {

  Worker(const std::string& name);

  /**
   * This worker's local context or, if that's not set, then the global default.
   */
  Context& context();

  /**
   * Just accesses the loggin functionality of the active context. Convenient
   * logging is very important for avoiding fallback to cout/printf.
   *
   * TODO Introduce error log level.
   * TODO Introduce verbose log level or is default considered verbose?
   * TODO Provide stream-like log access, or just figure folks can do that on
   * TODO their own with stringstream? I want convenience but not overkill. Hmm.
   */
  void log(const std::string& message);

  /**
   * Set this context if you want to avoid the global.
   */
  Context* localContext;

  /**
   * The name of this worker. It could used for identification in output, for
   * log filtering, and so on.
   */
  std::string name;

};

}

#endif

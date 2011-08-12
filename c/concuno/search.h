#ifndef concuno_learn_h
#define concuno_learn_h


#include "core.h"


/**
 * A search option is a state in a forward graph search process. It's entirely
 * abstract, managed by the client.
 */
typedef void* cnSearchOption;


/**
 * Implements forward graph search in nonmetric spaces by chasing higher
 * priority ("better") options first.
 *
 * Actual searchers extend this structure, so don't presume this is everything.
 *
 * TODO Consider that RRT-inspired Monte Carlo search technique I saw at AAAI
 * TODO 2011? Consider making algorithms pluggable at some point?
 */
typedef struct cnSearcher* cnSearcher;

struct cnSearcher {

  /**
   * The best option found so far during the search process.
   *
   * TODO Destroyed with the searcher?
   *
   * TODO A list of bests? How to define?
   */
  cnSearchOption bestOption;

  /**
   * Compares options to know which to consider first. The option priorities
   * should not change dynamically. Return true to say that a is better than b.
   *
   * This function must be thread safe. For example, the relevant data should
   * be fixed before ever providing it to the searcher.
   *
   * TODO Actual bounds for B&B-type searches?
   *
   * TODO Support dynamic reordering? It would be expensive, and would need to
   * TODO be explicit, but some needs might warrant it.
   */
  cnBool (*better)(cnSearcher searcher, cnSearchOption a, cnSearchOption b);

  /**
   * Set to non-null for automatic destruction of info.
   *
   * Destroy must ignore nulls.
   */
  void (*destroyInfo)(cnSearcher searcher);

  /**
   * Set to non-null for automatic destruction of options.
   *
   * Destroy must ignore nulls.
   */
  void (*destroyOption)(cnSearcher searcher, cnSearchOption option);

  /**
   * Says when the search is finished.
   *
   * If null, the search only terminates when all the options run out.
   */
  cnBool (*finished)(cnSearcher searcher);

  /**
   * Custom info for your own needs.
   */
  void *info;

  /**
   * The initial options from which to begin the search. At least one is needed.
   */
  cnList(cnSearchOption) initialOptions;

  /**
   * Given the current option, advance the search, providing the next options to
   * consider.
   *
   * The step function must be thread safe when using threaded search.
   */
  cnBool (*step)(
    cnSearcher searcher, cnSearchOption option, cnList(cnSearchOption)* nexts
  );

};


cnSearcher cnSearcherCreate(void);


void cnSearcherDestroy(cnSearcher searcher);


/**
 * Searches until finished. Returns true for no error. The best option is
 * available inside the searcher.
 */
cnBool cnSearch(cnSearcher searcher);


#endif

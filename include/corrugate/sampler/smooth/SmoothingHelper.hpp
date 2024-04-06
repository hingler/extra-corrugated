#ifndef SMOOTHING_HELPER_H_
#define SMOOTHING_HELPER_H_

namespace cg {
  // write as vir - specialize as concrete type
  // (note: smoothing will prob be handled similarly)

  // trouble with grad smoothing: because of how smooth works, we might end up with massive discontinuity
  // (ie: down slope gets smoothed harshly, flat area gets smoothed not-at-all - end up just shifting the impasse if delta is large)

  // alt2 for smooth: normal-smooth, then grad-smooth (if above threshold)
  // (issue2: discontinuities on chunk borders)
  // - could just look up points along edge :) [im fine w that]
  //
  // pull height from cache if avail - otherwise, look up

  // so, the goal:
  // - two smooth-components:
  //   - smoothing factor
  //   - desired max gradient
  //
  // alt3: this is more complicated than it needs to be.
  // - plan for changes - but for now: just treat it as an absolute smoothing val
  class SmoothingHelper {

  };
}

#endif // SMOOTHING_HELPER_H_

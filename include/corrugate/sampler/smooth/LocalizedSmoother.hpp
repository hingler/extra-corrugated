#ifndef EC_LOCALIZED_SMOOTHER_H_
#define EC_LOCALIZED_SMOOTHER_H_

#include "corrugate/sampler/smooth/impl/RecursiveSmoothSampler.hpp"
#include "corrugate/sampler/smooth/impl/SubSmoother.hpp"
#include <glm/glm.hpp>
#include <memory>

namespace cg {
  namespace smooth {
    // maintains several sub-smoothers which each smooth relative to some local point
    template <typename HeightType, typename SDFType>
    class LocalizedSmoother {
      typedef RecursiveSmoothSampler<HeightType, LocalizedSmoother<HeightType, SDFType>> recursive_height_type;
      typedef impl::SubSmoother<recursive_height_type, SDFType> sub_type;
     public:
      LocalizedSmoother(
        const std::shared_ptr<HeightType>& height,
        const glm::dvec2& global_origin
      ) : height(height), global_origin(global_origin) {}

      double GetDelta(
        double x,
        double y,
        double underlying
      ) const {
        double net_delta = 0.0;

        // smoothing based on whats underneath => each smoother is smoothing the output of the last
        for (int i = 0; i < subsmoothers.size(); i++) {
          double smooth_delta = subsmoothers.at(i)->Smooth(x, y, underlying + net_delta);
          net_delta += smooth_delta;
        }

        return net_delta;
      }

      // (pass in aabb ourselves hehe)
      void AddSubSmoother(
        const std::shared_ptr<SDFType>& sdf,
        const glm::dvec2& start,
        const glm::dvec2& end,
        double fade,
        double max_slope
      ) {
        // (tba: want a separate smoother, just for the green, to flatten it out a bit further)

        auto underlying = std::make_shared<
          recursive_height_type
        >(*this, height, global_origin);

        // pass down copy of contents thus far - smooth relative to underlying smoothers

        subsmoothers.push_back(
          std::make_shared<sub_type>(
            underlying, sdf, global_origin, start, end, fade, max_slope
          )
        );
      }

     private:
      std::shared_ptr<HeightType> height;
      std::vector<std::shared_ptr<sub_type>> subsmoothers;

      glm::dvec2 global_origin;
    };
  }
}

#endif // EC_LOCALIZED_SMOOTHER_H_

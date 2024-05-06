#ifndef EC_LOCALIZED_SMOOTHER_H_
#define EC_LOCALIZED_SMOOTHER_H_

#include "corrugate/sampler/smooth/impl/SubSmoother.hpp"
#include <glm/glm.hpp>
#include <memory>

namespace cg {
  namespace smooth {
    // maintains several sub-smoothers which each smooth relative to some local point
    template <typename HeightType, typename SDFType>
    class LocalizedSmoother {
      typedef impl::SubSmoother<HeightType, SDFType> sub_type;
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
        double falloff_sum = 0.0;
        double net_delta = 0.0;

        for (const std::shared_ptr<sub_type>& smoother : subsmoothers) {
          double falloff = smoother->GetFalloffWeight(x, y);
          falloff_sum += falloff;
          net_delta += smoother->Smooth(x, y, underlying);
        }

        if (falloff_sum > 1.0) {
          net_delta *= (1.0 / falloff_sum);
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
        subsmoothers.push_back(
          std::make_shared<sub_type>(
            height, sdf, global_origin, start, end, fade, max_slope
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

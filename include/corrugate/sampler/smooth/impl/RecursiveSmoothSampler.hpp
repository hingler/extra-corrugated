#ifndef EC_RECURSIVE_SMOOTH_SAMPLER_H_
#define EC_RECURSIVE_SMOOTH_SAMPLER_H_

#include <memory>
#include <glm/glm.hpp>

namespace cg {
  namespace smooth {


    template <typename HeightType, typename SmootherType>
    class RecursiveSmoothSampler {
     public:
      RecursiveSmoothSampler(
        const SmootherType& smoother,
        const std::shared_ptr<HeightType>& base_height,
        const glm::dvec2& global_origin
      ) : underlying_smoother(smoother), base_height(base_height), global_origin(global_origin) {}

      double Sample(double x, double y) {
        // (note: x and y are in global coordinates)
        double underlying = base_height->Sample(x, y);

        // for smoothing: need to coerce back to box-local
        double smooth = underlying_smoother.GetDelta(x - global_origin.x, y - global_origin.y, underlying);

        return underlying + smooth;
      }

     private:
      SmootherType underlying_smoother;
      std::shared_ptr<HeightType> base_height;
      glm::dvec2 global_origin;
    };
  }
}

#endif // EC_RECURSIVE_SMOOTH_SAMPLER_H_

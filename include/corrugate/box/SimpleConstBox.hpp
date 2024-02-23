#ifndef SIMPLE_CONST_BOX_H_
#define SIMPLE_CONST_BOX_H_

#include "corrugate/box/BaseTerrainBox.hpp"

// simple instantiation
namespace cg {
  namespace _impl {
    struct ConstSampler {
      ConstSampler(float in) : val(in) {

      }

      float Sample(double x, double y) {
        return val;
      }

      glm::vec4 Sample(double x, double y, size_t index) {
        return glm::vec4(val);
      }

      float val;
    };
  }
  class SimpleConstBox : public BaseTerrainBox {
   public:
    SimpleConstBox(const glm::dvec2& origin, const glm::dvec2& size) :
    BaseTerrainBox(
      origin,
      size,
      std::make_shared<_impl::ConstSampler>(1.0f),
      std::make_shared<_impl::ConstSampler>(0.0f),
      std::make_shared<_impl::ConstSampler>(1.0f),
      1.0f,
      1.0f
    ), SamplerBox(origin, size, 1.0f, 1.0f) {}

    // how do we want to handle tree fill?
    // - logic would "dictate" that we want to "carve it out"
    //   - ie: the default is something large, and we specify low values for our course to remove it
    //   - continuity doesn't matter here as much - we could just use the min of base and smoothing.
  };
}

#endif // SIMPLE_CONST_BOX_H_

#ifndef SMOOTHING_TERRAIN_BOX_H_
#define SMOOTHING_TERRAIN_BOX_H_

#include "corrugate/box/BaseTerrainBox.hpp"
#include "corrugate/sampler/SmoothingTerrainSampler.hpp"
#include "corrugate/box/BaseSmoothingSamplerBox.hpp"

#include "corrugate/sampler/DataSampler.hpp"

namespace cg {
  // extend baseterrain
  // behavior is the same, just want to be able to get a smoothing delta
  // at the end:
  // - avg smoothing deltas based on falloff
  // - add height change (which should already take falloff into account)

  // interface extends samplerbox, or is its own thing?
  // - contents need to be boxes...
  // - ...and contents need to be smoothing.
  // - ...so we'd need
  template <typename BaseType>
  class SmoothingTerrainBox : public BaseTerrainBox, public BaseSmoothingSamplerBox {
   public:
   template <typename HeightType, typename SplatType, typename FillType>
    SmoothingTerrainBox(
      const glm::dvec2& origin,
      const glm::dvec2& size,
      std::shared_ptr<HeightType> heightmap,
      std::shared_ptr<SplatType> splat,
      std::shared_ptr<FillType> fill,
      std::shared_ptr<BaseType> underlying,
      float falloff_radius,
      float falloff_dist,
      float smoothing_factor
    ) :
    BaseTerrainBox(origin, size, heightmap, splat, fill, falloff_radius, falloff_dist),
    BaseSmoothingSamplerBox(origin, size, falloff_radius, falloff_dist),
    SamplerBox(origin, size, falloff_radius, falloff_dist),   // v base class ctor
    smoother(underlying, *this),
    smoothing_factor(smoothing_factor) {
      smoother.smoothing_factor = smoothing_factor;
    }

    // this is handled before falloff!
    // ergo: we could work with linear values all the way
    float GetSmoothDelta(double x, double y, double underlying) const override {
      // do we want to return vanilla values
      glm::dvec2 origin = GetOrigin();
      return smoother.Smooth(underlying) * GetFalloffWeight_local(glm::dvec2(x - origin.x, y - origin.y));
    }

    size_t WriteSmoothDelta(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
      const DataSampler<float>& underlying_data,
      const DataSampler<float>& falloff_sums,
      float* output,
      size_t n_bytes
    ) const override {
      size_t required_bytes = sample_dims.x * sample_dims.y * sizeof(float);
      if (required_bytes > n_bytes) {
        return 0;
      }

      // should we be applying falloff values here, or later?
      // - i was under the impression that we were lol

      // sum all falloffs (1)
      // sample smooth (2) and divide by falloff (3)

      assert(underlying_data.data_size.x >= sample_dims.x);
      assert(underlying_data.data_size.y >= sample_dims.y);


      glm::dvec2 local_origin = origin - GetOrigin();
      glm::dvec2 local_coord;

      // falloff sum is a weighted average
      // after falloff: scale the whole thing by "falloff / falloff sum"

      for (int y = 0; y < sample_dims.y; y++) {
        local_coord.y = local_origin.y + static_cast<double>(y) * scale;
        for (int x = 0; x < sample_dims.x; x++) {
          local_coord.x = local_origin.x + static_cast<double>(x) * scale;
          float falloff = GetFalloffWeight_local(local_coord);
          float falloff_sum = std::max(falloff_sums.Get(x, y), 0.00001f);
          output[y * sample_dims.x + x] = smoother.Smooth(underlying_data.Get(x, y)) * falloff * (falloff / falloff_sum);
        }
      }

      return required_bytes;
    }

   private:
    const float smoothing_factor;
    SmoothingTerrainSampler<BaseType> smoother;

    // now:
    // sample+falloff
    // smoothing+falloff is handled "underneath"
  };
}

#endif // SMOOTHING_TERRAIN_BOX_H_

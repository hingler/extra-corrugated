#ifndef SMOOTHING_TERRAIN_BOX_H_
#define SMOOTHING_TERRAIN_BOX_H_

#include "corrugate/box/BaseTerrainBox.hpp"
#include "corrugate/box/SimpleConstBox.hpp"
#include "corrugate/sampler/SmoothingTerrainSampler.hpp"
#include "corrugate/box/BaseSmoothingSamplerBox.hpp"

#include "corrugate/sampler/DataSampler.hpp"
#include "gog43/Logger.hpp"

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
  class SmoothingTerrainBox : public BaseTerrainBox, public BaseSmoothingSamplerBox {
   public:
    template <typename HeightType, typename SplatType, typename FillType, typename GrassFillType>
    SmoothingTerrainBox(
      const cg::FeatureBox& box,
      std::shared_ptr<HeightType> heightmap,
      std::shared_ptr<SplatType> splat,
      std::shared_ptr<FillType> fill,
      const std::shared_ptr<GrassFillType>& grass_fill,
      float smoothing_factor
    ) : SmoothingTerrainBox(
      box,
      heightmap,
      splat,
      fill,
      grass_fill,
      std::make_shared<_impl::ConstSampler>(smoothing_factor)
    ) {};

    template <typename HeightType, typename SplatType, typename FillType, typename GrassFillType, typename SmoothType>
    SmoothingTerrainBox(
      const cg::FeatureBox& box,
      const std::shared_ptr<HeightType>& heightmap,
      const std::shared_ptr<SplatType>& splat,
      const std::shared_ptr<FillType>& fill,
      const std::shared_ptr<GrassFillType>& grass_fill,
      const std::shared_ptr<SmoothType>& smooth
    ) : SmoothingTerrainBox(
      box.GetOrigin(),
      box.GetSize(),
      heightmap,
      splat,
      fill,
      grass_fill,
      box.falloff_radius,
      box.falloff_size,
      smooth
    ) {}


    template <typename HeightType, typename SplatType, typename FillType, typename GrassFillType, typename SmoothType>
    SmoothingTerrainBox(
      const glm::dvec2& origin,
      const glm::dvec2& size,
      std::shared_ptr<HeightType> heightmap,
      std::shared_ptr<SplatType> splat,
      std::shared_ptr<FillType> fill,
      const std::shared_ptr<GrassFillType>& grass_fill,
      float falloff_radius,
      float falloff_dist,
      const std::shared_ptr<SmoothType>& smooth
    ) :
    BaseTerrainBox(origin, size, heightmap, splat, fill, grass_fill, falloff_radius, falloff_dist),
    BaseSmoothingSamplerBox(origin, size, falloff_radius, falloff_dist),
    SamplerBox(origin, size, falloff_radius, falloff_dist),   // v base class ctor
    smoother(*this, smooth),
    smoothing_factor(0.0) {}

    template <typename BaseType>
    void PrepareCache(const std::shared_ptr<BaseType>& sampler) {
      smoother.PrepareCache(sampler);
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
          if (Contains_Local(local_coord)) {
            float falloff = GetFalloffWeight_local(local_coord);
            float falloff_sum = std::max(falloff_sums.Get(x, y), 0.00001f);
            output[y * sample_dims.x + x] = smoother.Smooth(underlying_data.Get(x, y)) * falloff * (falloff / falloff_sum);
          } else {
            output[y * sample_dims.x + x] = 0.0f;
          }
        }
      }

      return required_bytes;
    }

   private:
    const float smoothing_factor;
    SmoothingTerrainSampler smoother;

    // now:
    // sample+falloff
    // smoothing+falloff is handled "underneath"
  };
}

#endif // SMOOTHING_TERRAIN_BOX_H_

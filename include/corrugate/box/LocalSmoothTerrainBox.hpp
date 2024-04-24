#ifndef EC_LOCAL_SMOOTH_TERRAIN_BOX_H_
#define EC_LOCAL_SMOOTH_TERRAIN_BOX_H_

#include "corrugate/box/BaseSmoothingSamplerBox.hpp"
#include "corrugate/box/BaseTerrainBox.hpp"
#include "corrugate/sampler/smooth/LocalizedSmoother.hpp"
namespace cg {
  // p sure this template doesn't matter hehe - we coerce to virt type anyway
  template <typename BaseType, typename SmootherSDF>
  class LocalSmoothTerrainBox : public BaseTerrainBox, public BaseSmoothingSamplerBox {
   public:
    template <
      typename HeightType,
      typename SplatType,
      typename FillType,
      typename GrassFillType
    >
    LocalSmoothTerrainBox(
      const cg::FeatureBox& box,
      const std::shared_ptr<HeightType>& heightmap,
      const std::shared_ptr<SplatType>& splat,
      const std::shared_ptr<FillType>& fill,
      const std::shared_ptr<GrassFillType>& grass_fill,
      const std::shared_ptr<smooth::LocalizedSmoother<BaseType, SmootherSDF>>& smoother
    ) : BaseTerrainBox(
      box.GetOrigin(),
      box.GetSize(),
      heightmap,
      splat,
      fill,
      grass_fill,
      box.falloff_radius,
      box.falloff_size
    ), BaseSmoothingSamplerBox(
      box.GetOrigin(), box.GetSize(), box.falloff_radius, box.falloff_size
    ), SamplerBox(
      box.GetOrigin(), box.GetSize(), box.falloff_radius, box.falloff_size
    ), smoother(smoother) {}

    float GetSmoothDelta(double x, double y, double underlying) const override {
      glm::dvec2 local = glm::dvec2(x, y) - GetOrigin();
      float falloff = GetFalloffWeight_local(local);
      return static_cast<float>(smoother->GetDelta(local.x, local.y, underlying)) * falloff;
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

      for (int y = 0; y < sample_dims.y; y++) {
        local_coord.y = local_origin.y + static_cast<double>(y) * scale;
        for (int x = 0; x < sample_dims.x; x++) {
          local_coord.x = local_origin.x + static_cast<double>(x) * scale;
          if (Contains_Local(local_coord)) {
            float falloff = GetFalloffWeight_local(local_coord);
            float falloff_sum = std::max(falloff_sums.Get(x, y), 0.0001f);
            output[y * sample_dims.x + x] =
              GetSmoothDelta(
                local_coord.x,
                local_coord.y,
                underlying_data.Get(x, y)
              );
          }
        }

      }

      return required_bytes;
    }

   private:
    std::shared_ptr<smooth::LocalizedSmoother<BaseType, SmootherSDF>> smoother;
  };
}

#endif // EC_LOCAL_SMOOTH_TERRAIN_BOX_H_

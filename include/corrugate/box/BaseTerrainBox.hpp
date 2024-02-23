#ifndef BASE_TERRAIN_BOX_H_
#define BASE_TERRAIN_BOX_H_

#include "corrugate/box/SamplerBox.hpp"
#include "corrugate/sampler/BaseTerrainSampler.hpp"

namespace cg {
  // inheritance tree
  // smoothing type will inherit samplerbox and some smoothing functionality

  class BaseTerrainBox : virtual public SamplerBox {
   public:
    template <typename HeightType, typename SplatType, typename FillType>
    BaseTerrainBox(
      const glm::vec2& origin,
      const glm::vec2& size,
      std::shared_ptr<HeightType> heightmap,
      std::shared_ptr<SplatType> splat,
      std::shared_ptr<FillType> fill,
      float falloff_radius,
      float falloff_dist
    ) : SamplerBox(origin, size, falloff_radius, falloff_dist),
        sampler(heightmap, splat, fill) {}


    float SampleHeight(double x, double y)                   const override {
      // tba: need to handle falloff in all of these
      auto origin = GetOrigin();
      glm::dvec2 local_coord(x - origin.x, y - origin.y);

      float falloff_weight = GetFalloffWeight_local(local_coord);
      return sampler.SampleHeight(local_coord.x, local_coord.y) * falloff_weight;
    };

    glm::vec4 SampleSplat(double x, double y, size_t index)     const override {
      auto origin = GetOrigin();
      glm::dvec2 local_coord(x - origin.x, y - origin.y);

      float falloff_weight = GetFalloffWeight_local(local_coord);
      return sampler.SampleSplat(local_coord.x, local_coord.y, index) * falloff_weight;
    };

    float SampleTreeFill( double x, double y)                   const override {
      auto origin = GetOrigin();
      glm::dvec2 local_coord(x - origin.x, y - origin.y);

      float falloff_weight = GetFalloffWeight_local(local_coord);
      return sampler.SampleTreeFill(local_coord.x, local_coord.y) * falloff_weight;
    };


    size_t WriteHeight(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
      float* output,
      size_t n_bytes
    ) const override {
      // get sampling origin relative
      glm::dvec2 origin_relative = origin - GetOrigin();

      // tba: we can def skip negative samples

      size_t bytes_written = sampler.WriteHeight(origin_relative, sample_dims, scale, output, n_bytes);
      size_t elements_written = bytes_written / sizeof(float);

      ApplyFalloff<float>(origin_relative, sample_dims, scale, output, elements_written, nullptr);

      return bytes_written;
    };

    size_t WriteSplat(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
      size_t index,
      glm::vec4* output,
      size_t n_bytes,
      const DataSampler<float>* falloffs
    ) const override {
      glm::dvec2 origin_relative = origin - GetOrigin();
      size_t bytes_written = sampler.WriteSplat(origin_relative, sample_dims,scale, index, output, n_bytes);
      size_t elements_written = bytes_written / sizeof(glm::vec4);

      ApplyFalloff<glm::vec4>(origin_relative, sample_dims, scale, output, elements_written, falloffs);
      return bytes_written;
    };

    size_t WriteTreeFill(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
      float* output,
      size_t n_bytes,
      const DataSampler<float>* falloffs
    ) const override {
      glm::dvec2 origin_relative = origin - GetOrigin();
      size_t bytes_written = sampler.WriteTreeFill(origin_relative, sample_dims, scale, output, n_bytes);
      size_t elements_written = bytes_written / sizeof(float);

      ApplyFalloff<float>(origin_relative, sample_dims, scale, output, elements_written, falloffs);
      return bytes_written;
    };

   private:
    BaseTerrainSampler sampler;

    // apply falloff to generic data type?
    template <typename FalloffDataType>
    void ApplyFalloff(const glm::dvec2& origin_relative, const glm::ivec2& sample_dims, const chunker::util::Fraction& scale, FalloffDataType* output, size_t n_elements, const DataSampler<float>* falloffs) const {
      // specify origin in local coords
      size_t cur = 0;

      glm::dvec2 local_coord;

      for (int y = 0; y < sample_dims.y; y++) {
        local_coord.y = static_cast<double>(y) * scale.AsDouble() + origin_relative.y;
        for (int x = 0; x < sample_dims.x; x++) {
          if (++cur > n_elements) {

            return;
          }

          local_coord.x = static_cast<double>(x) * scale.AsDouble() + origin_relative.x;
          // also apply falloff ptr

          float falloff_weight = GetFalloffWeight_local(local_coord);
          float falloff_fract = (falloffs != nullptr) ? (falloff_weight / std::max(falloffs->Get(x, y), 0.00001f)) : 1.0;

          // multiply by falloff weight, then scale based on pct of total
          output[y * sample_dims.x + x] *= falloff_weight * falloff_fract;
        }
      }
    }
  };
}

#endif // BASE_TERRAIN_BOX_H_

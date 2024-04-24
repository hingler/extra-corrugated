#ifndef BASE_TERRAIN_BOX_H_
#define BASE_TERRAIN_BOX_H_

#include "corrugate/box/SamplerBox.hpp"
#include "corrugate/sampler/BaseTerrainSampler.hpp"
#include <functional>

namespace cg {
  // inheritance tree
  // smoothing type will inherit samplerbox and some smoothing functionality

  class BaseTerrainBox : virtual public SamplerBox {
   public:
    template <typename HeightType, typename SplatType, typename FillType, typename GrassType>
    BaseTerrainBox(
      const glm::vec2& origin,
      const glm::vec2& size,
      std::shared_ptr<HeightType> heightmap,
      std::shared_ptr<SplatType> splat,
      std::shared_ptr<FillType> fill,
      const std::shared_ptr<GrassType>& grass,
      float falloff_radius,
      float falloff_dist
    ) : SamplerBox(origin, size, falloff_radius, falloff_dist),
        sampler(heightmap, splat, fill, grass, size) {}


    float SampleHeight(double x, double y)                   const override {
      return SampleFloatGeneric(x, y, &BaseTerrainSampler::SampleHeight);
    };

    glm::vec4 SampleSplat(double x, double y, size_t index)     const override {
      auto origin = GetOrigin();
      glm::dvec2 local_coord(x - origin.x, y - origin.y);

      if (Contains_Local(local_coord)) {
        float falloff_weight = GetFalloffWeight_local(local_coord);
        return sampler.SampleSplat(local_coord.x, local_coord.y, index) * falloff_weight;
      }

      return glm::vec4(0.0);
    };

    float SampleTreeFill( double x, double y)                   const override {
      return SampleFloatGeneric(x, y, &BaseTerrainSampler::SampleTreeFill);
    };

    float SampleGrassFill(double x, double y) const override {
      return SampleFloatGeneric(x, y, &BaseTerrainSampler::SampleGrassFill);
    }

    size_t WriteHeight(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
      float* output,
      size_t n_bytes
    ) const override {
      // get sampling origin relative
      glm::dvec2 origin_relative = origin - GetOrigin();

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
      size_t bytes_written = sampler.WriteSplat(origin_relative, sample_dims, scale, index, output, n_bytes);
      size_t elements_written = bytes_written / sizeof(glm::vec4);

      // test: don't apply falloff to splat data - think it's avg'ing
      ApplyFalloff<glm::vec4>(origin_relative, sample_dims, scale, output, elements_written, nullptr);
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
      return WriteFloatGeneric(
        origin, sample_dims, scale, output, n_bytes, &BaseTerrainSampler::WriteTreeFill
      );
    };

    size_t WriteGrassFill(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
      float* output,
      size_t n_bytes
    ) const override {
      return WriteFloatGeneric(
        origin, sample_dims, scale, output, n_bytes, &BaseTerrainSampler::WriteGrassFill
      );
    }

   private:
    BaseTerrainSampler sampler;

    typedef float(BaseTerrainSampler::*sample_fnptr)(double, double) const;
    typedef size_t(BaseTerrainSampler::*write_fnptr)(
      const glm::dvec2&,
      const glm::ivec2&,
      double,
      float*,
      size_t
    ) const;

    float SampleFloatGeneric(
      double x, double y, sample_fnptr samplerPointer
    ) const {
      auto origin = GetOrigin();
      glm::dvec2 local_coord(x - origin.x, y - origin.y);

      if (Contains_Local(local_coord)) {
        float falloff_weight = GetFalloffWeight_local(local_coord);
        return (sampler.*samplerPointer)(local_coord.x, local_coord.y) * falloff_weight;
      }

      return 0.0f;
    }

    size_t WriteFloatGeneric(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
      float* output,
      size_t n_bytes,
      write_fnptr writerPointer,
      const DataSampler<float>* falloffs = nullptr
    ) const {
      glm::dvec2 origin_relative = origin - GetOrigin();
      size_t bytes_written = (sampler.*writerPointer)(origin_relative, sample_dims, scale, output, n_bytes);
      size_t elements_written = bytes_written / sizeof(float);

      ApplyFalloff<float>(origin_relative, sample_dims, scale, output, elements_written, falloffs);
      return bytes_written;
    }

    // apply falloff to generic data type?
    template <typename FalloffDataType>
    void ApplyFalloff(const glm::dvec2& origin_relative, const glm::ivec2& sample_dims, const double scale, FalloffDataType* output, size_t n_elements, const DataSampler<float>* falloffs) const {
      // specify origin in local coords
      size_t cur = 0;

      glm::dvec2 local_coord;

      for (int y = 0; y < sample_dims.y; y++) {
        local_coord.y = static_cast<double>(y) * scale + origin_relative.y;
        for (int x = 0; x < sample_dims.x; x++) {
          if (++cur > n_elements) {

            return;
          }

          local_coord.x = static_cast<double>(x) * scale + origin_relative.x;
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

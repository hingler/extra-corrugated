#ifndef VARIABLE_SMOOTHING_TERRAIN_SAMPLER_H_
#define VARIABLE_SMOOTHING_TERRAIN_SAMPLER_H_

#include "corrugate/FeatureBox.hpp"
#include "corrugate/sampler/DataSampler.hpp"
#include "corrugate/sampler/SampleWriterGeneric.hpp"

#include <memory>

namespace cg {
  // like terrain sampler, geared towards catching discontinuities in terrain
  class VariableSmoothingTerrainSampler {
   public:
    template <typename BaseType, typename SmoothType>
    VariableSmoothingTerrainSampler(
      const FeatureBox& box,
      const std::shared_ptr<BaseType>& base,
      const std::shared_ptr<SmoothType>& smooth
    ) : height_sampler(
      std::make_unique<SampleWriterGenericImpl<float, BaseType>>(
        base,
        box.GetSize()
      )
    ), box(box) {}

    // outputs smoothed contents at sample point
    size_t SmoothSample(
      double x, double y, float underlying
    );

    // outputs smoothed contents of underlying chunk
    size_t SmoothChunk(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
      const DataSampler<float>& underlying,
      float* output,
      size_t n_bytes
    );
   private:
    // issue: height sampler is global, but smooth sampler is local
    // - provide coordinates in global
    // - use `box` to convert to local
    // - same rules - if OOB, then ignore (prob)
    // (aside: also want to do sand pits)
    // (aside2: also want to shuffle up the terrain!!!)
    std::unique_ptr<SampleWriterGeneric<float>> height_sampler;
    std::unique_ptr<SampleWriterGeneric<float>> smooth_sampler;
    FeatureBox box;
  };
}

#endif // VARIABLE_SMOOTHING_TERRAIN_SAMPLER_H_

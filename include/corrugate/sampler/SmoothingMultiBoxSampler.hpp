#ifndef SMOOTHING_MULTI_TERRAIN_SAMPLER_H_
#define SMOOTHING_MULTI_TERRAIN_SAMPLER_H_

#include "corrugate/sampler/MultiBoxSampler.hpp"
#include "corrugate/box/BaseSmoothingSamplerBox.hpp"

#include <algorithm>

namespace cg {
  template <typename SmoothingBoxType>
  class SmoothingMultiBoxSampler {

    static_assert(std::is_base_of_v<BaseSmoothingSamplerBox, SmoothingBoxType>);
    // same logic
    // - height function: need to add the smoothing values at the end
    // the rest are the same
   public:
    template <typename IterableType>
    SmoothingMultiBoxSampler(const IterableType& contents) : samplers(contents.begin(), contents.end()), wrap(samplers) {}

    // how does this end up working for samples??
    // - if we just wrap the underlying component, it would be easy
    // - i guess in either case, we're doing the same amount of work:
    // - either this does the fetch, or someone else does
    // stitch the two together at the end
    float SampleHeight(double x, double y, double underlying) const {
      float acc = 0.0f;
      acc += wrap.SampleHeight(x, y);

      float smooth_acc = 0.0f;

      float falloffs[samplers.size()];
      float falloff_sum = 0.0f;
      for (size_t i = 0; i < samplers.size(); i++) {
        falloffs[i] = samplers[i]->GetFalloffWeight(x, y);
        falloff_sum += falloffs[i];
      }

      falloff_sum = std::max(falloff_sum, 0.000001f);

      for (size_t i = 0; i < samplers.size(); i++) {
        acc += samplers[i]->GetSmoothDelta(x, y, underlying) * (falloffs[i] / falloff_sum);
      }

      return acc;
    }

    glm::vec4 SampleSplat(double x, double y, size_t index) const {
      return wrap.SampleSplat(x, y, index);
    }

    float SampleTreeFill(double x, double y) const {
      return wrap.SampleTreeFill(x, y);
    }

    size_t WriteHeight(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      const chunker::util::Fraction& scale,
      const DataSampler<float>& underlying,

      float* output,
      size_t n_bytes
    ) const {
      size_t elems = sample_dims.x * sample_dims.y;
      size_t bytes = elems * sizeof(float);
      if (bytes > n_bytes) {
        return 0;
      }

      float* falloffs = new float[sample_dims.x * sample_dims.y];
      float* temp = new float[sample_dims.x * sample_dims.y];
      wrap.WriteFalloffSum(origin, sample_dims, scale, falloffs, bytes);

      // first: write height

      // wondering: is there a way to avoid these per-chunk allocs?
      // - typically, all chunks are the same size - pre-alloc untyped workspace
      // - swap out as we perform larger operations
      wrap.WriteHeight(origin, sample_dims, scale, temp, bytes);
      memcpy(output, temp, bytes);
      // next: add to output

      DataSampler<float> falloff_sums(sample_dims, falloffs);
      for (size_t i = 0; i < samplers.size(); i++) {
        // write weighted smoothing to temp
        samplers[i]->WriteSmoothDelta(origin, sample_dims, scale, underlying, falloff_sums, temp, bytes);
        for (size_t c = 0; c < elems; c++) {
          // add delta to output
          output[c] += temp[c];
        }
      }

      return bytes;
    }

    size_t WriteSplat(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      const chunker::util::Fraction& scale,
      size_t index,
      glm::vec4* output,
      size_t n_bytes
    ) const {
      return wrap.WriteSplat(origin, sample_dims, scale, index, output, n_bytes);
    }

    size_t WriteTreeFill(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      const chunker::util::Fraction& scale,
      float* output,
      size_t n_bytes
    ) const {
      return wrap.WriteTreeFill(origin, sample_dims, scale, output, n_bytes);
    }
   private:
    std::vector<std::shared_ptr<const SmoothingBoxType>> samplers;
    MultiBoxSampler<SmoothingBoxType> wrap;
  };
}

#endif // SMOOTHING_MULTI_TERRAIN_SAMPLER_H_

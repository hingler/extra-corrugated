#ifndef SMOOTHING_MULTI_TERRAIN_SAMPLER_H_
#define SMOOTHING_MULTI_TERRAIN_SAMPLER_H_

#include "corrugate/sampler/MultiBoxSampler.hpp"
#include "corrugate/box/BaseSmoothingSamplerBox.hpp"
#include "gog43/Logger.hpp"

#include <algorithm>

// correct type! nvm :-)

namespace cg {
  template <typename SmoothingBoxType>
  class SmoothingMultiBoxSampler {

    static_assert(std::is_base_of_v<BaseSmoothingSamplerBox, SmoothingBoxType>);
    // same logic
    // - height function: need to add the smoothing values at the end
    // the rest are the same
   public:
    template <typename IterableType>
    SmoothingMultiBoxSampler(const IterableType& contents) : samplers(contents.begin(), contents.end()), wrap(samplers) {
      // lengthy test - want to see something
      auto itr = contents.begin();
      while (itr != contents.end()) {
        auto itr_prev = itr++;
        if (std::find(itr, contents.end(), *itr_prev) != contents.end()) {
          gog43::print("SAMPLER: bug - found duplicate in incoming samplers!!!");
        }

        for (auto itr_sub = itr; itr_sub != contents.end(); itr_sub++) {
          if (
            glm::length((*itr_prev)->GetOrigin() - (*itr_sub)->GetOrigin()) < 0.0001
            && glm::length((*itr_prev)->GetSize() - (*itr_sub)->GetSize()) < 0.0001
            && itr_sub != itr_prev && (*itr_sub).get() != (*itr_prev).get()
          ) {
            // logged - probably a whole bunch of duplicate boxes!
            gog43::print("SAMPLER: potential duplicate box found!");
          }
        }
      }


    }

    // how does this end up working for samples??
    // - if we just wrap the underlying component, it would be easy
    // - i guess in either case, we're doing the same amount of work:
    // - either this does the fetch, or someone else does
    // stitch the two together at the end
    float GetFalloffWeight(double x, double y) const {
      return wrap.GetFalloffWeight(x, y);
    }

    float SampleHeight(double x, double y, double underlying) const {
      float acc = 0.0f;
      // acc += wrap.SampleHeight(x, y);

      float smooth_acc = 0.0f;

      float falloffs[samplers.size()];
      float falloff_sum = 0.0f;

      glm::dvec2 point(x, y);
      for (size_t i = 0; i < samplers.size(); i++) {
        if (samplers[i]->Contains(point)) {
          falloffs[i] = samplers[i]->GetFalloffWeight(x, y);
          falloff_sum += falloffs[i];
        } else {
          falloffs[i] = 0.0f;
        }
      }

      falloff_sum = std::max(falloff_sum, 0.000001f);

      for (size_t i = 0; i < samplers.size(); i++) {
        if (falloffs[i] > 0.00001f) {
          acc += samplers[i]->GetSmoothDelta(x, y, underlying);
        }
      }

      return acc;
    }

    glm::vec4 SampleSplat(double x, double y, size_t index) const {
      return wrap.SampleSplat(x, y, index);
    }

    float SampleTreeFill(double x, double y) const {
      return wrap.SampleTreeFill(x, y);
    }

    float SampleGrassFill(double x, double y) const {
      return wrap.SampleGrassFill(x, y);
    }

    size_t WriteHeight(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
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
      double scale,
      size_t index,
      glm::vec4* output,
      size_t n_bytes
    ) const {
      return wrap.WriteSplat(origin, sample_dims, scale, index, output, n_bytes);
    }

    size_t WriteTreeFill(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
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

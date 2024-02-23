#ifndef MULTI_BOX_SAMPLER_H_
#define MULTI_BOX_SAMPLER_H_

#include "corrugate/box/SamplerBox.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace cg {
  // collates samples from multiple boxes (better name)
  template <typename BoxType>
  class MultiBoxSampler {
    static_assert(std::is_base_of_v<SamplerBox, BoxType>);
   public:
    typedef std::vector<std::shared_ptr<const BoxType>> vector_type;

    // this kicks ass lol
    template <typename IterableType>
    MultiBoxSampler(const IterableType& contents) : samplers(contents.begin(), contents.end()) {}

    template <>
    MultiBoxSampler(const std::vector<std::shared_ptr<const BoxType>>& contents) : samplers(contents) {}

    float SampleHeight(double x, double y) const {
      float acc = 0.0f;
      for (auto& sampler : samplers) {
        acc += sampler->SampleHeight(x, y);
      }

      return acc;
    }
    glm::vec4 SampleSplat(double x, double y, size_t index) const {
      glm::vec4 acc(0.0f);
      float falloffs[samplers.size()];
      float falloff_sum = 0.0f;
      // not a vector (oops!)
      for (size_t i = 0; i < samplers.size(); i++) {
        falloffs[i] = samplers[i]->GetFalloffWeight(x, y);
        falloff_sum += falloffs[i];
      }

      for (size_t i = 0; i < samplers.size(); i++) {
        acc += samplers[i]->SampleSplat(x, y, index) * (falloffs[i] / falloff_sum);
      }

      return acc;
    }

    float SampleTreeFill(double x, double y) const {
      float acc = 0.0f;

      float falloffs[samplers.size()];
      float falloff_sum = 0.0f;
      for (size_t i = 0; i < samplers.size(); i++) {
        falloffs[i] = samplers[i]->GetFalloffWeight(x, y);
        falloff_sum += falloffs[i];
      }

      // how do we want to do this? probably another weighted average (tba)
      for (size_t i = 0; i < samplers.size(); i++) {
        acc += (samplers[i]->SampleTreeFill(x, y) * (falloffs[i] / falloff_sum));
      }

      return acc;
    }

    size_t WriteHeight(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      const chunker::util::Fraction& scale,
      float* output,
      size_t n_bytes
    ) const {
      size_t elems = sample_dims.x * sample_dims.y;
      size_t bytes = elems * sizeof(float);

      if (bytes > n_bytes) {
        return 0;
      }

      float* temp = new float[elems];
      memset(output, 0, bytes);
      // seems like for all of these, we want to average out using the same strat (except for height)
      for (auto& sampler : samplers) {
        size_t written = sampler->WriteHeight(
          origin,
          sample_dims,
          scale,
          temp,
          n_bytes
        );

        assert(written == bytes);

        for (size_t i = 0; i < elems; i++) {
          // accrue sampler values into output
          output[i] += temp[i];
        }
      }

      delete[] temp;
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
      size_t elems = sample_dims.x * sample_dims.y;
      size_t bytes = elems * sizeof(glm::vec4);
      if (bytes > n_bytes) {
        return 0;
      }

      glm::vec4* temp = new glm::vec4[elems];
      float* falloffs = new float[elems];
      memset(temp, 0, bytes);
      memset(output, 0, bytes);
      WriteFalloffSum(
        origin,
        sample_dims,
        scale,
        falloffs,
        elems * sizeof(float)
      );

      DataSampler<float> falloff_sampler(sample_dims, falloffs);

      for (auto& sampler : samplers) {
        sampler->WriteSplat(
          origin,
          sample_dims,
          scale,
          index,
          temp,
          n_bytes,
          &falloff_sampler
        );

        for (size_t i = 0; i < elems; i++) {
          output[i] += temp[i];
        }
      }

      delete[] temp;
      delete[] falloffs;

      return bytes;
    }

    size_t WriteTreeFill(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
      float* output,
      size_t n_bytes
    ) const {
      size_t elems = sample_dims.x * sample_dims.y;
      size_t bytes = elems * sizeof(float);

      if (bytes > n_bytes) {
        return 0;
      }

      float* temp = new float[elems];
      float* falloffs = new float[elems];
      memset(temp, 0, bytes);
      memset(output, 0, bytes);
      WriteFalloffSum(
        origin,
        sample_dims,
        scale,
        falloffs,
        bytes
      );

      DataSampler<float> falloff_sampler(sample_dims, falloffs);

      for (auto& sampler : samplers) {
        sampler->WriteTreeFill(
          origin,
          sample_dims,
          scale,
          temp,
          n_bytes,
          &falloff_sampler
        );

        for (size_t i = 0; i < elems; i++) {
          // accrue sampler values into output
          output[i] += temp[i];
        }
      }

      delete[] temp;
      delete[] falloffs;

      return bytes;
    }

    float SampleFalloffSum(const glm::dvec2& coords) const {
      float acc = 0.0f;
      for (auto& sampler : samplers) {
        acc += sampler->GetFalloffWeight(coords);
      }

      return acc;
    }

    // leave public so we can use it for smoothing :3
    // this is "done" - write test code
    size_t WriteFalloffSum (
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
      float* output,
      size_t n_bytes
    ) const {
      size_t elems = sample_dims.x * sample_dims.y;
      size_t bytes = elems * sizeof(float);

      if (bytes > n_bytes) {
        return 0;
      }

      memset(output, 0, bytes);

      glm::dvec2 coords;
      for (size_t y = 0; y < sample_dims.y; y++) {
        coords.y = y * scale + origin.y;
        for (size_t x = 0; x < sample_dims.x; x++) {
          coords.x = x * scale + origin.x;
          output[y * sample_dims.x + x] = SampleFalloffSum(coords);
        }
      }

      return bytes;
    }

   private:
    const vector_type samplers;
  };
}

#endif // MULTI_BOX_SAMPLER_H_

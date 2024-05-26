#ifndef STUB_SMOOTHING_SAMPLER_BOX_H_
#define STUB_SMOOTHING_SAMPLER_BOX_H_

#include "corrugate/box/BaseSmoothingSamplerBox.hpp"
namespace cg {
  class StubSmoothingSamplerBox : public BaseSmoothingSamplerBox {
   public:
    StubSmoothingSamplerBox() : SamplerBox(glm::dvec2(0), glm::dvec2(0), 0.0f, 0.0f) {}

    float SampleHeight(double x, double y) const override { return 0.0f; }
    glm::vec4 SampleSplat(double x, double y, size_t index) const override { return glm::vec4(0.0f); }
    float SampleTreeFill(double x, double y) const override { return 0.0f; }
    float SampleGrassFill(double x, double y) const override { return 0.0f; }

    size_t WriteHeight(   const glm::dvec2& origin, const glm::ivec2& sample_dims, double scale,                float* output,      size_t n_bytes) const override {
      return 0;
    }
    size_t WriteSplat(    const glm::dvec2& origin, const glm::ivec2& sample_dims, double scale, size_t index,  glm::vec4* output,  size_t n_bytes, const DataSampler<float>* falloffs) const override {
      return 0;
    }
    size_t WriteTreeFill( const glm::dvec2& origin, const glm::ivec2& sample_dims, double scale,                float* output,      size_t n_bytes, const DataSampler<float>* falloffs) const override {
      return 0;
    }
    size_t WriteGrassFill(const glm::dvec2& origin, const glm::ivec2& sample_dims, double scale,                float* output,      size_t n_bytes) const override {
      return 0;
    }

    float GetSmoothDelta(double x, double y, double underlying) const override { return 0.0f; }

    size_t WriteSmoothDelta(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
      const DataSampler<float>& underlying_data,
      const DataSampler<float>& falloff_sums,
      float* output,
      size_t n_bytes
    ) const override { return 0; }
  };
}

#endif // STUB_SMOOTHING_SAMPLER_BOX_H_

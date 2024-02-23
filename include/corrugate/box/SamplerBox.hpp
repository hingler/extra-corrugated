#ifndef SAMPLER_BOX_H_
#define SAMPLER_BOX_H_

#include "corrugate/FeatureBox.hpp"
#include "corrugate/sampler/DataSampler.hpp"

#include <glm/glm.hpp>

#include <chunker/util/Fraction.hpp>

// behavior
// - virtuals for sampling height, splat, fills
// - virtuals for writing chunks of height, splat, fill (to output)

namespace cg {
  class SamplerBox : public cg::FeatureBox {
   public:

    /// @brief Create sampler box
    /// @param falloff_radius - distance on which to handle falloff - 0 = sharp edge
    /// @param falloff_dist - distance from border on which to handle falloff
    SamplerBox(const glm::vec2& origin, const glm::dvec2& size, float falloff_radius, float falloff_dist) : FeatureBox(origin, size, falloff_radius, falloff_dist) {}

    /// @brief   Samples height
    /// @param x - global x coordinate
    /// @param y - global y coordinate
    /// @return - sample
    virtual float SampleHeight(   double x, double y) const = 0;
    virtual glm::vec4 SampleSplat(double x, double y, size_t index)     const = 0;
    virtual float SampleTreeFill( double x, double y)                   const = 0;

    /**
     * @brief Writes a chunk of data
     *
     * @param origin - global origin
     * @param sample_dims - num of x/y samples
     * @param scale - scale of sampling
     * @param output - output
     * @param n_bytes - num bytes available to write
     * @return size_t - number of bytes written
     */
    virtual size_t WriteHeight(   const glm::dvec2& origin, const glm::ivec2& sample_dims, double scale,                float* output,      size_t n_bytes) const = 0;
    virtual size_t WriteSplat(    const glm::dvec2& origin, const glm::ivec2& sample_dims, double scale, size_t index,  glm::vec4* output,  size_t n_bytes, const DataSampler<float>* falloffs) const = 0;
    virtual size_t WriteTreeFill( const glm::dvec2& origin, const glm::ivec2& sample_dims, double scale,                float* output,      size_t n_bytes, const DataSampler<float>* falloffs) const = 0;
  };
}

#endif // SAMPLER_BOX_H_

#ifndef BASE_SMOOTHING_SAMPLER_BOX_H_
#define BASE_SMOOTHING_SAMPLER_BOX_H_

#include "corrugate/box/SamplerBox.hpp"
#include "corrugate/sampler/DataSampler.hpp"

namespace cg {
  class BaseSmoothingSamplerBox : virtual public SamplerBox {
   public:
    using SamplerBox::SamplerBox;

    /// @brief Returns smoothing delta, applied to underlying terrain.
    /// @param x - global x
    /// @param y - global y
    /// @return value indicating delta applied to underlying terrain in order to smooth this sampler
    virtual float GetSmoothDelta(double x, double y, double underlying) const = 0;

    /**
     * @brief Write smoothing deltas for an entire chunk
     * @param underlying_data - underlying data, in dims space
     * @param falloff_sums - sum of falloffs of all overlapping components, in dims space
     * @param output - float outputs for smoothed data
     * @param n_bytes - number of bytes
     * @return size_t - outputted delta to modify underlying terrain by
     */
    virtual size_t WriteSmoothDelta(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
      const DataSampler<float>& underlying_data,
      const DataSampler<float>& falloff_sums,
      float* output,
      size_t n_bytes
    ) const = 0;

    virtual ~BaseSmoothingSamplerBox() {}
  };
}

#endif // SMOOTHING_SAMPLER_BOX_H_

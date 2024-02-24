#ifndef SMOOTHING_TERRAIN_SAMPLER_H_
#define SMOOTHING_TERRAIN_SAMPLER_H_

#include "corrugate/FeatureBox.hpp"

#include <algorithm>
#include <mutex>

namespace cg {
  class SmoothingTerrainSampler {
    // lazy init origin here still? thinking so
   public:
    // box param doesn't work
    // eventually: option to provide a max slope
    SmoothingTerrainSampler(
      const FeatureBox& box
    ) : box_(box) {}

    template <typename BaseType>
    void PrepareCache(const std::shared_ptr<BaseType>& sampler) {
      CalculateOrigin(sampler);
    }


    double Smooth(double input) const {

      double delta = height_origin - input;

      // should we build this functionality into the box itself?

      return delta * smoothing_factor;
    }

    // 1.0: completely flat
    // 0.0: no smoothing

    // tba: replace this with a smoothing sampler (or a const sampler, if not provided)
    // LOTS of ctor args :-)

    // replace smoothing factor with a "max slope" estimated from gradient vecs
    double smoothing_factor = 0.0;
   private:
    static constexpr double MAX_SLOPE = 0.09;
    mutable double height_origin = 0.0;
    mutable double secret_smoothing_factor = 0.0;
    mutable bool cached_ = false;
    mutable std::mutex cache_lock_;

    // this doesn't work lole
    FeatureBox box_;


    glm::dvec2 GetHammersley(unsigned int x, unsigned int n, const glm::dvec2& origin, const glm::dvec2& size) const {
      unsigned int y = BitReverse(x); // need this as a fraction
      double x_frac = x / static_cast<double>(n);

      // bit reversed - need it as a fraction
      double y_frac = y / static_cast<double>(0x100000000);

      return glm::dvec2(x_frac * size.x + origin.x, y_frac * size.y + origin.y);
    }

    unsigned int BitReverse(unsigned int x) const {
      x = ((x & 0x0000FFFF) << 16) | ((x & 0xFFFF0000) >> 16);
      x = ((x & 0x00FF00FF) << 8)  | ((x & 0xFF00FF00) >> 8);
      x = ((x & 0x0F0F0F0F) << 4)  | ((x & 0xF0F0F0F0) >> 4);
      x = ((x & 0x33333333) << 2)  | ((x & 0xCCCCCCCC) >> 2);
      x = ((x & 0x55555555) << 1)  | ((x & 0xAAAAAAAA) >> 1);

      return x;
    }

    // 64x64 equiv (could probably decrease further)
    #define _HAMMERSLEY_SAMPLES 4096U

    // handled all at once by some chunk - estimates a height origin from contained samples
    #define EPSILON 0.001
    #define INV_EPSILON (1.0 / EPSILON)

    template <typename US>
    void CalculateOrigin(const std::shared_ptr<US>& base_sampler) const {
      std::lock_guard<std::mutex> lock(cache_lock_);

      if (cached_) {
        return;
      }

      double height_sum = 0.0;
      glm::dvec2 origin = box_.GetOrigin();
      glm::dvec2 size   = box_.GetSize();
      double max_slope = 0.00001;
      for (unsigned int i = 0; i < _HAMMERSLEY_SAMPLES; i++) {
        glm::dvec2 point = GetHammersley(i, _HAMMERSLEY_SAMPLES, origin, size);
        double height = base_sampler->Sample(point.x, point.y);
        double grad_x = (base_sampler->Sample(point.x + EPSILON, point.y) - height) * INV_EPSILON;
        double grad_y = (base_sampler->Sample(point.x, point.y + EPSILON) - height) * INV_EPSILON;
        max_slope = std::max(std::sqrt(grad_x * grad_x + grad_y * grad_y), max_slope);

        // two free samples :3
        height_sum += height / static_cast<double>(_HAMMERSLEY_SAMPLES);
        // tba: calculate gradient samples as well, to get a rough estimate of max height
        // add opt. parameter for "max slope"

      }

      // safe keeping for now
      secret_smoothing_factor = 1.0 - (MAX_SLOPE / max_slope);
      height_origin = height_sum;
      cached_ = true;
    }
  };
}

#endif // SMOOTHING_TERRAIN_SAMPLER_H_

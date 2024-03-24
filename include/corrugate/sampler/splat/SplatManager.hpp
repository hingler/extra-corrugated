#ifndef I_SPLAT_MANAGER_H_
#define I_SPLAT_MANAGER_H_

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "corrugate/sampler/splat/impl/SplatWriterImpl.hpp"

// tba: deprecate impl in terraingen in favor of this
// (why is this here? because it helps w creating boxes!)
// (plus: we kinda shrunk down its scope to boxes instead of a broad map)

namespace cg {
  /**
   * @brief Class which will wrap operations pertaining to splatmaps
   */
  class SplatManager {
    public:
    /**
     * @brief Binds a group of samplers to a specified splat index.
     *
     * @tparam sR - red sampler
     * @tparam sG - green sampler
     * @tparam sB - blue sampler
     * @tparam sA - alpha sampler
     * @param r - red channel sampler
     * @param g - green channel sampler
     * @param b - blue channel sampler
     * @param a - alpha channel sampler
     * @param splat_index - splat map index for these samplers
     */
    template <typename sR, typename sG, typename sB, typename sA>
    void BindSamplers(
      const std::shared_ptr<sR>& r,
      const std::shared_ptr<sG>& g,
      const std::shared_ptr<sB>& b,
      const std::shared_ptr<sA>& a,
      size_t splat_index
    ) {
      EnsureCapacity(splat_index);
      writers_[splat_index] = std::make_unique<impl::SplatWriterImpl<sR, sG, sB, sA>>(
        r, g, b, a
      );
    }

    template <typename Splat>
    void BindSampler(
      const std::shared_ptr<Splat>& s,
      size_t splat_index
    ) {
      EnsureCapacity(splat_index);
      writers_[splat_index] = std::make_unique<impl::SingleSplatWriter<Splat>>(
        s,
        splat_index
      );
    }

    // thinking: pass in number of samplers to pull, for consistency (don't write if not avail)

    bool HasSampler(size_t splat_index) const {
      return splat_index >= 0 && splat_index < writers_.capacity() && writers_[splat_index] != nullptr;
    }

    int GetLayerCount() const { return writers_.size(); }


    /**
     * @brief Writes splat contents to an output buffer.
     *
     * @param size - size of output, in px
     * @param offset - offset applied to bottom left corner of image
     * @param scale - distance between pixel samples
     * @param float output for image - must have enough space to wrap entire image
     * @param splat_index - splat index to sample from
     */
    bool WriteSampler(const glm::ivec2& size, const glm::dvec2& offset, const glm::dvec2& scale, float* output, size_t splat_index) const {
      if (!HasSampler(splat_index)) {
        return false;
      }

      writers_[splat_index]->Write(size, offset, scale, output);
      return true;
    }

    glm::vec4 Sample(double x, double y, size_t index) const {
      if (!HasSampler(index)) {
        return glm::vec4(0);
      }

      return writers_[index]->Sample(x, y);
    }

    // tba: write a simple "splat test" which just puts some sample data in an image

   private:
    void EnsureCapacity(size_t splat_index) {
      if (writers_.size() < (splat_index + 1)) {
        writers_.resize(splat_index + 1);
      }
    }
    std::vector<std::unique_ptr<impl::SplatWriter>> writers_;
  };
}

#endif // I_SPLAT_MANAGER_H_

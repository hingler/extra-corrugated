#ifndef BASE_TERRAIN_SAMPLER_H_
#define BASE_TERRAIN_SAMPLER_H_

#include "corrugate/sampler/SampleWriterGeneric.hpp"

#include <chunker/util/Fraction.hpp>

// smoothing box should handle itself - we just pass in the params
// (alt: thinking we just compose this lol)
// (or: expose both funcs and just call the one)

namespace cg {
  // base sampler for a terrain box
  class BaseTerrainSampler {
   public:
    template <typename HeightType, typename SplatType, typename TreeFillType>
    BaseTerrainSampler(
      std::shared_ptr<HeightType> heightmap,
      std::shared_ptr<SplatType> splat,
      std::shared_ptr<TreeFillType> tree_fill
    ) :
      height_(std::make_unique<SampleWriterGenericImpl<float, HeightType>>(heightmap)),
      splat_(std::make_unique<IndexedSampleWriterGenericImpl<glm::vec4, SplatType>>(splat)),
      tree_fill_(std::make_unique<SampleWriterGenericImpl<float, TreeFillType>>(tree_fill))
    {}

    float SampleHeight(double x, double y) const {
      return height_->Sample(x, y);
    }

    // issue was: in order to do avg smoothing on course segments, we need to know weights (as falloff).
    // prob: expose a trivial "GetFalloff" func which calcs weight given pos

    glm::vec4 SampleSplat(double x, double y, size_t index) const {
      return splat_->Sample(x, y, index);
    }

    float SampleTreeFill(double x, double y) const {
      return tree_fill_->Sample(x, y);
    }

    size_t WriteHeight(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      const chunker::util::Fraction& scale,
      float* output,
      size_t n_bytes
    ) const {
      return height_->WriteChunk(origin, sample_dims, scale, output, n_bytes);
    }

    // who should be remapping origin? us, or them?
    // (probably: us)
    size_t WriteSplat(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      const chunker::util::Fraction& scale,
      size_t index,
      glm::vec4* output,
      size_t n_bytes
    ) const {
      // why tf did i do this (oh - because indices didn't really work)
      return splat_->WriteChunk(origin, sample_dims, scale.AsDouble(), index, output, n_bytes);
    }

    size_t WriteTreeFill(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      const chunker::util::Fraction& scale,
      float* output,
      size_t n_bytes
    ) const {
      return tree_fill_->WriteChunk(origin, sample_dims, scale, output, n_bytes);
    }
   private:
    std::unique_ptr<SampleWriterGeneric<float>> height_;
    std::unique_ptr<IndexedSampleWriterGeneric<glm::vec4>> splat_;
    std::unique_ptr<SampleWriterGeneric<float>> tree_fill_;

  };
}

#endif // BASE_TERRAIN_SAMPLER_H_

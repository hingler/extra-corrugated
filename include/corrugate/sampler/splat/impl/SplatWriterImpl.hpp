#ifndef SPLAT_WRITER_IMPL_H_
#define SPLAT_WRITER_IMPL_H_

// tba: really bad for cache interleaving (how can we speed this up?)
// - wrap classes in groups of four (four templates?)
// - how do we handle empty samplers, in that case??
// - defaultsampler (just do nothing) [would have to pass in, that's fine]

#include <memory>

#include "corrugate/sampler/splat/impl/SplatWriter.hpp"
#include "corrugate/traits/chunk_write_trait.hpp"

#include <gog43/Logger.hpp>

namespace cg {
  namespace impl {

    template <typename Splat, typename Enable = void>
    class SingleSplatChunkWriteDelegate {
     public:
      SingleSplatChunkWriteDelegate(
        const std::shared_ptr<Splat>& splat,
        size_t index
      ) : splat(splat), index(index) {}

      void Write(const glm::ivec2& size, const glm::dvec2& offset, const glm::dvec2& scale, float* output) const {
        gog43::print("using single write :/");
        glm::vec4* wptr = reinterpret_cast<glm::vec4*>(output);
        glm::dvec2 sample_pos;
        glm::dvec2 half_scale = scale * 0.5;

        for (int y = 0; y < size.y; ++y) {
          sample_pos.y = offset.y + scale.y * y + half_scale.y;
          for (int x = 0; x < size.x; ++x) {
            sample_pos.x = offset.x + scale.x * x + half_scale.x;
            *wptr = splat->Sample(sample_pos.x, sample_pos.y, index);
            ++wptr;
          }
        }
      }

     private:
      std::shared_ptr<Splat> splat;
      size_t index;
    };

    // should have written the specialization as a separate method lole
    // lgtm : )
    // specialization if chunk write supported
    template <typename Splat>
    class SingleSplatChunkWriteDelegate<
      Splat,
      typename std::enable_if_t<trait::splat_chunk_trait<Splat>::value>
    > {
     public:
      SingleSplatChunkWriteDelegate(
        const std::shared_ptr<Splat>& splat,
        size_t index
      ) : splat(splat), index(index) {}

      void Write(const glm::ivec2& size, const glm::dvec2& offset, const glm::dvec2& scale, float* output) const {
        gog43::print("using bulk write!");
        splat->WriteSplat(
          offset,
          size,
          scale.x,
          index,
          reinterpret_cast<glm::vec4*>(output),
          (size.x * size.y * sizeof(glm::vec4))
        );
      }
     private:
      std::shared_ptr<Splat> splat;
      size_t index;
    };


    template <typename Splat, typename Enable = void>
    class SingleSplatWriter : public SplatWriter {
     public:
      SingleSplatWriter(
        const std::shared_ptr<Splat>& splat,
        size_t index
      ) : splat(splat), index(index) {}

      void Write(const glm::ivec2& size, const glm::dvec2& offset, const glm::dvec2& scale, float* output) const override {
        SingleSplatChunkWriteDelegate<Splat> del(splat, index);
        del.Write(size, offset, scale, output);
      }

      glm::vec4 Sample(double x, double y) override {
        return splat->Sample(x, y, index);
      }
     private:
      std::shared_ptr<Splat> splat;
      size_t index;
    };

    /**
     * @brief virtual
     *
     * @tparam sR - red channel sampler
     * @tparam sG - green channel sampler
     * @tparam sB - blue channel sampler
     * @tparam sA - alpha channel sampler
     */
    template <typename sR, typename sG, typename sB, typename sA>
    class SplatWriterImpl : public SplatWriter {
      // old code - tba :3
      // static_assert(traits::sampler_type<sR>::value);
      // static_assert(traits::sampler_type<sG>::value);
      // static_assert(traits::sampler_type<sB>::value);
      // static_assert(traits::sampler_type<sA>::value);
      public:
      SplatWriterImpl(
        const std::shared_ptr<sR>& r,
        const std::shared_ptr<sG>& g,
        const std::shared_ptr<sB>& b,
        const std::shared_ptr<sA>& a
      ) : sampler_r(*r), sampler_g(*g), sampler_b(*b), sampler_a(*a) {}
      void Write(const glm::ivec2& size, const glm::dvec2& offset, const glm::dvec2& scale, float* output) const override {
        float* wptr = output;
        glm::dvec2 sample_pos;
        // just realized: splat is probably
        glm::dvec2 half_scale = scale * 0.5;
        for (int y = 0; y < size.y; ++y) {
          for (int x = 0; x < size.x; ++x) {
            // inc by half scale - want to sample pixel center, not pixel corner
            sample_pos.x = offset.x + scale.x * x + half_scale.x;
            sample_pos.y = offset.y + scale.y * y + half_scale.y;
            *wptr++ = sampler_r.Sample(sample_pos.x, sample_pos.y);
            *wptr++ = sampler_g.Sample(sample_pos.x, sample_pos.y);
            *wptr++ = sampler_b.Sample(sample_pos.x, sample_pos.y);
            *wptr++ = sampler_a.Sample(sample_pos.x, sample_pos.y);
          }
        }
      }

      glm::vec4 Sample(double x, double y) override {
        return glm::vec4(
          sampler_r.Sample(x, y),
          sampler_g.Sample(x, y),
          sampler_b.Sample(x, y),
          sampler_a.Sample(x, y)
        );
      }
      private:
      sR sampler_r;
      sG sampler_g;
      sB sampler_b;
      sA sampler_a;
    };
  }
}

#endif // SPLAT_WRITER_IMPL_H_

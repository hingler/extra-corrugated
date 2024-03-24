#ifndef SAMPLE_WRITER_GENERIC_H_
#define SAMPLE_WRITER_GENERIC_H_

#include <glm/glm.hpp>

#include "corrugate/sampler/SingleIndexSplatManager.hpp"

// for splat manager: how to handle?
// prob just a thin wrapper that picks a specific sample
namespace cg {
  template <typename DataType>
  class SampleWriterGeneric {
   public:
    virtual DataType Sample(double x, double y) const;
    virtual size_t WriteChunk(const glm::dvec2& origin, const glm::ivec2& sample_dims, double scale, DataType* output, size_t n_bytes) const = 0;
    virtual ~SampleWriterGeneric() {}
  };

  template <typename DataType>
  class IndexedSampleWriterGeneric {
    public:
     virtual DataType Sample(double x, double y, size_t index) const;
     virtual size_t WriteChunk(const glm::dvec2& origin, const glm::ivec2& sample_dims, double scale, size_t index, DataType* output, size_t n_bytes) const = 0;
     virtual ~IndexedSampleWriterGeneric() {};
  };

  template <typename DataType, typename SamplerType>
  class SampleWriterGenericImpl : public SampleWriterGeneric<DataType> {
   public:
    SampleWriterGenericImpl(std::shared_ptr<SamplerType> sampler) : sampler_(sampler) {}

    DataType Sample(double x, double y) const override {
      return sampler_->Sample(x, y);
    }

    // prob not gonna test this

    size_t WriteChunk(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
      DataType* output,
      size_t n_bytes
    ) const override {
      size_t required_space = sample_dims.x * sample_dims.y * sizeof(DataType);
      if (required_space > n_bytes) {
        return 0;
      }

      DataType data;
      glm::dvec2 pos;

      double scale_d = scale;

      // side note: for splats we need to adjust by 0.5

      for (int y = 0; y < sample_dims.y; y++) {
        for (int x = 0; x < sample_dims.x; x++) {
          pos.x = origin.x + x * scale_d;
          pos.y = origin.y + y * scale_d;
          output[y * sample_dims.x + x] = sampler_->Sample(pos.x, pos.y);
        }
      }

      return required_space;
    }
   private:
    std::shared_ptr<SamplerType> sampler_;
  };

  template <typename DataType, typename SplatType>
  class IndexedSampleWriterGenericImpl : public IndexedSampleWriterGeneric<DataType> {
   public:
    IndexedSampleWriterGenericImpl(std::shared_ptr<SplatType> splat) : splat_(splat) {}
    DataType Sample(double x, double y, size_t index) const override {
      return splat_->Sample(x, y, index);
    }

    size_t WriteChunk(
      const glm::dvec2& origin,
      const glm::ivec2& sample_dims,
      double scale,
      size_t index,
      DataType* output,
      size_t n_bytes
    ) const override {
      SampleWriterGenericImpl<DataType, SingleIndexSplatManager<SplatType>> temp(std::make_shared<SingleIndexSplatManager<SplatType>>(splat_, index));
      return temp.WriteChunk(
        origin,
        sample_dims,
        scale,
        output,
        n_bytes
      );
    }
   private:
    std::shared_ptr<SplatType> splat_;
  };
}

#endif // SAMPLE_WRITER_GENERIC_H_

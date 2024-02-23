#ifndef DATA_SAMPLER_H_
#define DATA_SAMPLER_H_

#include <glm/glm.hpp>

namespace cg {
  template <typename DataType>
  class DataSampler {
    // map to preexisting data
   public:
    // does not take ownership of data
    DataSampler(const glm::ivec2& data_size, DataType* data) : data_size(data_size), data_(data) {}

    DataType Get(int x, int y) const {
      if (x >= 0 && x < data_size.x && y >= 0 && y < data_size.y) {
        return data_[y * data_size.x + x];
      }

      return DataType{};
    }

    const glm::ivec2 data_size;

   private:
    const DataType* data_;
  };
}

#endif // DATA_SAMPLER_H_

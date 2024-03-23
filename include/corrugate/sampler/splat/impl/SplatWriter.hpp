#ifndef SPLAT_WRITER_H_
#define SPLAT_WRITER_H_

#include <glm/glm.hpp>

namespace cg {
  namespace impl {
    class SplatWriter {
      public:
      /**
       * @brief Writes writer's contents to passed float output
       *
       * @param size - size of output image
       * @param offset - offset applied to samplers
       * @param scale - distance between pixels
       * @param output - output for image data
       */
      virtual void Write(const glm::ivec2& size, const glm::dvec2& offset, const glm::dvec2& scale, float* output) const = 0;
      virtual glm::vec4 Sample(double x, double y) = 0;
      virtual ~SplatWriter() {}
    };
  }
}

#endif // SPLAT_WRITER_H_

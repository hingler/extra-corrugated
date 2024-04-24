#ifndef EC_HAMMERSLEY_H_
#define EC_HAMMERSLEY_H_

#include <glm/glm.hpp>

namespace cg {
  namespace smooth {
    namespace impl {
      inline unsigned int BitReverse(unsigned int x) {
        x = ((x & 0x0000FFFF) << 16) | ((x & 0xFFFF0000) >> 16);
        x = ((x & 0x00FF00FF) << 8)  | ((x & 0xFF00FF00) >> 8);
        x = ((x & 0x0F0F0F0F) << 4)  | ((x & 0xF0F0F0F0) >> 4);
        x = ((x & 0x33333333) << 2)  | ((x & 0xCCCCCCCC) >> 2);
        x = ((x & 0x55555555) << 1)  | ((x & 0xAAAAAAAA) >> 1);

        return x;
      }

      inline glm::dvec2 Hammersley(
        unsigned int x,
        unsigned int n,
        const glm::dvec2& origin,
        const glm::dvec2& size
      ) {
        unsigned int y = BitReverse(x); // need this as a fraction
        double x_frac = x / static_cast<double>(n);

        // bit reversed - need it as a fraction
        double y_frac = y / static_cast<double>(0x100000000);

        return glm::dvec2(x_frac * size.x + origin.x, y_frac * size.y + origin.y);
      }
    }
  }
}

#endif // EC_HAMMERSLEY_H_

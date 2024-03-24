#ifndef CG_CHUNK_WRITE_TRAIT_H_
#define CG_CHUNK_WRITE_TRAIT_H_

#include <type_traits>

#include <glm/glm.hpp>

namespace cg {
  namespace trait {
    namespace _impl {
      struct splat_chunk_trait_impl {
        template <typename Writer,
        typename WriteFunc = std::is_same<
          size_t,
          decltype(
            std::declval<const Writer&>().WriteSplat(
              std::declval<const glm::dvec2&>(),
              std::declval<const glm::ivec2&>(),
              (double)1.0,
              (size_t)0,
              std::declval<glm::vec4*>(),
              (size_t)0
            )
          )>>
        static std::true_type test(int);

        template <typename Writer, typename...>
        static std::false_type test(...);
      };
    }

    template <typename T>
    struct splat_chunk_trait : decltype(_impl::splat_chunk_trait_impl::test<T>(0)) {};
  }
}

#endif // CG_SPLAT_TRAIT_H_

#ifndef SINGLE_INDEX_SPLAT_MANAGER_H_
#define SINGLE_INDEX_SPLAT_MANAGER_H_

#include <glm/glm.hpp>

#include <memory>

// super trivial
namespace cg {
  template <typename SplatType>
  class SingleIndexSplatManager {
   public:
    SingleIndexSplatManager(std::shared_ptr<SplatType> mgr, size_t index) : manager(mgr), ind(index) {}

    glm::vec4 Sample(double x, double y) {
      return manager->Sample(x, y, ind);
    }
   private:
    std::shared_ptr<SplatType> manager;
    size_t ind;
  };
}

#endif // SINGLE_INDEX_SPLAT_MANAGER_H_

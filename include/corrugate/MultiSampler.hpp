#ifndef MULTI_SAMPLER_H_
#define MULTI_SAMPLER_H_

#include "corrugate/FeatureBox.hpp"


#include <memory>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#define _SAMPLER_CHUNK_SIZE 512

namespace cg {
  template <typename BoxType>
  class MultiSampler {
   public:
    // assert that our boxes implement the box spec
    // this is what we want - enforce impl of spec
    static_assert(std::is_base_of_v<FeatureBox, BoxType>);

    void FetchPoint(const glm::dvec2& point, std::unordered_set<std::shared_ptr<const BoxType>>& output) const {
      FetchRange(point, glm::dvec2(0), output);
    }

    // fetch all boxes within a certain range
    void FetchRange(const glm::dvec2& origin, const glm::dvec2& size, std::unordered_set<std::shared_ptr<const BoxType>>& output) const {
      glm::dvec2 end = origin + size;

      glm::ivec2 chunk_floor = static_cast<glm::ivec2>(glm::floor(origin / static_cast<double>(_SAMPLER_CHUNK_SIZE)));
      glm::ivec2 chunk_ceil = static_cast<glm::ivec2>(glm::ceil(end / static_cast<double>(_SAMPLER_CHUNK_SIZE)));

      // zlup - this section of code recurs a ton
      std::lock_guard<std::recursive_mutex> lock(sampler_lock);

      glm::dvec2 box_origin, box_end;
      for (int x = chunk_floor.x; x < chunk_ceil.x; x++) {
        for (int y = chunk_floor.y; y < chunk_ceil.y; y++) {
          glm::ivec2 chunk(x, y);
          typename cache_type::const_iterator itr = chunk_lookup_cache.find(chunk);
          if (itr != chunk_lookup_cache.end()) {
            for (auto& box : itr->second) {
              box_origin = box->GetOrigin();
              box_end = box->GetEnd();

              if (
                   box_origin.x < end.x     && box_origin.y < end.y
                && box_end.x    > origin.x  && box_end.y    > origin.y
              ) {
                output.insert(std::const_pointer_cast<const BoxType>(box));
              }
            }
          }
        }
      }
    }

    // fetches all boxes in the range of some pre-specified box
    void FetchRange(const std::shared_ptr<BoxType>& box, std::unordered_set<std::shared_ptr<const BoxType>>& output) const {
      FetchRange(box->GetOrigin(), box->GetSize(), output);
    }

    // the issue is, effectively, that this container doesn't maintain its own state (it cant!)
    template <typename InsertType, class... Args>
    std::shared_ptr<const BoxType> InsertBox(Args... args) {
      std::shared_ptr<InsertType> instance = std::make_shared<InsertType>(args...);
      std::shared_ptr<BoxType> box = std::dynamic_pointer_cast<BoxType>(instance);
      glm::dvec2 origin = box->origin;
      glm::dvec2 end = box->GetEnd();

      glm::ivec2 chunk_floor = static_cast<glm::ivec2>(glm::floor(origin / static_cast<double>(_SAMPLER_CHUNK_SIZE)));
      glm::ivec2 chunk_ceil = static_cast<glm::ivec2>(glm::ceil(end / static_cast<double>(_SAMPLER_CHUNK_SIZE)));

      std::lock_guard<std::recursive_mutex> lock(sampler_lock);
      for (int x = chunk_floor.x; x < chunk_ceil.x; x++) {
        for (int y = chunk_floor.y; y < chunk_ceil.y; y++) {
          glm::ivec2 chunk(x, y);
          // need to test
          InsertIntoChunk(chunk, box);
        }
      }

      return std::const_pointer_cast<const BoxType>(box);
    }

    std::shared_ptr<BoxType> RemoveBox(const std::shared_ptr<const BoxType>& box) {
      glm::dvec2 origin = box->GetOrigin();
      glm::dvec2 end = origin + box->GetSize();

      glm::ivec2 chunk_floor = static_cast<glm::ivec2>(glm::floor(origin / static_cast<double>(_SAMPLER_CHUNK_SIZE)));
      glm::ivec2 chunk_ceil = static_cast<glm::ivec2>(glm::ceil(end / static_cast<double>(_SAMPLER_CHUNK_SIZE)));

      std::lock_guard<std::recursive_mutex> lock(sampler_lock);
      for (int x = chunk_floor.x; x < chunk_ceil.x; x++) {
        for (int y = chunk_floor.y; y < chunk_ceil.y; y++) {
          glm::ivec2 chunk(x, y);
          typename cache_type::iterator itr = chunk_lookup_cache.find(chunk);
          if (itr != chunk_lookup_cache.end()) {
            auto box_cast = std::const_pointer_cast<BoxType>(box);
            typename set_type::iterator itr_local = itr->second.find(box_cast);
            if (itr_local != itr->second.end()) {
              itr->second.erase(itr_local);
              return box_cast;
            }
          }
        }
      }

      return std::shared_ptr<BoxType>();
    }
   private:
    // eff: i want "multisampler" to manage everything for me... but there's no way to spawn things atm
    void InsertIntoChunk(const glm::ivec2& chunk, const std::shared_ptr<BoxType>& box) {
      typename cache_type::iterator itr = chunk_lookup_cache.find(chunk);
      if (itr == chunk_lookup_cache.end()) {
        chunk_lookup_cache.insert(std::make_pair(chunk, std::unordered_set<std::shared_ptr<BoxType>> {}));
        itr = chunk_lookup_cache.find(chunk);
      }

      itr->second.insert(box);
    }

    mutable std::recursive_mutex sampler_lock;

    // best way to handle these chunking operations? prob just vector
    // - no way to get around memory stipulations, i don't think :/
    typedef std::shared_ptr<BoxType> box_type;
    typedef std::unordered_set<box_type> set_type;
    typedef std::unordered_map<glm::ivec2, set_type> cache_type;

    cache_type chunk_lookup_cache;

    // removal: sample bounds on the passed-in box!
  };
}

#endif // MULTI_SAMPLER_H_

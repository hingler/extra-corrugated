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

// tba: create a class which abstracts registering smoothing/normal
//
// iteration behavior
// - iterate over map -> iterate over set
// - assume no modifications
// - if at end of map, then we're at end
// - if at end of set, then increment map and re-fetch itr
// - else, increment set

namespace cg {
  // base sampler - identifies box positions
  template <typename BoxType>
  class MultiSampler {
    typedef std::shared_ptr<BoxType> box_type;
    typedef std::unordered_set<box_type> set_type;
    typedef std::unordered_map<glm::ivec2, set_type> cache_type;
   public:
    typedef std::unordered_set<std::shared_ptr<const BoxType>> output_type;
    // (making these public for impl)
    // best way to handle these chunking operations? prob just vector
    // - no way to get around memory stipulations, i don't think :/

    static_assert(std::is_base_of_v<FeatureBox, BoxType>);

    typedef typename set_type::const_iterator       iterator;

    iterator begin() const {
      return box_store.cbegin();
    }

    iterator end() const {
      return box_store.cend();
    }

    void FetchPoint(const glm::dvec2& point, std::unordered_set<std::shared_ptr<const BoxType>>& output) const {
      // narrow bounds here instead??
      FetchRange(point - glm::dvec2(0.5), glm::dvec2(1), output);
    }

    // fetch all boxes within a certain range
    void FetchRange(const glm::dvec2& origin, const glm::dvec2& size, std::unordered_set<std::shared_ptr<const BoxType>>& output) const {
      glm::dvec2 end = origin + size;

      // add an epsilon i think?
      // idea1: "pre-prep" the cache by collecting all boxes in some broad range (wrap)
      // idea2: idk!

      glm::ivec2 chunk_floor = static_cast<glm::ivec2>(glm::floor((origin - DVEC_EPSILON) / static_cast<double>(_SAMPLER_CHUNK_SIZE)));
      glm::ivec2 chunk_ceil = static_cast<glm::ivec2>(glm::ceil((end + DVEC_EPSILON) / static_cast<double>(_SAMPLER_CHUNK_SIZE)));

      // possible lock contention here??
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

    size_t size() const {
      return box_store.size();
    }

    // the issue is, effectively, that this container doesn't maintain its own state (it cant!)
    template <typename InsertType, class... Args>
    std::shared_ptr<const BoxType> InsertBox(Args... args) {
      std::shared_ptr<InsertType> instance = std::make_shared<InsertType>(args...);
      std::shared_ptr<BoxType> box = std::dynamic_pointer_cast<BoxType>(instance);

      InsertBoxPointer(box);

      return std::const_pointer_cast<const BoxType>(box);
    }

    std::shared_ptr<const BoxType> InsertBox(std::unique_ptr<BoxType>&& box) {
      std::shared_ptr<BoxType> box_ptr = std::move(box);
      InsertBoxPointer(box_ptr);
      return std::const_pointer_cast<const BoxType>(box_ptr);
    }

    std::shared_ptr<BoxType> RemoveBox(const std::shared_ptr<const BoxType>& box) {
      auto itr = box_store.find(std::const_pointer_cast<BoxType>(box));
      if (itr == box_store.end()) {
        return std::shared_ptr<BoxType>();
      }

      std::shared_ptr<BoxType> res = *itr;


      glm::dvec2 origin = res->GetOrigin();
      glm::dvec2 end = origin + res->GetSize();

      glm::ivec2 chunk_floor = static_cast<glm::ivec2>(glm::floor((origin - DVEC_EPSILON) / static_cast<double>(_SAMPLER_CHUNK_SIZE)));
      glm::ivec2 chunk_ceil = static_cast<glm::ivec2>(glm::ceil((end + DVEC_EPSILON) / static_cast<double>(_SAMPLER_CHUNK_SIZE)));


      std::lock_guard<std::recursive_mutex> lock(sampler_lock);
      for (int x = chunk_floor.x; x < chunk_ceil.x; x++) {
        for (int y = chunk_floor.y; y < chunk_ceil.y; y++) {
          glm::ivec2 chunk(x, y);
          typename cache_type::iterator itr = chunk_lookup_cache.find(chunk);
          if (itr != chunk_lookup_cache.end()) {

            typename set_type::iterator itr_local = itr->second.find(res);
            if (itr_local != itr->second.end()) {
              itr->second.erase(itr_local);
            }
          }
        }
      }

      box_store.erase(res);

      return res;
    }


   private:
    static constexpr glm::dvec2 DVEC_EPSILON = glm::dvec2(0.00001);
    void InsertBoxPointer(const std::shared_ptr<BoxType>& box) {
      glm::dvec2 origin = box->origin;
      glm::dvec2 end = box->GetEnd();

      glm::ivec2 chunk_floor = static_cast<glm::ivec2>(glm::floor((origin - DVEC_EPSILON) / static_cast<double>(_SAMPLER_CHUNK_SIZE)));
      glm::ivec2 chunk_ceil = static_cast<glm::ivec2>(glm::ceil((end + DVEC_EPSILON) / static_cast<double>(_SAMPLER_CHUNK_SIZE)));

      std::lock_guard<std::recursive_mutex> lock(sampler_lock);
      for (int x = chunk_floor.x; x < chunk_ceil.x; x++) {
        for (int y = chunk_floor.y; y < chunk_ceil.y; y++) {
          glm::ivec2 chunk(x, y);
          // need to test
          InsertIntoChunk(chunk, box);
        }
      }

      box_store.insert(box);
    }
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

   public:

    cache_type chunk_lookup_cache;
    // no great way to handle, other than backing up with a dupe set
    set_type   box_store;

    // (dupe'd shared ptr - about 24 bytes per, so assume like a few dozen extra KB :-])
  };
}

#endif // MULTI_SAMPLER_H_

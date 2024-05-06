#ifndef FEATURE_BOX_H_
#define FEATURE_BOX_H_

// a box which contains a feature
//
// need to move this out?? course gen needs to be able to build these
// course gen needs:
// - to be able to create feature boxes representing individual holes
// - to be able to fill said feature boxes with SDFs representing a hole
//
// we don't need splat functionality (and i dont want it)
// - higher level code should be responsible for communicating SDF data to terraingen
// - that being said: we should support roughly the same interface for fairway, green, sand, etc...
// - we should associate SDFs with boxes (i think sdfs for green, rough, etc etc...)

#include <algorithm>
#include <glm/glm.hpp>

namespace cg {
  class FeatureBox {
   public:
    FeatureBox() : FeatureBox(glm::dvec2(0), glm::dvec2(0)) {}
    FeatureBox(const glm::dvec2& origin, const glm::dvec2& size) : FeatureBox(origin, size, 1.0f, 1.0f) {}
    FeatureBox(const glm::dvec2& origin, const glm::dvec2& size, float falloff_radius, float falloff_dist) : FeatureBox(origin, size, falloff_radius, falloff_dist, 1.0) {}
    FeatureBox(const glm::dvec2& origin, const glm::dvec2& size, float falloff_radius, float falloff_size, float border_radius) : origin(origin), size(size), falloff_radius(falloff_radius), falloff_size(falloff_size), border_radius(border_radius) {}
    /// @brief Fetches the origin of this feature box
    /// @return origin
    glm::dvec2 GetOrigin() const { return origin; }

    /**
     * @brief Fetches the size of this feature box
     *
     * @return glm::dvec2 size
     */
    glm::dvec2 GetSize() const { return size; }


    glm::dvec2 GetEnd() const { return origin + size; }

    bool Contains(double x, double y) const {
      return Contains(glm::dvec2(x, y));
    }

    bool Contains(const glm::dvec2& point) const {
      glm::dvec2 local = point - origin;
      return Contains_Local(local);
    }

    bool Contains_Local(const glm::dvec2& point) const {
      if (size.x >= 0.0 && size.y >= 0.0) {
        return !(point.x < 0.0 || point.x > size.x || point.y < 0.0 || point.y > size.y);
      }

      // bullshit hack hehe
      return true;
    }

    float GetFalloffWeight(double x, double y) const {
      return GetFalloffWeight(glm::dvec2(x, y));
    }

    float GetFalloffWeight(const glm::dvec2& point) const {
      glm::dvec2 local = point - GetOrigin();
      return GetFalloffWeight_local(local);
    }

    bool operator==(const FeatureBox& other) const {
      return origin == other.origin
      && size == other.size
      && falloff_radius == other.falloff_radius
      && falloff_size == other.falloff_size
      && border_radius == other.border_radius;
    }

    const glm::dvec2 origin;
    const glm::dvec2 size;

    // radius: point at which falloff terminates
    const float falloff_radius;
    // size: ground covered by falloff
    const float falloff_size;

    // border radius of resultant falloff - 0.0 = sharp edge, 1.0 = rounded to shorter side
    // ignoring this rn hehe
    const float border_radius;

   protected:
    float GetFalloffWeight_local(const glm::dvec2& point_local) const {
      // -1 -> 1
      glm::dvec2 coords_local = (point_local / glm::max(GetSize(), glm::dvec2(0.001))) * 2.0 - 1.0;

      // one norm
      // dist = max cmpt (abs)
      float dist = static_cast<float>(glm::max(glm::abs(coords_local.x), glm::abs(coords_local.y)));
      // dist at which falloff begins ('falloff size' to 1.0 along 'falloff_radius')
      // avoid 1.0 case - breaks it
      float falloff_start = glm::min(falloff_radius * (1.0f - falloff_size), 0.99999f);
      // smoothstep?? not even doing it
      float falloff_val = (glm::clamp((dist - falloff_start) / (1.0f - falloff_start), 0.0f, 1.0f));
      falloff_val = glm::smoothstep(0.0f, 1.0f, falloff_val);
      return std::max(1.0f - falloff_val, 0.0f);
    }
   private:
  };

  // these should be generalized for every box
  // need to override sampling behavior via template

}

#endif // FEATURE_BOX_H_

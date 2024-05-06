#ifndef EC_SUB_SMOOTHER_H_
#define EC_SUB_SMOOTHER_H_

#include "glm/common.hpp"
#include <glm/glm.hpp>

#include <atomic>
#include <cmath>
#include <memory>
#include <mutex>

#include "corrugate/sampler/smooth/impl/Hammersley.hpp"

#define _SUB_HAMMERSLEY_SAMPLES 2048U
#define _SUB_EPSILON 0.0001
#define _SUB_INV_EPSILON (1.0 / _SUB_EPSILON)

namespace cg {
  namespace smooth {
    namespace impl {
      template <typename HeightType, typename SDFType>
      class SubSmoother {
       public:
        SubSmoother(
          const std::shared_ptr<HeightType>& underlying,
          const std::shared_ptr<SDFType>& sdf,
          const glm::dvec2& global_origin,
          const glm::dvec2& start,
          const glm::dvec2& end,
          double fade_dist,
          double target_slope_cents
        ) : height_map(underlying),
            weight_map(sdf),
            fade(std::abs(fade_dist)),
            global_origin(global_origin),
            start(start - fade),
            end(end + fade),
            cache_flag(false),
            target_slope_cents(target_slope_cents)
        {
        }


        double GetFalloffWeight(double x, double y) {
          double dist = weight_map->Sample(x, y);
          // negative when inside
          // positive when outside (dist)
          if (x < start.x || x > end.x || y < start.y || y > end.y) {
            return 0.0;
          }

          double falloff = 1.0 - glm::smoothstep(0.0, fade, dist);
          // should be OK here!
          // - add non-null rough samplers to this
          // - pass as sharedptr to sdf box
          // - pass to smoothing box (to use in the place of old smoother)
          return std::min(std::max(falloff, 0.0), 1.0);
        }

        double Smooth(double x, double y, double underlying) {
          glm::dvec2 pos(x, y);
          if (!cache_flag.test()) {
            Update_Cache();
          }

          // still busted hehe

          // temp. remove this to see if it fixes things
          if (pos.x < start.x || pos.x > end.x || pos.y < start.y || pos.y > end.y) {
            return 0.0;
          }


          double falloff = GetFalloffWeight(x, y);
          // delta: offset to bring "underlying" to "localorigin" (if added)
          // what if this is being double-applied somehow??
          double delta = local_origin - underlying;
          // scale down delta by deviation
          // oh - delta * (1.0 - fac)??

          double res = delta * (1.0 - local_smooth_factor) * falloff;

          return res;
        }

        // cache height origin
       private:
        void Update_Cache() {
          std::lock_guard<std::mutex> lock(cache_lock);
          // test only - possible race cond in multithread ctx???
          if (!cache_flag.test()) {
            // build cache

            double height_sum = 0.0;
            // need to offset further - sample in global space!!! (size is the same tho)
            glm::dvec2 origin = global_origin + start;
            glm::dvec2 size = (end - start);

            double max_slope = 0.00001;

            for (unsigned int i = 0; i < _SUB_HAMMERSLEY_SAMPLES; i++) {
              glm::dvec2 point = Hammersley(i, _SUB_HAMMERSLEY_SAMPLES, origin, size);
              double height = height_map->Sample(point.x, point.y);
              double grad_x = (height_map->Sample(point.x + _SUB_EPSILON, point.y) - height) * _SUB_INV_EPSILON;
              double grad_y = (height_map->Sample(point.x, point.y + _SUB_EPSILON) - height) * _SUB_INV_EPSILON;
              max_slope = std::max(std::sqrt(grad_x * grad_x + grad_y * grad_y), max_slope);

              height_sum += height;
            }

            height_sum /= static_cast<double>(_SUB_HAMMERSLEY_SAMPLES);

            // convert max slope to gradians
            double max_slope_cents = atan2(max_slope, 1.0) * (2.0 / M_PI);
            local_origin = height_sum;
            // lt 1.0
            local_smooth_factor = std::min(std::abs(target_slope_cents / max_slope_cents), 1.0);

            // set at the end
            cache_flag.test_and_set();
          }

        }

        const std::shared_ptr<HeightType> height_map;
        const std::shared_ptr<SDFType> weight_map;
        const double fade;

        const glm::dvec2 global_origin;

        const glm::dvec2 start;
        const glm::dvec2 end;

        const double target_slope_cents;

        double local_origin;
        double local_smooth_factor;

        std::atomic_flag cache_flag;
        std::mutex cache_lock;

      };
    }
  }
}

#endif // EC_SUB_SMOOTHER_H_

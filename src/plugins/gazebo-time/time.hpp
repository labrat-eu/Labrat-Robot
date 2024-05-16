/**
 * @file time.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/plugin.hpp>

/** @cond INTERNAL */
inline namespace labrat {
/** @endcond */
namespace lbot::plugins {

class GazeboTimeSource final : public UniquePlugin {
public:
  /**
   * @brief Construct a new Gazebo Time Source object
   * 
   */
  GazeboTimeSource();
  ~GazeboTimeSource();
};

}  // namespace lbot::plugins
/** @cond INTERNAL */
}  // namespace labrat
/** @endcond */

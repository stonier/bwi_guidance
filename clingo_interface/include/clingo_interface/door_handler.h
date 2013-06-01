/**
 * \file  door_handler.h
 * \brief  A wrapper around navfn and costmap_2d that determines if a door is 
 *         open or not, and calculates navigation targets while approaching a 
 *         door and going through a door
 *
 * \author  Piyush Khandelwal (piyushk@cs.utexas.edu)
 *
 * Copyright (c) 2013, UT Austin

 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 * Neither the name of the <organization> nor the
 names of its contributors may be used to endorse or promote products
 derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 *
 * $ Id: 05/06/2013 11:24:01 AM piyushk $
 *
 **/

#ifndef DOOR_HANDLER_WW75RJPS
#define DOOR_HANDLER_WW75RJPS

#include <boost/shared_ptr.hpp>
#include <clingo_interface/structures.h>
#include <topological_mapper/map_loader.h>
#include <topological_mapper/map_utils.h>

namespace clingo_interface {

  class DoorHandler {

    public:

      DoorHandler (std::string map_file, std::string door_file, std::string location_file) {
        readDoorFile(door_file, doors_);
        readLocationFile(location_file, locations_, location_map_);
        std::cout << "location map size" << location_map_.size() << std::endl;
        mapper_.reset(new topological_mapper::MapLoader(map_file));
        nav_msgs::OccupancyGrid grid;
        mapper_->getMap(grid);
        info_ = grid.info;
      }

      bool isDoorOpen(size_t idx) {
        return true;
      }

      bool getApproachPoint(size_t idx, 
          const topological_mapper::Point2f& current_location,
          topological_mapper::Point2f& point, float &yaw) {

        for (size_t pt = 0; pt < 2; ++pt) {
          std::cout << getLocationIdx(doors_[idx].approach_names[pt]) << " vs " <<  getLocationIdx(current_location) << std::endl;
          if (getLocationIdx(doors_[idx].approach_names[pt]) == getLocationIdx(current_location)) {
            point = doors_[idx].approach_points[pt];
            yaw = doors_[idx].approach_yaw[pt];
            return true;
          }
        }

        /* The door is not approachable from the current location */
        return false;
      }

      bool getThroughDoorPoint(size_t idx, 
          const topological_mapper::Point2f& current_location,
          topological_mapper::Point2f& point, float& yaw) {

        for (size_t pt = 0; pt < 2; ++pt) {
          if (getLocationIdx(doors_[idx].approach_names[pt]) == getLocationIdx(current_location)) {
            point = doors_[idx].approach_points[1 - pt];
            yaw = M_PI + doors_[idx].approach_yaw[1 - pt];
            return true;
          }
        }

        return false;
      }

      size_t getLocationIdx(const topological_mapper::Point2f& current_location) {

        topological_mapper::Point2f grid = topological_mapper::toGrid(current_location, info_);
        std::cout << grid.x << " and " << grid.y << std::endl;
        size_t map_idx = MAP_IDX(info_.width, (int) grid.x, (int) grid.y);
        std::cout << "location_map_ size" << location_map_.size() << std::endl;
        if (map_idx > location_map_.size()) {
          std::cout << "size too big" << std::endl; 
          return (size_t) -1;
        }
        return (size_t) location_map_[map_idx];

      }

      inline size_t getLocationIdx(const std::string& loc_str) const {
        for (size_t i = 0; i < locations_.size(); ++i) {
          if (locations_[i] == loc_str) {
            return i;
          }
        }
        return (size_t)-1;
      }

      inline size_t getDoorIdx(const std::string& door_str) const {
        for (size_t i = 0; i < doors_.size(); ++i) {
          if (doors_[i].name == door_str) {
            return i;
          }
        }
        return (size_t)-1;
      }

      inline std::string getLocationString(size_t idx) const {
        if (idx >= locations_.size())
          return "";
        return locations_[idx];
      }

      inline std::string getDoorString(size_t idx) const {
        if (idx >= doors_.size())
          return "";
        return doors_[idx].name;
      }

    private:

      std::vector<clingo_interface::Door> doors_;
      std::vector<std::string> locations_;
      std::vector<int32_t> location_map_;

      std::string door_yaml_file_;
      std::string location_file_;
      std::string map_file_;
      boost::shared_ptr <topological_mapper::MapLoader> mapper_;
      nav_msgs::MapMetaData info_;

      // boost::shared_ptr <navfn::NavfnROS> navfn_;
      // boost::shared_ptr <costmap_2d::Costmap2DROS> costmap_;
      // boost::shared_ptr<clingo_interface::CostmapDoorPlugin> door_plugin_;

  }; /* DoorHandler */
  
} /* clingo_interface */

#endif /* end of include guard: DOOR_HANDLER_WW75RJPS */

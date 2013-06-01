/**
 * \file  costmap_door_plugin.h
 * \brief  
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
 * $ Id: 05/30/2013 11:36:37 AM piyushk $
 *
 **/

#ifndef COSTMAP_DOOR_PLUGIN_CYIQSKZ0
#define COSTMAP_DOOR_PLUGIN_CYIQSKZ0

#include <costmap_2d/costmap_2d.h>
#include <costmap_2d/layer.h>
#include <costmap_2d/cost_values.h>
#include <clingo_interface/structures.h>
#include <topological_mapper/map_loader.h>
#include <topological_mapper/map_utils.h>

namespace clingo_interface {
    
  class CostmapDoorPlugin : public costmap_2d::Layer {

    public:
      virtual ~CostmapDoorPlugin();
      virtual void onInitialize();
      virtual void updateBounds(double origin_x, double origin_y, 
          double origin_yaw, double* min_x, double* min_y, double* max_x,
          double* max_y);
      virtual void updateCosts(costmap_2d::Costmap2D& master_grid, int min_i, 
          int min_j, int max_i, int max_j);
      virtual bool isDiscretized();
      virtual void matchSize();

      bool closeDoor(size_t idx) {
        boost::mutex::scoped_lock lock(door_plugin_mutex_);
        if (idx > doors_.size())
          return false;
        topological_mapper::Point2f image_pixels[2];
        image_pixels[0] = 
          topological_mapper::toGrid(doors_[idx].corners[0], info_);
        image_pixels[1] = 
          topological_mapper::toGrid(doors_[idx].corners[1], info_);
        drawLine(costmap_2d::LETHAL_OBSTACLE, image_pixels[0], image_pixels[1]);
        costmap_current_ = false;
        bound_left_ = std::min(bound_left_, doors_[idx].corners[0].x - 0.1);
        bound_left_ = std::min(bound_left_, doors_[idx].corners[1].x - 0.1);
        bound_right_ = std::max(bound_right_, doors_[idx].corners[0].x + 0.1);
        bound_right_ = std::max(bound_right_, doors_[idx].corners[1].x + 0.1);
        bound_up_ = std::min(bound_up_, doors_[idx].corners[0].y + 0.1);
        bound_up_ = std::min(bound_up_, doors_[idx].corners[1].y + 0.1);
        bound_down_ = std::max(bound_down_, doors_[idx].corners[0].y - 0.1);
        bound_down_ = std::max(bound_down_, doors_[idx].corners[1].y - 0.1);
        return true;
      }

      bool openDoor(size_t idx) {
        boost::mutex::scoped_lock lock(door_plugin_mutex_);
        if (idx > doors_.size())
          return false;
        topological_mapper::Point2f image_pixels[2];
        image_pixels[0] = 
          topological_mapper::toGrid(doors_[idx].corners[0], info_);
        image_pixels[1] = 
          topological_mapper::toGrid(doors_[idx].corners[1], info_);
        drawLine(costmap_2d::FREE_SPACE, image_pixels[0], image_pixels[1]);
        costmap_current_ = false;
        bound_left_ = std::min(bound_left_, doors_[idx].corners[0].x - 0.1);
        bound_left_ = std::min(bound_left_, doors_[idx].corners[1].x - 0.1);
        bound_right_ = std::max(bound_right_, doors_[idx].corners[0].x + 0.1);
        bound_right_ = std::max(bound_right_, doors_[idx].corners[1].x + 0.1);
        bound_up_ = std::min(bound_up_, doors_[idx].corners[0].y + 0.1);
        bound_up_ = std::min(bound_up_, doors_[idx].corners[1].y + 0.1);
        bound_down_ = std::max(bound_down_, doors_[idx].corners[0].y - 0.1);
        bound_down_ = std::max(bound_down_, doors_[idx].corners[1].y - 0.1);
        return true;
      }

      bool isCostmapCurrent() {
        return costmap_current_;
      }

      inline size_t getLocationIdx(const std::string& loc_str) const {
        for (size_t i = 0; i < locations_.size(); ++i) {
          if (locations_[i].name == loc_str) {
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
        return locations_[idx].name;
      }

      inline std::string getDoorString(size_t idx) const {
        if (idx >= doors_.size())
          return "";
        return doors_[idx].name;
      }

      std::vector<Door>& doors() {
        return doors_;
      }

      std::vector<Location>& locations() {
        return locations_;
      }

    protected:
      virtual void onFootprintChanged();

    private:

      inline void drawPixel(unsigned int x, unsigned int y, 
          unsigned char value) {
        int index = layered_costmap_->getCostmap()->getIndex(x, y);
        plugin_layer_value_[index] = value; 
      }

      inline void drawLine(unsigned char value, 
          topological_mapper::Point2f from, topological_mapper::Point2f to) {

        int x0 = from.x, y0 = from.y;
        int x1 = to.x, y1 = to.y;
        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);
        int sgnX = x0 < x1 ? 1 : -1;
        int sgnY = y0 < y1 ? 1 : -1;
        int e = 0;
        for (int i=0; i < dx+dy; i++) {
          drawPixel(x0, y0, value);
          int e1 = e + dy;
          int e2 = e - dx;
          if (abs(e1) < abs(e2)) {
            x0 += sgnX;
            e = e1;
          } else {
            y0 += sgnY;
            e = e2;
          }
        }

      }

      std::vector<clingo_interface::Door> doors_;
      std::vector<clingo_interface::Location> locations_;

      std::string door_yaml_file_;
      std::string location_file_;
      std::string map_file_;
      boost::shared_ptr <topological_mapper::MapLoader> mapper_;
      nav_msgs::MapMetaData info_;

      std::vector<unsigned char> plugin_layer_value_;
      bool costmap_current_;
      boost::mutex door_plugin_mutex_;

      double bound_left_, bound_right_, bound_up_, bound_down_;
  };

} /* clingo_interface */

#endif /* end of include guard: COSTMAP_DOOR_PLUGIN_CYIQSKZ0 */

/*
 *  Gazebo - Outdoor Multi-Robot Simulator
 *  Copyright (C) 2003
 *     Nate Koenig & Andrew Howard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 * Desc: 3D position interface for ground truth.
 * Author: Sachin Chitta and John Hsu
 * Date: 1 June 2008
 * SVN info: $Id$
 */

#include <bwi_gazebo_entities/gazebo_ros_video.h>

namespace gazebo
{


////////////////////////////////////////////////////////////////////////////////
// Constructor
GazeboRosVideo::GazeboRosVideo()
{
}

////////////////////////////////////////////////////////////////////////////////
// Destructor
GazeboRosVideo::~GazeboRosVideo()
{
}

////////////////////////////////////////////////////////////////////////////////
// Load the controller
void GazeboRosVideo::Load(rendering::VisualPtr _parent, sdf::ElementPtr _sdf )
{
  this->model = _parent;
  this->updateConnection = event::Events::ConnectPreRender(boost::bind(&GazeboRosVideo::UpdateChild, this));

  std::string _name = "laptop_visual";
  video_visual_.reset(new VideoVisual(_name, _parent, 50, 50));
}

////////////////////////////////////////////////////////////////////////////////
// Update the controller
void GazeboRosVideo::UpdateChild()
{
  cv::Mat image(50,50,CV_8UC3);
  video_visual_->render(image);
}

GZ_REGISTER_VISUAL_PLUGIN(GazeboRosVideo);

}

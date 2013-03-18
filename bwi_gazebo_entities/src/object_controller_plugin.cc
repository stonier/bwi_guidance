/*
    Copyright (c) 2010, Daniel Hewlett, Antons Rebguns
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

    THIS SOFTWARE IS PROVIDED BY Antons Rebguns <email> ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL Antons Rebguns <email> BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <bwi_gazebo_entities/object_controller_plugin.h>

#include <common/common.h>
#include <math/gzmath.h>
#include <physics/physics.h>
#include <sdf/sdf.h>

#include <ros/ros.h>
#include <tf/transform_broadcaster.h>
#include <tf/transform_listener.h>
#include <geometry_msgs/Twist.h>
#include <nav_msgs/GetMap.h>
#include <nav_msgs/Odometry.h>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>


namespace gazebo
{

// Constructor
ObjectControllerPlugin::ObjectControllerPlugin()
{
}

// Destructor
ObjectControllerPlugin::~ObjectControllerPlugin()
{
  delete rosnode_;
  delete transform_broadcaster_;
}

// Load the controller
void ObjectControllerPlugin::Load(physics::ModelPtr _parent, sdf::ElementPtr _sdf)
{
  this->parent = _parent;
  this->world = _parent->GetWorld();

  gzdbg << "plugin parent sensor name: " << parent->GetName() << "\n";

  if (!this->parent) { gzthrow("Object controller plugin requires a Model as its parent"); }

  if (!_sdf->HasElement("robotNamespace")) {
    ROS_WARN("Object controller plugin missing <robotNamespace>, defaults to \"\"");
    this->modelNamespace = "";
  } else {
    this->modelNamespace = _sdf->GetElement("robotNamespace")->GetValueString();
  }

  if (!_sdf->HasElement("topicName")) {
    ROS_WARN("Object controller plugin missing <topicName>, defaults to cmd_vel");
    this->topicName = "cmd_vel";
  } else {
    this->topicName = _sdf->GetElement("topicName")->GetValueString();
  }

  if (!_sdf->HasElement("updateRate")) {
    ROS_WARN("Object controller plugin missing <updateRate>, defaults to 100.0");
    this->updateRate = 100.0;
  } else {
    this->updateRate = _sdf->GetElement("updateRate")->GetValueDouble();
  }

  // In the simple model:
  //  - the collision model of the object is a cylinder.
  //  - the object is manually moved in world space
  //  - collisions with static objects are taken care of using an accurate map of the world
  //  - WARN! collisions with dynamic objects are not taken care. Some testing reveals they don't behave too badly, though they may.

  if (!_sdf->HasElement("mapTopic")) {
    ROS_WARN("Object controller plugin missing <mapTopic>, defaults to static_map");
    this->mapTopic = "map";
  } else {
    this->mapTopic = _sdf->GetElement("mapTopic")->GetValueString();
  }
  if (!_sdf->HasElement("modelRadius")) {
    ROS_WARN("Object controller Drive plugin missing <modelRadius>, defaults to 0.5");
    this->modelRadius = 0.5;
  } else {
    this->modelRadius = _sdf->GetElement("modelRadius")->GetValueDouble();
  }
  if (!_sdf->HasElement("modelPadding")) {
    ROS_WARN("Object controller Drive plugin missing <modelPadding>, defaults to 0.05");
    this->modelPadding = 0.05;
  } else {
    this->modelPadding = _sdf->GetElement("modelPadding")->GetValueDouble();
  }
  this->circumscribed_model_distance_ = this->modelPadding + this->modelRadius;

  // Initialize update rate stuff
  if (this->updateRate > 0.0) {
    this->update_period_ = 1.0 / this->updateRate;
  } else {
    this->update_period_ = 0.0;
  }
  last_update_time_ = this->world->GetSimTime();

  // Initialize velocity stuff
  last_odom_pose_ = this->parent->GetWorldPose();

  x_ = 0;
  y_ = 0;
  rot_ = 0;
  alive_ = true;

  // Initialize the ROS node and subscribe to cmd_vel
  int argc = 0;
  char** argv = NULL;
  ros::init(argc, argv, "botdrive_plugin", ros::init_options::NoSigintHandler | ros::init_options::AnonymousName);
  rosnode_ = new ros::NodeHandle(this->modelNamespace);

  ROS_INFO("Starting object controller plugin in ns: %s", this->modelNamespace.c_str());

  tf_prefix_ = tf::getPrefixParam(*rosnode_);
  transform_broadcaster_ = new tf::TransformBroadcaster();

  // ROS: Subscribe to the velocity command topic (usually "cmd_vel")
  ros::SubscribeOptions so =
      ros::SubscribeOptions::create<geometry_msgs::Twist>(topicName, 1,
                                                          boost::bind(&ObjectControllerPlugin::cmdVelCallback, this, _1),
                                                          ros::VoidPtr(), &queue_);

  sub_ = rosnode_->subscribe(so);

    ROS_INFO("Subscribing to %s", mapTopic.c_str());
    ros::SubscribeOptions so2 =
        ros::SubscribeOptions::create<nav_msgs::OccupancyGrid>(mapTopic, 1,
                                                            boost::bind(&ObjectControllerPlugin::getSimpleMap, this, _1),
                                                            ros::VoidPtr(), &queue_);
    sub2_ = rosnode_->subscribe(so2);
  map_available_ = false;

  pub_ = rosnode_->advertise<nav_msgs::Odometry>("odom", 1);
  pub2_ = rosnode_->advertise<nav_msgs::OccupancyGrid>("expanded_map", 1, true);

  teleport_service_server_ = rosnode_->advertiseService("teleport", 
      &ObjectControllerPlugin::teleport, this);

  // start custom queue for diff drive
  this->callback_queue_thread_ = boost::thread(boost::bind(&ObjectControllerPlugin::QueueThread, this));

  // listen to the update event (broadcast every simulation iteration)
  this->updateConnection = event::Events::ConnectWorldUpdateStart(boost::bind(&ObjectControllerPlugin::UpdateChild, this));


}

// Update the controller
void ObjectControllerPlugin::UpdateChild()
{
  common::Time current_time = this->world->GetSimTime();
  double seconds_since_last_update = (current_time - last_update_time_).Double();
  if (seconds_since_last_update > update_period_) {

    writePositionData(seconds_since_last_update);
    publishOdometry(seconds_since_last_update);

    last_update_time_+= common::Time(update_period_);

  }
}

// Finalize the controller
void ObjectControllerPlugin::FiniChild()
{
  alive_ = false;
  queue_.clear();
  queue_.disable();
  rosnode_->shutdown();
  callback_queue_thread_.join();
}

bool ObjectControllerPlugin::teleport(bwi_msgs::Teleport::Request &req,
    bwi_msgs::Teleport::Response &resp) {

  // TODO probably need a transformation here
  geometry_msgs::Pose &pose = req.location;
  math::Vector3 new_vec(pose.position.x, pose.position.y, pose.position.z);
  math::Quaternion new_quat(pose.orientation.w, pose.orientation.x, 
      pose.orientation.y, pose.orientation.z);
  math::Pose new_pose(new_vec, new_quat);

  // Check if the new pose can be allowed
  int x_pxl = 
    (new_pose.pos.x - map_.info.origin.position.x) / map_.info.resolution;
  int y_pxl = 
    (new_pose.pos.y - map_.info.origin.position.y) / map_.info.resolution;

  if (x_pxl < 0 || x_pxl >= (int)map_.info.width ||
      y_pxl < 0 || y_pxl >= (int)map_.info.height ||
      (map_.data[y_pxl * map_.info.width + x_pxl] < 50)) {
    this->parent->SetWorldPose(new_pose);
    resp.success = true;
  } else {
    resp.success = false;
    resp.error_msg = "Given location inside obstacle space.";
  } 
  
  return true;
}

void ObjectControllerPlugin::cmdVelCallback(const geometry_msgs::Twist::ConstPtr& cmd_msg)
{
  boost::mutex::scoped_lock scoped_lock(lock);
  x_ = cmd_msg->linear.x;
  y_ = cmd_msg->linear.y;
  rot_ = cmd_msg->angular.z;
}

void ObjectControllerPlugin::QueueThread()
{
  static const double timeout = 0.01;

  while (alive_ && rosnode_->ok())
  {
    queue_.callAvailable(ros::WallDuration(timeout));
  }
}

void ObjectControllerPlugin::getSimpleMap(const nav_msgs::OccupancyGrid::ConstPtr& map) {

  ROS_INFO("Map received.");

  map_.header = map->header;

  map_.info.map_load_time = map->info.map_load_time;
  map_.info.resolution = map->info.resolution;
  map_.info.width = map->info.width;
  map_.info.height = map->info.height;

  // Get the map origin in global space
  // useful for multi_level_map
  tf::TransformListener listener;
  geometry_msgs::PoseStamped pose_in, pose_out;
  pose_in.header = map->header;
  pose_in.pose = map->info.origin;

  map_.info.origin = pose_in.pose;
  // TODO: not sure why I wrote in this if condition... It seems to be a bit messed up
  if (map->header.frame_id != "map" && map->header.frame_id != "/map") {
    if (listener.waitForTransform(map->header.frame_id, "/map", ros::Time(0), ros::Duration(5.0))) {
      ROS_INFO("Transformation for simple map acquired");
      listener.transformPose("/map", ros::Time(0), pose_in, pose_in.header.frame_id, pose_out);
      map_.info.origin = pose_out.pose;
    } else {
      ROS_ERROR("Unable to get transformation from /map to %s.", map->header.frame_id.c_str());
    }
  }

  // expand the map out based on the circumscribed model distance
  int expand_pixels = ceil(circumscribed_model_distance_ / map->info.resolution);
  map_.data.resize(map->info.height * map->info.width);
  for (int i = 0; i < (int)map->info.height; ++i) {
    for (int j = 0; j < (int)map->info.width; ++j) {
      int low_i = (i - expand_pixels < 0) ? 0 : i - expand_pixels;
      int high_i = (i + expand_pixels >= (int)map->info.height) ? 
          map->info.height - 1 : i + expand_pixels;
      int max = 0;
      for (int k = low_i; k <= high_i; ++k) {
        int diff_j = floor(sqrtf(expand_pixels * expand_pixels - (i - k) * (i - k)));
        int low_j = (j - diff_j < 0) ? 0 : j - diff_j;
        int high_j = (j + diff_j >= (int)map->info.width) ? 
            map->info.width - 1 : j + diff_j;
        for (int l = low_j; l <= high_j; ++l) {
          if (map->data[k * map->info.width + l] > max) {
            max = map->data[k * map->info.width + l];
          }
        }
      }
      map_.data[i * map->info.width + j] = max;
    }
  }

  pub2_.publish(map_);
  map_available_ = true;

  ROS_INFO("Map processed.");
}

void ObjectControllerPlugin::publishOdometry(double step_time)
{
  ros::Time current_time = ros::Time::now();
  std::string odom_frame = tf::resolve(tf_prefix_, "odom");
  std::string base_footprint_frame = tf::resolve(tf_prefix_, "base_footprint");

  // getting data for base_footprint to odom transform
  math::Pose pose = this->parent->GetWorldPose();

  tf::Quaternion qt(pose.rot.x, pose.rot.y, pose.rot.z, pose.rot.w);
  tf::Vector3 vt(pose.pos.x, pose.pos.y, pose.pos.z);

  tf::Transform base_footprint_to_odom(qt, vt);
  transform_broadcaster_->sendTransform(tf::StampedTransform(base_footprint_to_odom,
                                                             current_time,
                                                             odom_frame,
                                                             base_footprint_frame));

  // publish odom topic
  odom_.pose.pose.position.x = pose.pos.x;
  odom_.pose.pose.position.y = pose.pos.y;

  odom_.pose.pose.orientation.x = pose.rot.x;
  odom_.pose.pose.orientation.y = pose.rot.y;
  odom_.pose.pose.orientation.z = pose.rot.z;
  odom_.pose.pose.orientation.w = pose.rot.w;
  odom_.pose.covariance[0] = 0.00001;
  odom_.pose.covariance[7] = 0.00001;
  odom_.pose.covariance[14] = 1000000000000.0;
  odom_.pose.covariance[21] = 1000000000000.0;
  odom_.pose.covariance[28] = 1000000000000.0;
  odom_.pose.covariance[35] = 0.001;

  // get velocity in /odom frame
  math::Vector3 linear;
    // Getting values from the worlds model in gazebo instead of supplied
    // velocites as a simple means of error correction
    linear.x = (pose.pos.x - last_odom_pose_.pos.x) / step_time;
    linear.y = (pose.pos.y - last_odom_pose_.pos.y) / step_time;
    boost::mutex::scoped_lock scoped_lock(lock);
    if (rot_ > M_PI / step_time) { // we cannot calculate the angular velocity correctly
      odom_.twist.twist.angular.z = rot_;
    } else {
      float last_yaw = last_odom_pose_.rot.GetYaw();
      float current_yaw = pose.rot.GetYaw();
      while (current_yaw < last_yaw - M_PI) current_yaw += 2*M_PI;
      while (current_yaw > last_yaw + M_PI) current_yaw -= 2*M_PI;
      float angular_diff = current_yaw - last_yaw;
      odom_.twist.twist.angular.z = angular_diff / step_time;
    }
    last_odom_pose_ = pose;

  // convert velocity to child_frame_id (aka base_footprint)
  float yaw = pose.rot.GetYaw();
  odom_.twist.twist.linear.x = cosf(yaw) * linear.x + sinf(yaw) * linear.y;
  odom_.twist.twist.linear.y = cosf(yaw) * linear.y - sinf(yaw) * linear.x;

  odom_.header.stamp = current_time;
  odom_.header.frame_id = odom_frame;
  odom_.child_frame_id = base_footprint_frame;

  pub_.publish(odom_);
}

// Update the data in the interface
void ObjectControllerPlugin::writePositionData(double step_time)
{
  // move the simple model manually
  if (map_available_) {

    boost::mutex::scoped_lock scoped_lock(lock);
    float dr = x_ * step_time;
    float dy = y_ * step_time;
    float da = rot_ * step_time;

    //ROS_INFO_THROTTLE(1.0,"%f %f %f", dr, dy, da);

    math::Pose orig_pose = this->parent->GetWorldPose();
    math::Pose new_pose = orig_pose;
    new_pose.pos.x = orig_pose.pos.x 
                   + dr * cos(orig_pose.rot.GetYaw()) 
                   - dy * sin(orig_pose.rot.GetYaw());
    new_pose.pos.y = orig_pose.pos.y 
                   + dr * sin(orig_pose.rot.GetYaw()) 
                   + dy * cos(orig_pose.rot.GetYaw());
    new_pose.rot.SetFromEuler(math::Vector3(0,0,orig_pose.rot.GetYaw() + da));

    // Check if the new pose can be allowed
    int x_pxl = (new_pose.pos.x - map_.info.origin.position.x) / map_.info.resolution;
    int y_pxl = (new_pose.pos.y - map_.info.origin.position.y) / map_.info.resolution;

    if (x_pxl < 0 || x_pxl >= (int)map_.info.width ||
        y_pxl < 0 || y_pxl >= (int)map_.info.height ||
        (map_.data[y_pxl * map_.info.width + x_pxl] < 50)) {
      this->parent->SetWorldPose(new_pose);
    } else {
      ROS_DEBUG_THROTTLE(2.0,"Simple Segbot plugin is preventing running into a wall. If you think this is an error, then check the map");
    } 
  } else {
    ROS_WARN_THROTTLE(2.0,"BWI Object Controller plugin has not received a map, cannot move the object");

  }

}

GZ_REGISTER_MODEL_PLUGIN(ObjectControllerPlugin)
}


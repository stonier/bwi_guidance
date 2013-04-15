/**
 * \file  view_data.cpp
 * \brief  Plays the data recorded from the experiments
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
 * $ Id: 04/15/2013 10:44:56 AM piyushk $
 *
 **/

#include <boost/filesystem.hpp>
#include <topological_mapper/map_loader.h>

std::string map_file;
std::string graph_file;
std::string control_experiments_file;
std::string test_experiments_file;

int main(int argc, const char *argv[]) {

  ros::init(argc, argv, "view_data");

  ros::param::get("~map_file", map_file);
  ros::param::get("~graph_file", graph_file);
  ros::param::get("~control_experiments_file", control_experiments_file);
  ros::param::get("~test_experiments_file", test_experiments_file);
  ros::param::get("~data_directory", data_directory);

  // Read in everything here

  topological_mapper::MapLoader mapper(map_file);

  
  createTrackbar(Track, "Display", &seconds, alpha_slider_max, onSecondsTrackbarChange);
  
  return 0;
}

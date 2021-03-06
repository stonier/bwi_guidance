# http://ros.org/doc/groovy/api/catkin/html/user_guide/supposed.html
cmake_minimum_required(VERSION 2.8.3)
project(bwi_guidance_visualizer)
find_package(catkin REQUIRED COMPONENTS bwi_mapper bwi_guidance)

catkin_package(
  DEPENDS qt4
  CATKIN_DEPENDS bwi_mapper bwi_guidance 
  INCLUDE_DIRS include
)

include_directories(include ${catkin_INCLUDE_DIRS})

##############################################################################
# Qt Environment
##############################################################################

find_package(Qt4 COMPONENTS QtCore QtGui REQUIRED)
include(${QT_USE_FILE})

file(GLOB QT_FORMS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ui/*.ui)
file(GLOB_RECURSE QT_MOC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} FOLLOW_SYMLINKS include/bwi_guidance_visualizer/*.hpp)

qt4_wrap_ui(QT_FORMS_HPP ${QT_FORMS})
qt4_wrap_cpp(QT_MOC_HPP ${QT_MOC})

#To locate generate ui headers
include_directories(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB_RECURSE QT_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} FOLLOW_SYMLINKS src/*.cpp)

##############################################################################
# Binaries
##############################################################################

## add_executable(bwi_guidance_visualizer ${QT_SOURCES} ${QT_FORMS_HPP} ${QT_MOC_HPP})
## target_link_libraries(bwi_guidance_visualizer 
##   ${catkin_LIBRARIES}
##   ${QT_QTCORE_LIBRARY} 
##   ${QT_QTGUI_LIBRARY}
## )

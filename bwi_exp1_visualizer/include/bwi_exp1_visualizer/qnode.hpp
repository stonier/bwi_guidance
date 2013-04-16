/**
 * @file /include/bwi_exp1_visualizer/qnode.hpp
 *
 * @brief Communications central!
 *
 * @date February 2011
 **/
/*****************************************************************************
** Ifdefs
*****************************************************************************/

#ifndef bwi_exp1_visualizer_QNODE_HPP_
#define bwi_exp1_visualizer_QNODE_HPP_

/*****************************************************************************
** Includes
*****************************************************************************/

#include <ros/ros.h>
#include <string>
#include <QThread>
#include <QStringListModel>

/*****************************************************************************
** Namespaces
*****************************************************************************/

namespace bwi_exp1_visualizer {

/*****************************************************************************
** Class
*****************************************************************************/

  class QNode : public QThread {
    Q_OBJECT

    public:
      QNode(int argc, char** argv);
      virtual ~QNode();
      bool init();
      void run();
      void updateFrameInfo(double time_step);

      std::vector<QString> experiment_box_strings_;
      std::vector<QString> user_box_strings_;
      double max_experiment_time_;
      int max_time_steps_;
      double current_experiment_time_;
      int current_time_step_;
      double time_step_;
      cv::Mat generated_image_;

    public slots:
      void on_timeSlider_valueChanged(int value);
      void on_autoPlayBox_toggled(bool checked);
      void on_experimentBox_currentIndexChanged(int index);
      void on_userBox_currentIndexChanged(int index);

    signals:
      void incrementTime();
      void updateFrameInfo();
      void rosShutdown();

    private:
      bool autoplay;
      int init_argc;
      char** init_argv;

  };

}  // namespace bwi_exp1_visualizer

#endif /* bwi_exp1_visualizer_QNODE_HPP_ */

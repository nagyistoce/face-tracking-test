#ifndef __DETECTOR_HPP__
# define __DETECTOR_HPP__

# include "subsystem.hpp"

# include <opencv2/objdetect/objdetect.hpp>
# include <boost/thread.hpp>

class Detector : public Subsystem
{
public:
    std::vector<cv::Rect> faces();

    virtual void start();
    virtual void stop();

private:
    friend class Master;
    explicit Detector(Master *master);

    void detect_thread();

    boost::thread _detect_thr;

    cv::CascadeClassifier _face_cascade;

    std::vector<cv::Rect> _faces;
    boost::mutex _guard;
};


#endif //__DETECTOR_HPP__

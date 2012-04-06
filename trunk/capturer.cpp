#include "capturer.hpp"
#include "master.hpp"

Capturer::Capturer(Master *master)
    : Subsystem(master)
{
    _capture = cvCaptureFromCAM(-1);
    //_capture = cvCaptureFromFile("test1.mp4");
    if (!_capture)
    {
        throw std::runtime_error("Unable capture video"); 
    }
}

Capturer::~Capturer()
{
    cvReleaseCapture(&_capture);
}

cv::Mat Capturer::frame()
{
    boost::mutex::scoped_lock g(_guard);
    IplImage *raw = cvQueryFrame(_capture);
    return raw ? cv::Mat(raw) : cv::Mat();
}

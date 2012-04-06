#include "detector.hpp"
#include "capturer.hpp"
#include "master.hpp"

#include <opencv2/imgproc/imgproc.hpp>
#include <stdexcept>

Detector::Detector(Master *master)
    : Subsystem(master)
{
    //if (!_face_cascade.load("haarcascade_frontalface_alt.xml"))
    if (!_face_cascade.load("lbpcascade_frontalface.xml"))
    { 
        throw std::runtime_error("Unable to load face template"); 
    }
}

void Detector::start()
{
    _detect_thr = boost::thread(&Detector::detect_thread, this);
}

void Detector::stop()
{
    try
    {
        if (_detect_thr.joinable())
        {
            _detect_thr.interrupt();
            _detect_thr.join();
        }
    }
    catch(...)
    {
    }
}

std::vector<cv::Rect> Detector::faces()
{
    boost::mutex::scoped_lock g(_guard);
    return _faces;
}

void Detector::detect_thread()
{
    try
    {
        while (true)
        {
            boost::this_thread::interruption_point();

            cv::Mat frame = master().subsystem<Capturer>().frame();

            if (frame.empty())
            {
                boost::this_thread::interruptible_wait(100u);
                continue;
            }

            cv::Mat frame_gray;

            cv::cvtColor(frame, frame_gray, CV_BGR2GRAY);
            cv::equalizeHist(frame_gray, frame_gray);

            //-- Detect faces
            std::vector<cv::Rect> faces;
            _face_cascade.detectMultiScale(frame_gray, faces, 1.1, 2, CV_HAAR_SCALE_IMAGE, cv::Size(30, 30));

            boost::mutex::scoped_lock g(_guard);
            _faces.swap(faces);
        }
    }
    catch(...)
    {
    }
}


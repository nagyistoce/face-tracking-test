#ifndef __CAPTURER_HPP__
# define __CAPTURER_HPP__

# include "subsystem.hpp"

# include <opencv2/highgui/highgui.hpp>
# include <boost/thread.hpp>

class Capturer : public Subsystem
{
public:
    cv::Mat frame();

    ~Capturer();
private:
    friend class Master;
    explicit Capturer(Master *master);

    CvCapture *_capture;
    boost::mutex _guard;
};


#endif //__CAPTURER_HPP__

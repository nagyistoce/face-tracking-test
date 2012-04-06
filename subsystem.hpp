#ifndef __SUBSYSTEM_HPP__
# define __SUBSYSTEM_HPP__

# include <boost/noncopyable.hpp>

class Master;

class Subsystem : boost::noncopyable
{
public:
    Master & master();

    virtual void start() {}
    virtual void stop() {}

    virtual ~Subsystem() = 0;

protected:
    explicit Subsystem(Master *master);

private:
    Master *_master;
};

#endif //__SUBSYSTEM_HPP__

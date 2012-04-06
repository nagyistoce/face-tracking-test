#include "master.hpp"
#include <boost/foreach.hpp>

void Master::start()
{
    BOOST_FOREACH(holder_type::value_type &sys_ptr, _subsystem_holder)
    {
        sys_ptr->start();
    }
}

Master::~Master()
{
    BOOST_FOREACH(holder_type::value_type &sys_ptr, _subsystem_holder)
    {
        sys_ptr->stop();
    }

    BOOST_FOREACH(holder_type::value_type &sys_ptr, _subsystem_holder)
    {
        delete sys_ptr;
    }
}

#include "master.hpp"

void Master::start()
{
    for (holder_type::iterator it = _subsystem_holder.begin(); it != _subsystem_holder.end(); ++it)
    {
        (*it)->start();
    }
}

Master::~Master()
{
    for (holder_type::iterator it = _subsystem_holder.begin(); it != _subsystem_holder.end(); ++it)
    {
        (*it)->stop();
    }

    for (holder_type::iterator it = _subsystem_holder.begin(); it != _subsystem_holder.end(); ++it)
    {
        delete *it;
    }
}

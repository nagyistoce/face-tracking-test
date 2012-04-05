#include "master.hpp"

Master::~Master()
{
    for (holder_type::iterator it = _subsystem_holder.begin(); it != _subsystem_holder.end(); ++it)
    {
        delete *it;
    }
}

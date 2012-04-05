#include "subsystem.hpp"

Master & Subsystem::master()
{
    return *_master;
}

Subsystem::~Subsystem()
{
}

Subsystem::Subsystem(Master *master)
    : _master(master)
{
}

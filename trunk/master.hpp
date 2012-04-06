#ifndef __MASTER_HPP__
# define __MASTER_HPP__

# include "subsystem.hpp"

# include <boost/noncopyable.hpp>
# include <vector>
# include <cassert>

class Master : boost::noncopyable
{
public:
    template <typename SubsystemType>
    inline void add_subsystem();

    template <typename SubsystemType>
    inline SubsystemType & subsystem();

    void start();

    ~Master();

private:

    template <typename SubsystemType>
    inline SubsystemType * & subsystem_instance();

    typedef std::vector<Subsystem *> holder_type;
    holder_type _subsystem_holder;
};

// Implementation

template <typename SubsystemType>
inline SubsystemType * & Master::subsystem_instance()
{
    static SubsystemType *instance = 0;
    return instance;
}

template <typename SubsystemType>
inline void Master::add_subsystem()
{
    SubsystemType * &instance = subsystem_instance<SubsystemType>();
    assert(instance == 0);

    instance = new SubsystemType(this);
    _subsystem_holder.push_back(instance);
}

template <typename SubsystemType>
inline SubsystemType & Master::subsystem()
{
    SubsystemType * &instance = subsystem_instance<SubsystemType>();
    assert(instance != 0);
   
    return *instance;
}

#endif //__MASTER_HPP__
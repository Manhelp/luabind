// Boost Software License http://www.boost.org/LICENSE_1_0.txt
// Copyright (c) 2009 The Luabind Authors

#include "test.hpp"
#include <luabind/luabind.hpp>

struct Base
{
    Base(int value)
      : value(value)
    {}

    virtual ~Base()
    {}

    int g() const
    {
        return value;
    }

    int value;
};

struct Derived : Base
{
    Derived()
      : Base(2)
    {}

    int f() const
    {
        return 1;
    }
};

struct Unregistered : Base
{
    Unregistered()
      : Base(3)
    {}
};

luabind::unique_ptr<Base> make_derived()
{
    return luabind::unique_ptr<Base>(luabind::luabind_new<Derived>());
}

luabind::unique_ptr<Base> make_unregistered()
{
    return luabind::unique_ptr<Base>(luabind::luabind_new<Unregistered>());
}

TEST_CASE("dynamic_type")
{
    using namespace luabind;

    module(L) [
        class_<Base>("Base")
            .def("g", &Base::g),
        class_<Derived, Base>("Derived")
            .def("f", &Derived::f),
        def("make_derived", &make_derived),
        def("make_unregistered", &make_unregistered)
    ];

    DOSTRING(L,
        "x = make_derived()\n"
        "assert(x:f() == 1)\n"
    );

    DOSTRING(L,
        "x = make_unregistered()\n"
        "assert(x:g() == 3)\n"
    );
}


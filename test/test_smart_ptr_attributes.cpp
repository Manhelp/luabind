// Boost Software License http://www.boost.org/LICENSE_1_0.txt
// Copyright (c) 2009 The Luabind Authors

#include "test.hpp"
#include <luabind/luabind.hpp>
#include <memory>
#include <luabind/shared_ptr_converter.hpp>
#include <luabind/class_info.hpp>

struct Foo
{
    Foo() : m_baz(0) {}
    int m_baz;
};

struct Bar
{
    std::shared_ptr<Foo> getFoo() const { return m_foo; }
    void setFoo( std::shared_ptr<Foo> foo ) {  m_foo = foo; }

    std::shared_ptr<Foo> m_foo;
};

void test_main(lua_State* L)
{
    using namespace luabind;

    bind_class_info(L);

    module( L )
    [
        class_<Foo, no_bases, std::shared_ptr<Foo> >( "Foo" )
            .def( constructor<>() )
            .def_readwrite("baz", &Foo::m_baz),
        class_<Bar, no_bases, std::shared_ptr<Bar> >( "Bar" )
            .def( constructor<>() )
            .property("fooz", &Bar::getFoo, &Bar::setFoo)
            .def_readwrite("foo", &Bar::m_foo)
    ];
    dostring( L, "foo = Foo();");
    dostring( L,    "foo.baz = 42;");
    dostring( L,    "x = Bar();");
    dostring( L,    "x.fooz = foo;");
    dostring( L,    "print(type(x), class_info(x).name);");
    dostring( L,    "print(type(x.fooz), class_info(x.fooz).name);");
    dostring( L,    "print(type(x.foo), class_info(x.foo).name);"); // crashes here);
}


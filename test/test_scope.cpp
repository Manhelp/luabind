// Boost Software License http://www.boost.org/LICENSE_1_0.txt
// Copyright (c) 2004 The Luabind Authors

#include "test.hpp"
#include <luabind/luabind.hpp>
#include <luabind/yield_policy.hpp>     // for yield

int f() { return 1; }
int f_(int /*a*/) { return 2; }
int f__(int /*a*/) { return 3; }
int g() { return 4; }
int g_(int)	{ return 5; }
int h() { return 6; }

struct test_class : counted_type<test_class>
{
	test_class()
	  : test(1)
	{}
	int test;
};

struct test_class2 : counted_type<test_class2>
{
	test_class2() {}
	int string_string(luabind::string const& /*s1*/, luabind::string const& /*s2*/)
	{ return 1; }
};

struct test_class_unnamed : counted_type<test_class_unnamed>
{
	test_class_unnamed() {}
	int f() { return 42; }
};

test_class_unnamed create_unnamed()
{
	return test_class_unnamed();
}

COUNTER_GUARD(test_class);
COUNTER_GUARD(test_class2);
COUNTER_GUARD(test_class_unnamed);

void test_main(lua_State* L)
{
	using namespace luabind;

	module(L)
	[
		class_<test_class2>("test_class2")
				.def(constructor<>())
				.def("string_string", &test_class2::string_string)
	];

	module(L, "test")
	[
		class_<test_class>("test_class")
			.def(constructor<>())
			.def_readonly("test", &test_class::test)
			.scope
			[
				def("inner_fun", &f)
			]
            .def("inner_fun2", &f) // this should become static
			.enum_("vals")
			[
				value("val1", 1),
				value("val2", 2)
			],

		def("f", &f),
		def("f", &f_),

		namespace_("inner")
		[
			def("g", &g),
			def("f", &f__)
		],

		namespace_("inner")
		[
			def("g", &g_)
		],

		class_<test_class_unnamed>()
			.def(constructor<>())
			.def("f", &test_class_unnamed::f),

		def("create_unnamed", &create_unnamed)
	];

	module(L, "test")
	[
		namespace_("inner")
		[
			def("h", &h)
		]
	];

	object test_obj = newtable(L);
	module(test_obj)
	[
		namespace_("inner")
		[
			def("h", &h)
		]
	];

	DOSTRING(L, "assert(test.f() == 1)");
	DOSTRING(L, "assert(test.f(3) == 2)");
	DOSTRING(L, "assert(test.test_class.inner_fun() == 1)");
    DOSTRING(L,
		"a = test.test_class()\n"
		"assert(a.test == 1)");
	DOSTRING(L, "assert(a.inner_fun2() == 1)"); // free function
    DOSTRING(L,
		"b = test.test_class.val2\n"
		"assert(b == 2)");
	DOSTRING(L, "assert(test.inner.g() == 4)");
	DOSTRING(L, "assert(test.inner.g(7) == 5)");
	DOSTRING(L, "assert(test.inner.f(4) == 3)");
	DOSTRING(L, "assert(test.inner.h() == 6)");

	globals(L)["test_object"] = test_obj;
	DOSTRING(L, "assert(not inner)");
	DOSTRING(L, "assert(test_object.inner.h() == 6)");

	DOSTRING(L, "assert(not test_class_unnamed)");
	DOSTRING(L,
		"u = test.create_unnamed()\n"
		"assert(u:f() == 42)");

	DOSTRING_EXPECTED(L, "_ = test_class2.foo",
		"no static 'foo' in class 'test_class2'");
	DOSTRING_EXPECTED(L, "_ = test.test_class['val1\\0foo']",
		"no static 'val1' (followed by embedded 0) in class 'test_class'");
	DOSTRING_EXPECTED(L, "_ = test_class2[{}]",
		"no static value at table-index in class 'test_class2'");
}


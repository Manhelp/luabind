// Boost Software License http://www.boost.org/LICENSE_1_0.txt
// Copyright (c) 2012 The Luabind Authors

#include <luabind/function_introspection.hpp>
#include <luabind/config.hpp>
#include <luabind/wrapper_base.hpp>
#include <luabind/detail/call_member.hpp>
#include <luabind/detail/stack_utils.hpp> // for stack_pop
#include <luabind/function.hpp> // for def, is_luabind_function
#include <luabind/detail/call.hpp> // for function_object
#include <luabind/detail/object.hpp> // for argument, object, etc
#include <luabind/from_stack.hpp> // for from_stack
#include <luabind/scope.hpp> // for module, module_, scope
#include <luabind/lua_argument_proxy.hpp>
#include <luabind/detail/conversion_policies/conversion_policies.hpp>
#include <string> // for string
#include <cstddef> // for NULL

namespace luabind
{
	static detail::function_object* get_function_object(argument const& fn)
	{
		lua_State* L = fn.interpreter();
		{
			fn.push(L);
			detail::stack_pop pop(L, 1);
			if (!detail::is_luabind_function(L, -1))
				return nullptr;
		}
		return *touserdata<detail::function_object*>(std::get<1>(getupvalue(fn, 1)));
	}

	static luabind::string get_function_name(argument const& fn)
	{
		detail::function_object* f = get_function_object(fn);
		if (!f)
			return "";
		return f->name;
	}

	static object get_function_overloads(argument const& fn)
	{
		lua_State* L = fn.interpreter();
		detail::function_object* fobj = get_function_object(fn);
		if (!fobj)
			return object();
		object overload_table(newtable(L));
		int count = 1;
		char const* function_name = fobj->name.c_str();
		for (detail::function_object const* f = fobj; f; f = f->next)
		{
			f->format_signature(L, function_name);
			detail::stack_pop pop(L, 1);
			overload_table[count] = object(from_stack(L, -1));
			count++;
		}
		return overload_table;
	}

	int bind_function_introspection(lua_State* L)
	{
		lua_getglobal(L, "function_info");
		detail::stack_pop pop(L, 1);
		if (lua_istable(L, -1))
			return 0; // already registered
		module(L, "function_info")
		[
			def("get_function_overloads", &get_function_overloads),
			def("get_function_name", &get_function_name)
		];
		return 0;
	}
} // namespace luabind

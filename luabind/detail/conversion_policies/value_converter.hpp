// Boost Software License http://www.boost.org/LICENSE_1_0.txt
// Copyright (c) 2013 The Luabind Authors

#ifndef LUABIND_VALUE_CONVERTER_HPP_INCLUDED
#define LUABIND_VALUE_CONVERTER_HPP_INCLUDED

#include <type_traits>
#include <luabind/lua_include.hpp>
#include <luabind/back_reference.hpp>
#include <luabind/detail/object_rep.hpp>

namespace luabind {
	namespace detail {

		struct value_converter
		{
			using type      = value_converter;
			using is_native = std::false_type;

			enum { consumed_args = 1 };

			value_converter()
				: result(0)
			{}

			void* result;

			template<class T>
			void to_lua(lua_State* L, T&& x)
			{
				if(luabind::get_back_reference(L, x))
					return;

				make_value_instance(L, std::forward<T>(x));
			}

			template<class T>
			T to_cpp(lua_State*, by_value<T>, int)
			{
				return *static_cast<T*>(result);
			}

			template<class T>
			int match(lua_State* L, by_value<T>, int index)
			{
				// special case if we get nil in, try to match the holder type
				if(lua_isnil(L, index))
					return no_match;

				object_rep* obj = get_instance(L, index);
				if(obj == 0) return no_match;

				std::pair<void*, int> s = obj->get_instance(registered_class<T>::id);
				result = s.first;
				return s.second;
			}

			template<class T>
			void converter_postcall(lua_State*, T, int) {}
		};

	}
}

#endif


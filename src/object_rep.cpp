// Copyright (c) 2003 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#define LUABIND_BUILDING

#include <luabind/detail/object_rep.hpp>
#include <luabind/detail/class_rep.hpp>
#include <cstring>

namespace luabind {
	namespace detail {

		// dest is a function that is called to delete the c++ object this struct holds
		object_rep::object_rep(instance_holder* instance, class_rep* crep)
			: m_instance(instance)
			, m_classrep(crep)
		{
		}

		object_rep::~object_rep()
		{
			if(!m_instance)
				return;
			m_instance->~instance_holder();
			deallocate(m_instance);
		}

		void object_rep::add_dependency(lua_State* L, int index)
		{
			if(!m_dependency_ref.is_valid())
			{
				lua_newtable(L);
				m_dependency_ref.pop(L);
			}
			m_dependency_ref.push(L);

			lua_pushvalue(L, index);
			lua_pushnumber(L, 0);
			lua_rawset(L, -3);
			lua_pop(L, 1);
		}

		int destroy_instance(lua_State* L)
		{
			object_rep* instance = static_cast<object_rep*>(lua_touserdata(L, 1));

			lua_pushliteral(L, "__finalize");
			lua_gettable(L, 1);

			if(lua_isnil(L, -1))
			{
				lua_pop(L, 1);
			}
			else
			{
				lua_pushvalue(L, 1);
				lua_call(L, 1, 0);
			}

			instance->~object_rep();

			lua_pushnil(L);
			lua_setmetatable(L, 1);
			return 0;
		}

		namespace
		{

			int set_instance_value(lua_State* L)
			{
				lua_getuservalue(L, 1);
				lua_pushvalue(L, 2);
				lua_rawget(L, -2);

				if(lua_isnil(L, -1) && lua_getmetatable(L, -2))
				{
					lua_pushvalue(L, 2);
					lua_rawget(L, -2);
					lua_replace(L, -3);
					lua_pop(L, 1);
				}

				if(lua_tocfunction(L, -1) == &property_tag && lua_tocfunction(L, 3) != &property_tag)
				{
					// this member is a property, extract the "set" function and call it.
					lua_getupvalue(L, -1, 2);

					if(lua_isnil(L, -1))
					{
						lua_pushfstring(L, "property '%s' is read only", lua_tostring(L, 2));
						lua_error(L);
					}

					lua_pushvalue(L, 1);
					lua_pushvalue(L, 3);
					lua_call(L, 2, 0);
					return 0;
				}
				else if (lua_isnil(L, -1))
				{
					// if a metatable exists we need to use that
					if (!lua_getmetatable(L, -2))
					{
						// no metatable, push user value to the right position
						lua_pushvalue(L, -2);
					}

					// value not known, check the __newindex function
					lua_pushliteral(L, "__newindex");
					lua_rawget(L, -2);

					if (!lua_isnil(L, -1))
					{
						// we hava an index function, hopefully it's actually a function...
						lua_pushvalue(L, 1);
						lua_pushvalue(L, 2);
						lua_pushvalue(L, 3);
						lua_call(L, 3, 0);
					}
					else
					{
						lua_pop(L, 1);
					}
					// pop the table again
					lua_pop(L, 1);
				}

				lua_pop(L, 1);

				if(!lua_getmetatable(L, 4))
				{
					lua_newtable(L);
					lua_pushvalue(L, -1);
					lua_setuservalue(L, 1);
					lua_pushvalue(L, 4);
					lua_setmetatable(L, -2);
				}
				else
				{
					lua_pop(L, 1);
				}

				lua_pushvalue(L, 2);
				lua_pushvalue(L, 3);
				lua_rawset(L, -3);

				return 0;
			}

			int get_instance_value(lua_State* L)
			{
				lua_getuservalue(L, 1);
				lua_pushvalue(L, 2);
				lua_rawget(L, -2);

				if(lua_isnil(L, -1) && lua_getmetatable(L, -2))
				{
					lua_pushvalue(L, 2);
					lua_rawget(L, -2);
				}

				if(lua_tocfunction(L, -1) == &property_tag)
				{
					// this member is a property, extract the "get" function and call it.
					lua_getupvalue(L, -1, 1);
					lua_pushvalue(L, 1);
					lua_call(L, 1, 1);
				}
				else if (lua_isnil(L, -1))
				{
					// we don't want to handle __finalize
					if (lua_isstring(L, 2) && !lua_isnumber(L, 2))
					{
						if (!strcmp(lua_tostring(L, 2), "__finalize"))
						{
							return 1;
						}
					}

					// value not known, check the __index function
					lua_pushliteral(L, "__index");
					lua_rawget(L, -3);

					if (!lua_isnil(L, -1))
					{
						// we have an index function
						lua_pushvalue(L, 1);
						lua_pushvalue(L, 2);
						lua_call(L, 2, 1);
					}
					else
					{
						lua_pop(L, 1);
					}
				}

				return 1;
			}

			int dispatch_operator(lua_State* L)
			{
				int const name_upvalue = lua_upvalueindex(1);
				for(int i = 0; i < 2; ++i)
				{
					if(get_instance(L, 1 + i))
					{
						int nargs = lua_gettop(L);

						lua_pushvalue(L, name_upvalue); // operator name
						// instance[operator name] (via get_instance_value / __index)
						lua_gettable(L, 1 + i);

						if(lua_isnil(L, -1))
						{
							lua_pop(L, 1);
							continue;
						}

						lua_insert(L, 1); // move the function to the bottom

						bool const is_unary = !!lua_toboolean(L, lua_upvalueindex(2));
						nargs = is_unary ? 1 : nargs;
						if(is_unary) // remove trailing nil
							lua_remove(L, 3);

						lua_call(L, nargs, 1);
						return 1;
					}
				}
				object_rep* inst = get_instance(L, 1);
				char const* const_s = inst->is_const() ? "const " : "";
				char const* cls_name = inst->crep()->name();
				char const* op_name = lua_tostring(L, name_upvalue);

				if (std::strcmp(op_name, "__tostring") == 0)
				{
					lua_pushfstring(L, "%s%s object: %p",
						const_s, cls_name, static_cast<void*>(inst));
					return 1;
				}
				lua_pushfstring(L, "%sclass %s: no %s operator defined.",
					const_s, cls_name, op_name);
				lua_error(L);

				return 0;
			}

		} // namespace unnamed

		LUABIND_API void push_instance_metatable(lua_State* L)
		{
			// One sequence entry for the tag, 3 non-sequence entries for
			// __gc, __index and __newindex and one more for each operator.
			lua_createtable(L, 1, 3 + number_of_operators);

			// This is used as a tag to determine if a userdata is a luabind
			// instance. We use a numeric key and a cclosure for fast comparison.
			lua_pushcfunction(L, get_instance_value);
			lua_rawseti(L, -2, 1);

			lua_pushliteral(L, "__gc");
			lua_pushcfunction(L, destroy_instance);
			lua_rawset(L, -3);

			lua_pushliteral(L, "__index");
			lua_pushcfunction(L, get_instance_value);
			lua_rawset(L, -3);

			lua_pushliteral(L, "__newindex");
			lua_pushcfunction(L, set_instance_value);
			lua_rawset(L, -3);

			for(int op = 0; op < number_of_operators; ++op)
			{
				lua_pushstring(L, get_operator_name(op));
				lua_pushvalue(L, -1);
				lua_pushboolean(L, op == op_unm || op == op_len); // Unary?
				lua_pushcclosure(L, &dispatch_operator, 2);
				lua_rawset(L, -3);
			}
		}

		LUABIND_API object_rep* get_instance(lua_State* L, int index)
		{
			object_rep* result = static_cast<object_rep*>(lua_touserdata(L, index));

			if(!result || !lua_getmetatable(L, index))
				return 0;

			lua_rawgeti(L, -1, 1);

			if(lua_tocfunction(L, -1) != &get_instance_value)
				result = 0;

			lua_pop(L, 2);

			return result;
		}

		LUABIND_API object_rep* push_new_instance(lua_State* L, class_rep* cls)
		{
			void* storage = lua_newuserdata(L, sizeof(object_rep));
			object_rep* result = new (storage) object_rep(0, cls);
			cls->get_table(L);
			lua_setuservalue(L, -2);
			lua_rawgeti(L, LUA_REGISTRYINDEX, cls->metatable_ref());
			lua_setmetatable(L, -2);
			return result;
		}

	}
}


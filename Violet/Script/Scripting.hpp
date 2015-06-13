#ifndef SCRIPTING_HPP
#define SCRIPTING_HPP

#include "Core/Component.hpp"

class Persist;

struct ScriptComponent : public Component
{
	ScriptComponent(std::string name) : name(name) {}
	const std::string name;
	void Load(const Persist&) {};
	void Unload(const Persist&) {};
	bool Has(Object obj) const { return false; };
	void Save(Object obj, Persist&) const {};
	void Remove(Object obj) {};
};

struct lua_State;

class Scripting
{
public:
	Scripting(ComponentManager& mgr);
	~Scripting();

	void PhysTick();

private:
	Scripting(const Scripting&) = delete;

	//unfortunately, ComponentManager needs pointers
	std::vector<std::unique_ptr<ScriptComponent>> components;

	int OpenLib(lua_State *L);
	int Component(lua_State* L);

	ComponentManager& mgr;

	lua_State* L;
};

#endif

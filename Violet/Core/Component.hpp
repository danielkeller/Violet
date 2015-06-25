#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include "Object.hpp"

struct Component
{
	//loads all objects from persist
	virtual void Load(const Persist&) = 0;

	//unloads all objects in persist
	virtual void Unload(const Persist&) = 0;

	//true if this object uses this component
	virtual bool Has(Object) const = 0;

	//writes this object to persist, or deletes it if Has(obj) is false
	virtual void Save(Object, Persist&) const = 0;

	//removes the object from the component, does not save
	virtual void Remove(Object) = 0;
};

class ComponentManager
{
public:
	void Register(Component* c);
	void Load(const Persist&);
	void Unload(const Persist&);
	void Delete(Object);
	void Save(Object, Persist&);
private:
	std::vector<Component*> components;
};

//for components that hold a simple map
#define MAP_COMPONENT_BOILERPLATE(Class, member)\
void Class::Unload(const Persist& persist)\
{\
	for (const auto& dat : persist.GetAll<Class>())\
		member.erase(std::get<0>(dat));\
}\
bool Class::Has(Object obj) const\
{\
	return member.count(obj) > 0;\
}\
void Class::Remove(Object obj)\
{\
	member.erase(obj);\
}

#endif
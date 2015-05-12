#include "stdafx.h"

#include "Component.hpp"

void ComponentManager::Register(Component* c)
{
	components.push_back(c);
}

void ComponentManager::Load(Persist& persist)
{
	for (auto comp : components) comp->Load(persist);
}

void ComponentManager::Unload(Persist& persist)
{
	for (auto comp : components) comp->Unload(persist);
}

void ComponentManager::Delete(Object obj)
{
	for (auto comp : components) comp->Remove(obj);
}

void ComponentManager::Save(Object obj, Persist& persist)
{
	for (auto comp : components) comp->Save(obj, persist);
}
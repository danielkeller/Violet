#include "stdafx.h"
#include "Window.hpp"
#include "Rendering/Render.hpp"
#include "Geometry/AABB.hpp"
#include "Utils/Profiling.hpp"
#include "Editor/Edit.hpp"
#include "Core/Time.hpp"
#include "Rendering/RenderPasses.hpp"
#include "File/Persist.hpp"
#include "UI/PixelDraw.hpp"

#include "Physics/NarrowPhase.hpp"
#include "Physics/RigidBody.hpp"

#include <iostream>
#include <cstdlib>
#include <ctime>

//#ifdef _WIN32
//#include "Windows.hpp"
//int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
//#else
int main(void)
//#endif
try
{
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
	Profile::CalibrateProfiling();

	ComponentManager mgr;
	//Components
	Persist persist;
	Object::Init(persist);
	ObjectName objName(persist);
	Window w;

	Position position;
	Render r(position);
	RenderPasses passes(position, w, r);
	Edit edit(r, passes, position, objName, mgr, persist);

	NarrowPhase narrowPhase(position, passes);
	RigidBody rigidBody(position);

	mgr.Register(&position);
	mgr.Register(&r);
	mgr.Register(&edit);
	mgr.Register(&narrowPhase);
	mgr.Load(persist);

	UI::Init(w);

	Object camera = objName["camera"];
	passes.Camera(camera);

	Object teapotObj = objName["teapot"];
	Object teapot2Obj = objName["teapot2"];

	if (!persist.Exists<Render>(teapotObj) || !persist.Exists<Render>(teapot2Obj))
	{
		//load the object
		VertexData capsule{ "assets/capsule.obj" };
		Material capsuleMat{ "capsule", { "assets/simple" }, { "assets/capsule.png" } };
		capsuleMat.Save(persist);

		r.Create(teapotObj, capsuleMat, capsule, Mobilty::Yes);
		r.Create(teapot2Obj, capsuleMat, capsule, Mobilty::No);
		mgr.Save(teapotObj, persist); mgr.Save(teapot2Obj, persist);
	}

	edit.Editable(teapotObj);
	edit.Editable(teapot2Obj);
	//uhhhh this is a problem
	//narrowPhase.Add(teapotObj, "assets/teapot.obj");
	//narrowPhase.Add(teapot2Obj, "assets/teapot.obj");
	rigidBody.Add(teapotObj, 1, 1);
	
	//AABBTree teapotAabb("assets/capsule.obj");
	//Object aabbObj;
	//ShowAABB aabb(teapotAabb);
	//r.Create(aabbObj, { "aabb", aabb.shaderProgram }, aabb.vertData);
	
	position[camera]->pos = {0, -3, 0};
    
    Time t;
	Events e;

    auto physTick = [&]()
	{
		auto p = Profile::Profile("physics");
		w.SetTime(t.SimTime());
		e = w.GetInput();

		UI::BeginFrame(w, e);
		edit.PhysTick(camera);
		w.SetView(UI::CurLayout().Pop().Box());

        //physics step
        //position[teapotObj]->rot *= Quaternionf{Eigen::AngleAxisf(0.04f, Vector3f::UnitY())};

		//narrowPhase.Query(teapotObj, teapot2Obj);
		rigidBody.PhysTick(t.SimTime());

        return !w.ShouldClose();
    };
    
    auto renderTick = [&](float alpha)
	{
		{
			auto p = Profile::Profile("rendering");

			w.PreDraw();
			passes.Draw(e, alpha);
			UI::EndFrame();
		}
        w.PostDraw();
	};
    
    t.MainLoop(physTick, renderTick);
    
	Profile::Print();

    return EXIT_SUCCESS;
}
catch (std::exception &ex)
{
    std::cerr << ex.what() << "\n";
#ifndef __APPLE__
    std::cerr << "Press enter to exit...\n";
    getchar();
#endif
	return EXIT_FAILURE;
}

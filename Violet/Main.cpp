#include "stdafx.h"
#include "Window.hpp"
#include "Rendering/Render.hpp"
#include "Utils/Profiling.hpp"
#include "Editor/Edit.hpp"
#include "Core/Time.hpp"
#include "Rendering/RenderPasses.hpp"
#include "File/Persist.hpp"
#include "UI/PixelDraw.hpp"
#include "Editor/Console.hpp"

#include "Physics/NarrowPhase.hpp"
#include "Physics/BroadPhase.hpp"
#include "Physics/RigidBody.hpp"

#include "Script/Scripting.hpp"

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

    auto initProf = Profile("init");
    
	ComponentManager mgr;
    
	//Components
	Persist persist;
	Object::Init(persist);
	ObjectName objName(persist);
    Window w;
    
	Position position;
	Render r(position);
	RenderPasses passes(position, w, r);
	NarrowPhase narrowPhase(position, passes);
    BroadPhase broadPhase(passes, narrowPhase);
	RigidBody rigidBody(position, narrowPhase, passes);

	Edit edit(r, passes, position, objName, narrowPhase, broadPhase, rigidBody, mgr, persist);

    Scripting script(mgr);
    Console console(script, persist);

	mgr.Register(&position);
	mgr.Register(&r);
	mgr.Register(&edit);
	mgr.Register(&narrowPhase);
	mgr.Register(&rigidBody);
	mgr.Load(persist); //load the default file

	Object camera = objName["camera"];
	passes.Camera(camera);
    position[camera]->pos = {0, 3, 0};
    
    UI::Init(w);
    
    Events e = w.GetInput();
    
    Time t;

    auto physTick = [&]()
	{
		auto p = Profile("physics");
		w.SetTime(t.SimTime());
		e = w.GetInput();

		UI::BeginFrame(w, e);
        console.Draw();
		edit.PhysTick(camera);

        //physics step
		rigidBody.PhysTick(t.SimTime());
		script.PhysTick();

        return !w.ShouldClose();
    };
    
    auto renderTick = [&](float alpha)
	{
        auto rprof = Profile("rendering");
        w.PreDraw();
        passes.Draw(e.mainView, alpha);
        UI::EndFrame();
        rprof.Stop();
        
        w.PostDraw();
	};
    
    initProf.Stop();
    
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

/*
	Object teapotObj = objName["teapot"];
	Object teapot2Obj = objName["teapot2"];
 
	if (!persist.Exists<Render>(teapotObj) || !persist.Exists<Render>(teapot2Obj))
	{
         Material capsuleMat{ "capsule", { "assets/simple" }, { "assets/capsule.png" } };
         capsuleMat.Save(persist);
         
         r.Create(teapotObj, capsuleMat, "assets/teapot.obj", Mobilty::Yes);
         r.Create(teapot2Obj, capsuleMat, "assets/triangle.obj", Mobilty::No);
         
         edit.Editable(teapotObj);
         edit.Editable(teapot2Obj);
         narrowPhase.Add(teapotObj, "assets/teapot.obj");
         narrowPhase.Add(teapot2Obj, "assets/triangle.obj");
         //rigidBody.Add(teapotObj, 1, 1);
         
         mgr.Save(teapotObj, persist); mgr.Save(teapot2Obj, persist);
	}
	
	//AABBTree teapotAabb("assets/capsule.obj");
	//Object aabbObj;
	//ShowAABB aabb(teapotAabb);
	//r.Create(aabbObj, { "aabb", aabb.shaderProgram }, aabb.vertData);
*/
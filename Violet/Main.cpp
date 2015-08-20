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

#include "Physics/Collision.hpp"
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
	Collision collision(position, passes);
	RigidBody rigidBody(position, collision, passes);

	Edit edit(r, passes, position, objName, collision, rigidBody, mgr, persist);

    Scripting script(mgr);
    Console console(script, persist);

	mgr.Register(&position);
	mgr.Register(&r);
	mgr.Register(&edit);
	mgr.Register(&collision);
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
        collision.PhysTick();
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

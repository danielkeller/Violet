#include "stdafx.h"
#include "Window.hpp"
#include "Rendering/Render.hpp"
#include "Geometry/AABB.hpp"
#include "Profiling.hpp"
#include "Mobile.hpp"
#include "Editor/Edit.hpp"
#include "Time.hpp"
#include "magic_ptr.hpp"
#include "Rendering/RenderPasses.hpp"
#include "Persist.hpp"

#include "UI/PixelDraw.hpp"
#include "UI/Layout.hpp"

#include <iostream>

int main(void)
try
{
	Profile::CalibrateProfiling();

	//Components
	Persist persist;
	Object::Init(persist);
	ObjectName objName(persist);

	Window w;
	Position position(persist);
	Mobile m(position);
	Render r(position, m, persist);
	RenderPasses passes(w, r);
	Edit edit(r, passes, position, objName);

	UI::Init(w);

	Object camera = objName["camera"];

	Object teapotObj = objName["teapot"];
	Object teapot2Obj = objName["teapot2"];

	//AABB teapotAabb(teapot.mesh);
	//Object aabbObj;
	//ShowAABB aabb(teapotAabb);

	if (!persist.Exists<Render>(teapotObj) || !persist.Exists<Render>(teapot2Obj))
	{
		//load the object
		Tex capsuleTex{ "assets/capsule.png" };
		ShaderProgram shdr{ "assets/simple" };
		VertexData capsule{ "assets/capsule.obj" };

		r.Create(teapotObj, shdr, { {}, { capsuleTex } }, capsule, Mobilty::Yes);
		r.Create(teapot2Obj, shdr, { {}, { capsuleTex } }, capsule, Mobilty::No);
		r.Save(teapotObj); r.Save(teapot2Obj);
	}

	edit.Editable(teapotObj);
	edit.Editable(teapot2Obj);

	position[camera]->pos = {0, -3, 0};
    
    Time t;
	Events e;
    
    auto physTick = [&]()
	{
		auto p = Profile::Profile("physics");
		m.Tick();
		w.SetTime(t.SimTime());
		e = w.GetInput();

		UI::BeginFrame(w, e);
		edit.PhysTick(e, camera);
		w.SetView(UI::CurLayout().Pop().Box());

        //physics step
        position[teapotObj]->rot *= Quaternionf{Eigen::AngleAxisf(0.04f, Vector3f::UnitY())};

        return !w.ShouldClose();
    };
    
    auto renderTick = [&](float alpha)
	{
		{
			auto p = Profile::Profile("rendering");
			m.Update(alpha);

			w.PreDraw();
			UI::EndFrame();
			passes.Draw(*m[camera], e);
		}
        w.PostDraw();
	};
    
    t.MainLoop(physTick, renderTick);
    
	Profile::Print();

    return EXIT_SUCCESS;
}
catch (const char* mesg)
{
    std::cerr << mesg << "\n";
#ifndef __APPLE__
    std::cerr << "Press enter to exit...\n";
    getchar();
#endif
    return EXIT_FAILURE;
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

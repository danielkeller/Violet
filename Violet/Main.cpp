#include "stdafx.h"
#include "Wavefront.hpp"
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
	Render r(position, m);
	RenderPasses passes(w, r);
	Edit edit(r, passes, w, position);

	Object camera = objName["camera"];

	Object teapotObj = objName["teapot"];
	Object teapot2Obj = objName["teapot2"];

	//load the object
	Wavefront teapot{ "assets/capsule.obj" };
	Tex capsuleTex{ "assets/capsule.png" };

	//AABB teapotAabb(teapot.mesh);
	//Object aabbObj;
	//ShowAABB aabb(teapotAabb);

    r.Create(teapotObj, teapot.shaderProgram, {{}, {capsuleTex}}, teapot.vertexData, Mobilty::Yes);
    r.Create(teapot2Obj, teapot.shaderProgram, {{}, {capsuleTex}}, teapot.vertexData, Mobilty::No);

	edit.Editable(teapotObj);
	edit.Editable(teapot2Obj);

	position[camera]->pos = {0, -3, 0};
    
    Time t;
    
    auto physTick = [&]()
	{
		auto p = Profile::Profile("physics");
		m.Tick();

        w.GetInput();
        edit.PhysTick(camera);

        //physics step
        position[teapotObj]->rot *= Quaternionf{Eigen::AngleAxisf(0.04f, Vector3f::UnitY())};

		w.ClearInput();
        return !w.ShouldClose();
    };
    
    auto renderTick = [&](float alpha)
	{
		{
			auto p = Profile::Profile("rendering");
			m.Update(alpha);

			w.PreDraw();
			r.camera = w.PerspMat() * *m[camera]; //FIXME
			passes.Draw();
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

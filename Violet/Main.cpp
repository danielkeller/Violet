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

#include "UI/UI.hpp"
#include "UI/PixelDraw.hpp"

#include "UI/Text.hpp"

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
	Edit edit(r, passes, position);

	PixelInit(w);

	Object camera = objName["camera"];

	Object teapotObj = objName["teapot"];
	Object teapot2Obj = objName["teapot2"];

	//AABB teapotAabb(teapot.mesh);
	//Object aabbObj;
	//ShowAABB aabb(teapotAabb);

	if (!persist.Exists<Render>(teapotObj))
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

	Font fnt{ "assets/DroidSansMono.ttf" };
    
    auto physTick = [&]()
	{
		auto p = Profile::Profile("physics");
		Events e = w.GetInput();

		m.Tick();
        edit.PhysTick(e, camera);

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
			r.camera = w.PerspMat() * *m[camera]; //FIXME
			passes.Draw();

			DrawUI(w);
			DrawText(fnt, "hi i am text", { 2, 14 });
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

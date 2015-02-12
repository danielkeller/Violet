#include "stdafx.h"
#include "Wavefront.hpp"
#include "Window.hpp"
#include "Rendering/Render.hpp"
#include "Geometry/Mesh.hpp"
#include "Geometry/AABB.hpp"
#include "Profiling.hpp"
#include "Mobile.hpp"
#include "Editor/Edit.hpp"
#include "Time.hpp"

#include <iostream>

int main(void)
try
{
	Profile::CalibrateProfiling();

	//Components
	Window w;
    Render r;
    Mobile m;
    Edit edit(r, w, m);

    //load the object
	Object teapotObj, teapot2Obj;
	Wavefront teapot{"assets/capsule.obj"};

	//AABB teapotAabb(teapot.mesh);
	//Object aabbObj;
	//ShowAABB aabb(teapotAabb);

    auto locProxy = r.Create(teapotObj, teapot.shaderProgram,
        {{}, {{"assets/capsule.png"}}},
        teapot.vertexData, Matrix4f::Identity()*2);
    //auto locProxyAabb = r.Create(aabbObj, {aabb.shaderProgram, {}}, {},
	//	aabb.vertData, Matrix4f::Identity());
    
    r.Create(teapot2Obj, teapot.shaderProgram,
        {{}, {{"assets/capsule.png"}}},
        teapot.vertexData, //Matrix4f::Identity());
        Eigen::Affine3f(Eigen::Translation3f{2,0,0}).matrix()*3);

    auto moveProxy = m.Create(Transform(), {locProxy}); //{locProxy, locProxyAabb});

	edit.Editable(teapotObj);
	edit.Editable(teapot2Obj);

	m.CameraLoc().pos = Vector3f(0.f, -3.f, 0.f);

	m.Tick();
    
    Time t;
    
    auto physTick = [&]()
    {
        m.Tick();

        w.GetInput();
        edit.PhysTick();

        //physics step
        moveProxy->rot *= Quaternionf{Eigen::AngleAxisf(0.04f, Vector3f::UnitY())};

        return !w.ShouldClose();
    };
    
    auto renderTick = [&](float alpha)
    {
        m.Update(alpha);
        edit.DrawTick();

        w.PreDraw();
        r.camera = w.PerspMat() * m.CameraMat();
        r.Draw();
        w.PostDraw();
        
    };
    
    t.MainLoop(physTick, renderTick);
    
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

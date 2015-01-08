#include "stdafx.h"
#include "Wavefront.hpp"
#include "Window.hpp"
#include "Rendering/Render.hpp"
#include "Geometry/Mesh.hpp"
#include "Geometry/AABB.hpp"
#include "Profiling.hpp"
#include "Mobile.hpp"
#include "Picker.hpp"
#include "Editor/Tool.hpp"
#include "Time.hpp"

#include <iostream>

#include <unistd.h>

int main(void)
try
{
	Profile::CalibrateProfiling();

	//Components
	Window w;
    Render r;
    Mobile m;
    Picker pick(r, w);
    
    //load the object
	Object teapotObj, teapot2Obj;
	Wavefront teapot{"assets/capsule.obj"};

	//AABB teapotAabb(teapot.mesh);
	//Object aabbObj;
	//ShowAABB aabb(teapotAabb);

    auto locProxy = r.Create(teapotObj, {teapot.shaderProgram, pick.shader},
        {{{{}, {{"assets/capsule.png"}}}, {}}},
        teapot.vertexData, Matrix4f::Identity()*2);
    //auto locProxyAabb = r.Create(aabbObj, {aabb.shaderProgram, {}}, {},
	//	aabb.vertData, Matrix4f::Identity());
    
    r.Create(teapot2Obj, {teapot.shaderProgram, pick.shader},
        {{{{}, {{"assets/capsule.png"}}}, {}}},
        teapot.vertexData, //Matrix4f::Identity());
        Eigen::Affine3f(Eigen::Translation3f{2,0,0}).matrix()*3);

    auto moveProxy = m.Create(Transform(), {locProxy}); //{locProxy, locProxyAabb});
    
    Tool tool(r, m);

	m.CameraLoc().pos = Vector3f(0.f, -3.f, 0.f);

	m.Tick();
    
    Time t;
    
    //todo: some objects have "clicky" behavior, some have "draggy"
    Object focused = Object::none;
    bool mouseDown = false;
    
    auto physTick = [&]() {
        w.GetInput();
        m.Tick();
        //physics step
        tool.Update(w, focused);
        moveProxy->rot *= Quaternionf{Eigen::AngleAxisf(0.04f, Vector3f::UnitY())};

        if (w.LeftMouse())
        {
            if (!mouseDown) //just clicked
                focused = pick.Picked();
            mouseDown = true;
        }
        else
        {
            mouseDown = false;
        }
        
        if (w.LeftMouse() && focused == Object::none)
        {
            m.CameraLoc().rot *= Quaternionf{
                Eigen::AngleAxisf(-w.mouseDeltaPct().x()*3.f,
                                  Vector3f::UnitZ()) }; //rotate around world z
            m.CameraLoc().rot *= Quaternionf{
                Eigen::AngleAxisf(-w.mouseDeltaPct().y()*3.f,
                                  //rotate around camera x
                                  m.CameraLoc().rot.conjugate() * Vector3f::UnitX()) };
        }

        return !w.ShouldClose();
    };
    
    auto renderTick = [&](float alpha) {
        m.Update(alpha);

        w.PreDraw();
        r.camera = w.PerspMat() * m.CameraMat();
        r.Draw();
        w.PostDraw();
        
        pick.Pick();
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

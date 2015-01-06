#include "stdafx.h"
#include "Wavefront.hpp"
#include "Window.hpp"
#include "Rendering/Render.hpp"
#include "Geometry/Mesh.hpp"
#include "Geometry/AABB.hpp"
#include "Profiling.hpp"
#include "Mobile.hpp"
#include "Picker.hpp"
#include "Time.hpp"

#include "GLFW/glfw3.h"

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

	m.CameraLoc().pos = Vector3f(0.f, -3.f, 0.f);

	m.Tick();
    
    auto physTick = [&]() {
        w.GetInput();
        m.Tick();
        //physics step
        moveProxy->rot *= Quaternionf{ Eigen::AngleAxisf(0.04f, Vector3f::UnitY()) };

        if (glfwGetMouseButton(w.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            m.CameraLoc().rot *= Quaternionf{
                Eigen::AngleAxisf(-w.mouseDeltaPct().x()*3.f,
                                  Vector3f::UnitZ()) };
            m.CameraLoc().rot *= Quaternionf{
                Eigen::AngleAxisf(w.mouseDeltaPct().y()*3.f,
                                  m.CameraLoc().rot.conjugate() * Vector3f::UnitX()) };
        }

        return !glfwWindowShouldClose(w.window);
    };
    
    auto renderTick = [&](float alpha) {
        m.Update(alpha);

        w.PreDraw();
        r.camera = w.PerspMat() * m.CameraMat();
        r.Draw();
        w.PostDraw();
        
        pick.Pick();
    };
    
    Time t;
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

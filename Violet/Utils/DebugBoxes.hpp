#ifndef DEBUG_BOXES_HPP
#define DEBUG_BOXES_HPP

#include "Rendering/Render.hpp"

//draw colored lines and boxes to debug visually
struct DebugBoxes
{
    DebugBoxes(RenderPasses&);
    
    struct Inst
    {
        Matrix4f loc;
        Vector3f color;
    };
    
    //These only have an effect if it is enabled
    void PushInst(Inst);
    void PushVector(const Vector3f& from, const Vector3f& dir, const Vector3f& color);
    void Begin();
    void End();
    
    bool enabled;
    
    Object obj;
    Material mat;
    VAO vao;
    std::vector<Inst> insts;
    BufferObject<Inst, GL_ARRAY_BUFFER, GL_STREAM_DRAW> instBuffer;
};

#endif

#ifndef FBO_HPP
#define FBO_HPP

#include "Texture.hpp"

class RenderBuffer
{
public:
    RenderBuffer(GLenum internalFormat, TexDim dim);
    RenderBuffer(const RenderBuffer&) = delete;
    RenderBuffer(RenderBuffer&&);
    ~RenderBuffer();
    GLuint Handle();

private:
    GLuint renderBuffer;
};

class FBOBindObject
{
public:
    ~FBOBindObject();
private:
    FBOBindObject(GLenum target, GLuint next);
    GLenum target;
    GLuint read_prev, draw_prev;
    static GLuint read_current, draw_current;

    template<class Pixel>
    friend class FBO;
};

template<class Pixel = RGBA8Px>
class FBO
{
public:
    FBO();
    FBO(TypedTex<Pixel> t);
    FBO(FBO&&);
    FBO(const FBO&) = delete;
    ~FBO();
    
    FBOBindObject Bind(GLenum target) const;

    void AttachTex(TypedTex<Pixel> t);
    void AttachDepth(RenderBuffer&& rb);
    void PreDraw();
    Matrix4f PerspMat() const;
    void CheckStatus() const; //throws on error
    Pixel ReadPixel(Vector2f windowPos);

private:
    GLuint fbo;
    TexDim dim;
    //keep a reference to the texture
    std::unique_ptr<TypedTex<Pixel>> tex;
    std::unique_ptr<RenderBuffer> depth;
};

#include "FBO_detail.hpp"

#endif

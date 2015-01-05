#include "GLMath.h"

template<class Pixel>
FBO<Pixel>::FBO()
{
    glGenFramebuffers(1, &fbo);
}

template<class Pixel>
FBO<Pixel>::FBO(TypedTex<Pixel> t)
    : FBO()
{
    AttachTex(t);
}

template<class Pixel>
FBO<Pixel>::FBO(FBO&& other)
    : fbo(other.fbo)
{
    other.fbo = 0;
}

template<class Pixel>
FBO<Pixel>::~FBO()
{
    glDeleteFramebuffers(1, &fbo);
}

template<class Pixel>
FBOBindObject FBO<Pixel>::Bind(GLenum target) const
{
    return {target, fbo};
}

template<class Pixel>
void FBO<Pixel>::AttachTex(TypedTex<Pixel> t)
{
    dim = t.Dim();
    auto bound = Bind(GL_FRAMEBUFFER);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.Handle(), 0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    tex.reset(new TypedTex<Pixel>(std::move(t)));
}

template<class Pixel>
void FBO<Pixel>::AttachDepth(RenderBuffer&& rb)
{
    auto bound = Bind(GL_FRAMEBUFFER);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb.Handle());
    depth.reset(new RenderBuffer(std::move(rb)));
}

#define FBO_ERROR_CASE(err) case err: throw std::runtime_error(#err);

template<class Pixel>
void FBO<Pixel>::CheckStatus() const
{
    auto bound = Bind(GL_DRAW_FRAMEBUFFER);
    switch (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER))
    {
        FBO_ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
        FBO_ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
        FBO_ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)
        FBO_ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)
        FBO_ERROR_CASE(GL_FRAMEBUFFER_UNSUPPORTED)
        FBO_ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
        FBO_ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS)
        case 0: throw std::runtime_error("Other framebuffer error");
        default: return;
    }
}

template<class Pixel>
void FBO<Pixel>::PreDraw()
{
    PreDraw({0,0,0,0});
}

template<class Pixel>
void FBO<Pixel>::PreDraw(const Eigen::Matrix<GLuint, 4, 1>& clearColor)
{
    auto bound = Bind(GL_DRAW_FRAMEBUFFER);
    glViewport(0, 0, dim.x(), dim.y());
    glClearBufferuiv(GL_COLOR, 0, clearColor.data());
    glClear(GL_DEPTH_BUFFER_BIT);
}

template<class Pixel>
Matrix4f FBO<Pixel>::PerspMat() const
{
	Matrix4f z_upToY_up;
	z_upToY_up <<
		1, 0, 0, 0,
		0, 0,-1, 0,
		0, 1, 0, 0,
		0, 0, 0, 1;
	return perspective((float)M_PI / 2.f, (float)dim.x() / dim.y(), .01f, 100.f) * z_upToY_up;
}

template<class Pixel>
Pixel FBO<Pixel>::ReadPixel(Vector2f windowPos)
{
    auto bound = Bind(GL_READ_FRAMEBUFFER);
    Pixel ret;
    glReadPixels(dim.x()*windowPos.x(), dim.y()*windowPos.y(), 1, 1,
        PixelTraits<Pixel>::format, PixelTraits<Pixel>::type, &ret);
    return ret;
}

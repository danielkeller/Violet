#include "stdafx.h"
#include "FBO.hpp"
#include "GLMath.h"

#include <iostream>

FBO::FBO()
{
    glGenFramebuffers(1, &fbo);
}

FBO::FBO(FBO&& other)
    : fbo(other.fbo)
{
    other.fbo = 0;
}

FBO::~FBO()
{
    glDeleteFramebuffers(1, &fbo);
}

GLuint FBO::BindObject::read_current = 0;
GLuint FBO::BindObject::draw_current = 0;

FBO::BindObject::BindObject(GLenum target, GLuint next)
    : target(target)
{
    glBindFramebuffer(target, next);
    if (target != GL_READ_FRAMEBUFFER)
    {
        draw_prev = draw_current;
        draw_current = next;
    }
    if (target != GL_DRAW_FRAMEBUFFER)
    {
        read_prev = read_current;
        read_current = next;
    }
}

FBO::BindObject::~BindObject()
{
    if (target != GL_READ_FRAMEBUFFER)
    {
        glBindFramebuffer(target, draw_prev);
        draw_current = draw_prev;
    }
    if (target != GL_DRAW_FRAMEBUFFER)
    {
        glBindFramebuffer(target, read_prev);
        read_current = read_prev;
    }
}

FBO::BindObject FBO::Bind(GLenum target) const
{
    return {target, fbo};
}

void FBO::AttachTex(IntTex& t)
{
    dim = t.Dim();
    auto bound = Bind(GL_FRAMEBUFFER);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.Handle(), 0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

#define ERROR_CASE(err) case err: throw std::runtime_error(#err);

void FBO::CheckStatus() const
{
    auto bound = Bind(GL_DRAW_FRAMEBUFFER);
    switch (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER))
    {
        ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
        ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
        ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)
        ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)
        ERROR_CASE(GL_FRAMEBUFFER_UNSUPPORTED)
        ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
        ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS)
        case 0: throw std::runtime_error("Other framebuffer error");
        default: return;
    }
}

void FBO::PreDraw()
{
    auto bound = Bind(GL_DRAW_FRAMEBUFFER);
    auto value = static_cast<std::uint32_t>(-1);
    glClearBufferuiv(GL_COLOR, 0, &value);
	glViewport(0, 0, dim.x(), dim.y());
}

Matrix4f FBO::PerspMat() const
{
	Matrix4f z_upToY_up;
	z_upToY_up <<
		1, 0, 0, 0,
		0, 0,-1, 0,
		0, 1, 0, 0,
		0, 0, 0, 1;
	return perspective((float)M_PI / 2.f, (float)dim.x() / dim.y(), .01f, 100.f) * z_upToY_up;
}

std::uint32_t FBO::ReadPixel(Vector2f windowPos)
{
    auto bound = Bind(GL_READ_FRAMEBUFFER);
    std::uint32_t ret;
    glReadPixels(dim.x()*windowPos.x(), dim.y()*windowPos.y(), 1, 1,
        GL_RED_INTEGER, GL_UNSIGNED_INT, &ret);
    return ret;
}

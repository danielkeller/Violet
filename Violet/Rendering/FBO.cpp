#include "stdafx.h"

#include "FBO.hpp"
#include "GLMath.h"

#include <numeric>

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

FBOBindObject FBO::Bind(GLenum target) const
{
	return{ target, fbo };
}

void FBO::AttachTexes(std::vector<Tex> ts)
{
	dim = ts[0].Dim();
	texes = std::move(ts);

	auto bound = Bind(GL_FRAMEBUFFER);

	GLuint drawbuffer = 0;
	for (const auto& tex : texes)
	{
		if (tex.Dim() != dim)
			throw std::domain_error("All textures must have the same dimension");

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + drawbuffer++,
			GL_TEXTURE_2D, tex.Handle(), 0);
	}

	std::vector<GLenum> bufs(texes.size());
	std::iota(bufs.begin(), bufs.end(), GL_COLOR_ATTACHMENT0);
	glDrawBuffers(static_cast<GLsizei>(bufs.size()), bufs.data());
}

void FBO::AttachDepth(RenderBuffer&& rb)
{
	auto bound = Bind(GL_FRAMEBUFFER);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb.Handle());
	depth = std::make_unique<RenderBuffer>(std::move(rb));
}

#define FBO_ERROR_CASE(err) case err: throw std::logic_error(#err);

void FBO::CheckStatus() const
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

void FBO::PreDraw()
{
	PreDraw(ClearColorsT{ texes.size(), { 0, 0, 0, 0 } });
}

void FBO::PreDraw(const ClearColorsT& clearColors)
{
	if (clearColors.size() != texes.size())
		throw std::domain_error("Wrong number of clear colors");

	auto bound = Bind(GL_DRAW_FRAMEBUFFER);
	glViewport(0, 0, dim.x(), dim.y());

	GLuint drawbuffer = 0;
	for (const auto& color : clearColors)
		glClearBufferuiv(GL_COLOR, drawbuffer++, color.data());

	glClear(GL_DEPTH_BUFFER_BIT);
}

Matrix4f FBO::PerspMat() const
{
	Matrix4f z_upToY_up;
	z_upToY_up <<
		1, 0, 0, 0,
		0, 0, -1, 0,
		0, 1, 0, 0,
		0, 0, 0, 1;
	return perspective((float)M_PI / 2.f, (float)dim.x() / dim.y(), .01f, 100.f) * z_upToY_up;
}

RenderBuffer::RenderBuffer(GLenum internalFormat, TexDim dim)
{
    glGenRenderbuffers(1, &renderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, dim.x(), dim.y());
}

RenderBuffer::RenderBuffer(RenderBuffer&& other)
    : renderBuffer(other.renderBuffer)
{
    other.renderBuffer = 0;
}

RenderBuffer::~RenderBuffer()
{
    glDeleteRenderbuffers(1, &renderBuffer);
}

GLuint RenderBuffer::Handle()
{
    return renderBuffer;
}

GLuint FBOBindObject::read_current = 0;
GLuint FBOBindObject::draw_current = 0;

FBOBindObject::FBOBindObject(GLenum target, GLuint next)
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

FBOBindObject::~FBOBindObject()
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

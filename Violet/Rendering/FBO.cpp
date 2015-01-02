#include "stdafx.h"

#include "FBO.hpp"

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

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

    friend class FBO;
};

class FBO
{
public:
    FBO();
    FBO(FBO&&);
    FBO(const FBO&) = delete;
    ~FBO();
    
	void AttachTexes(std::vector<Tex> ts);
	void AttachDepth(RenderBuffer&& rb);
	void AttachDepth(Tex tex);
	Tex& Texture(GLuint num) { return texes[num]; }

	template<typename... Colors>
	void PreDraw(const Colors&... colors)
	{
		PreDrawBase();
		Clear(0, colors...);
	}

	FBOBindObject Bind(GLenum target) const;
    void CheckStatus() const; //throws on error

	template<typename Pixel>
	Pixel ReadPixel(GLuint texNum, Vector2f windowPos) const
	{
		auto bound = Bind(GL_READ_FRAMEBUFFER);
		Pixel ret;
		glReadBuffer(GL_COLOR_ATTACHMENT0 + texNum);
		glReadPixels(GLint(dim.x()*windowPos.x()), GLint(dim.y()*windowPos.y()), 1, 1,
			PixelTraits<Pixel>::format, PixelTraits<Pixel>::type, &ret);
		return ret;
	}

private:
	void PreDrawBase();

	template<typename... Colors>
	void Clear(int n, const Vector4f& color, const Colors&... colors)
	{
		glClearBufferfv(GL_COLOR, n, color.data());
		Clear(n + 1, colors...);
	}

	template<typename... Colors>
	void Clear(int n, const Eigen::Matrix<GLuint, 4, 1>& color, const Colors&... colors)
	{
		glClearBufferuiv(GL_COLOR, n, color.data());
		Clear(n + 1, colors...);
	}

	void Clear(int n)
	{
		if (n != texes.size())
			throw std::domain_error("Wrong number of clear colors");
	}

    GLuint fbo;
    TexDim dim;
    //keep a reference to the textures
    std::vector<Tex> texes;
	std::unique_ptr<RenderBuffer> depth;
	std::unique_ptr<Tex> depthTex;
};

#endif

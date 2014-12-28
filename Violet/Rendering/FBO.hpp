#ifndef FBO_HPP
#define FBO_HPP

#include "Texture.hpp"

//TODO:
//template<class Pixel>
class FBO
{
public:
    FBO();
    FBO(FBO&&);
    ~FBO();
    
    class BindObject
    {
    public:
        ~BindObject();
    private:
        BindObject(GLenum target, GLuint next);
        GLenum target;
		GLuint read_prev, draw_prev;
		static GLuint read_current, draw_current;
        friend class FBO;
    };
    BindObject Bind(GLenum target) const;

    void AttachTex(IntTex& t);
    void PreDraw();
    Matrix4f PerspMat() const;
    void CheckStatus() const; //throws on error
    std::uint32_t ReadPixel(Vector2f windowPos);

private:
    GLuint fbo;
    TexDim dim;
};

#endif

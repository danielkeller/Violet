#include "MappedFile.hpp"
struct Tex::TexResource : public Resource<TexResource>
{
	TexResource(TexResource &&other)
		: ResourceTy(std::move(other))
		, textureObject(other.textureObject)
	{
		other.textureObject = 0;
	}

	//free resources associated with this program
	~TexResource();

	//object is not copyable
	TexResource(const TexResource&) = delete;
	TexResource& operator=(const TexResource&) = delete;

	//Construct from given file path
	TexResource(std::string path);

    //Construct empty texture
    TexResource(TexDim dim);

	GLuint textureObject;
    TexDim dim;
};

template<class Pixel>
TypedTex<Pixel>::TypedTex(TexDim dim)
    : Tex(std::make_shared<TexResource>(dim))
{
	//Bind(0);
	glBindTexture(GL_TEXTURE_2D, textureObject);
    //https://www.opengl.org/wiki/Common_Mistakes#Creating_a_complete_texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_NEAREST));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_NEAREST));

	glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(PixelTraits<Pixel>::internalFormat),
                 dim.x(), dim.y(), 0, PixelTraits<Pixel>::format, PixelTraits<Pixel>::type, nullptr);
}

template<class Pixel>
void TypedTex<Pixel>::Image(const Pixel* data)
{
	auto dim = resource->dim;
	glBindTexture(GL_TEXTURE_2D, textureObject);
	glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(PixelTraits<Pixel>::internalFormat),
		dim.x(), dim.y(), 0, PixelTraits<Pixel>::format, PixelTraits<Pixel>::type, data);
}
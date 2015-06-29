#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

struct Viewport
{
    //pixel coordinates
	Eigen::AlignedBox2i pixelBox;
    //GLFW screen coordinates
    Eigen::AlignedBox2i screenBox;
    
    Viewport();
    Viewport(Eigen::AlignedBox2i pixelBox, Eigen::AlignedBox2i screenBox);
    
    Vector2i PixelSize() const { return pixelBox.sizes(); }
    Vector2i ScreenSize() const { return screenBox.sizes(); }
    Vector2i Scaling() const {return PixelSize().array() / ScreenSize().array();}
    
    //convert GLFW screen coords to screen texture coords
    Vector2f Sc2Tex(Vector2f posSc) const;
    //Convert GLFW screen coords to Normalized Device Coords
    Vector2f Sc2Ndc(Vector2f posSc) const;
	Vector3f ApparentPos(Vector2f posSc, const Matrix4f& modelview) const;

	Matrix4f PerspMat() const;
	Matrix4f OrthoMat() const;
    
    Viewport SubView(Eigen::AlignedBox2i screenBox) const;

	void GlViewport() const;

	bool operator==(const Viewport& other) const
	{ return screenBox.isApprox(other.screenBox) && pixelBox.isApprox(other.pixelBox); }
	bool operator!=(const Viewport& other) const
	{ return !(*this == other); }
};

#endif
#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

struct Viewport
{
	Eigen::AlignedBox2i screenBox;
	Viewport() = default;
	Viewport(Eigen::AlignedBox2i sb);

	Vector2i size() const { return screenBox.sizes(); }

	Vector3f ApparentPos(Vector2f posPixel, const Matrix4f& modelview) const;

	Matrix4f PerspMat() const;
	Matrix4f OrthoMat() const;

	Vector2f Pixel2Scr(Vector2f posPixel) const;
	Vector2f Pixel2View(Vector2f posPixel) const;

	void GlViewport() const;

	bool operator==(const Viewport& other) const
	{ return screenBox.isApprox(other.screenBox); }
	bool operator!=(const Viewport& other) const
	{ return !(*this == other); }
};

#endif
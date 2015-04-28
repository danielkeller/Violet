#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

struct Viewport
{
	Eigen::AlignedBox2i screenBox;
	Viewport() = default;
	Viewport(Eigen::AlignedBox2i sb);

	Vector2i size() const { return screenBox.sizes(); }

	Vector3f ApparentPos(Vector2f posPixel, const Matrix4f& modelview) const
	{
		auto screenAxes = PerspMat() * modelview;
		Vector4f screenVec;
		screenVec << Pixel2Scr(posPixel)*screenAxes(3, 3), screenAxes(2, 3), screenAxes(3, 3);
		Vector4f worldVec(screenAxes.householderQr().solve(screenVec));
		return worldVec.block<3, 1>(0, 0) / worldVec[3];
	}

	Matrix4f PerspMat() const;

	Vector2f Pixel2Scr(Vector2f posPixel) const;
	Vector2f Pixel2View(Vector2f posPixel) const;

	bool operator==(const Viewport& other) const
	{ return screenBox.isApprox(other.screenBox); }
	bool operator!=(const Viewport& other) const
	{ return !(*this == other); }
};

#endif
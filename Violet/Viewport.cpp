#include "stdafx.h"
#include "Viewport.hpp"

inline Matrix4f perspective(float fovy, float aspect, float zNear, float zFar)
{
	assert(aspect > 0);
	assert(zFar > zNear);

	float tanHalfFovy = std::tan(fovy / 2.f);
	Matrix4f res = Matrix4f::Zero();
	res(0, 0) = 1.f / (aspect * tanHalfFovy);
	res(1, 2) = 1.f / (tanHalfFovy);
	res(2, 1) = -(zFar + zNear) / (zFar - zNear);
	res(3, 1) = -1.f;
	res(2, 3) = -(2.f * zFar * zNear) / (zFar - zNear);
	return res;
}

Viewport::Viewport(Eigen::AlignedBox2i sb)
	: screenBox(sb) {}

Matrix4f Viewport::PerspMat() const
{
	Vector2f dim = screenBox.sizes().cast<float>();
	return perspective((float)M_PI / 2.f, dim.x() / dim.y(), .01f, 100.f);
}

Vector2f Viewport::Pixel2View(Vector2f posPixel) const
{
	Eigen::AlignedBox2f vp = screenBox.cast<float>();
	Eigen::Array2f posView = (posPixel - vp.min()).cwiseQuotient(vp.sizes());

	posView.y() = 1 - posView.y();

	return posView;
}

Vector2f Viewport::Pixel2Scr(Vector2f posPixel) const
{
	return Pixel2View(posPixel).array() * 2.f - 1.f;
}
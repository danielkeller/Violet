#ifndef MOBILE_HPP
#define MOBILE_HPP

#include "Containers/l_map.hpp"
#include "Position.hpp"

class Mobile
{
public:
	Mobile(Position& p) : position(p) {}

	template<typename iter>
	void Update(float alpha, iter begin, iter end)
	{
		auto dat = data.begin();
		for (;begin != end; ++begin, ++dat)
		{
			auto newXfrm = position.Get(begin->obj);
			if (dat == data.end() || begin->obj != dat->first) //wrong object
			{
				auto it = data.find(begin->obj);
				if (it == data.end()) //just added
					dat = data.try_emplace(dat, begin->obj, newXfrm).first;
				else //just removed
				{
					dat = data.erase(dat);
					continue;
				}
			}
			begin->mat = interp(dat->second, newXfrm, alpha);
			dat->second = newXfrm;
		}
	}

private:
	static Matrix4f interp(const Transform& before, const Transform& loc, float alpha);

	Position& position;
	l_map<Object, Transform, std::less<Object>, Eigen::aligned_allocator<std::pair<Object, Transform>>> data;
};

#endif

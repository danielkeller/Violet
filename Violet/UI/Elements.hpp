#ifndef ELEMENTS_HPP
#define ELEMENTS_HPP

namespace UI
{
	struct Button
	{
		std::string text;
		bool hovered, active;

		bool Draw();
	};
}

#endif
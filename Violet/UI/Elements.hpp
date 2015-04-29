#ifndef ELEMENTS_HPP
#define ELEMENTS_HPP

namespace UI
{
	struct Button
	{
		Button(std::string text);
		bool Draw();

		std::string text;
		bool hovered, active;
	};
}

#endif
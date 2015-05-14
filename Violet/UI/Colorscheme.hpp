#ifndef COLORSCHEME_HPP
#define COLORSCHEME_HPP

#include <cstdint>

namespace UI
{
	using Color = std::uint32_t;

	namespace Colors
	{
		const extern Color bg;
		const extern Color divider;
		const extern Color secondary;
		const extern Color fg;

		const extern Color hilight;
		const extern Color selection;
	}
}

#endif
#include "stdafx.h"
#include "Elements.hpp"
#include "PixelDraw.hpp"
#include "Text.hpp"
#include "Window.hpp"

#include <iomanip>
#include <cstring>

using namespace UI;

//We almost never need to individually address codepoints, except for here. So the solution is to use
//UTF-32 here, and UTF-8 elsewhere. This may or may not prove to be a good idea. Also since codepoints
//are not, strictly speaking, characters, this may produce some less-than-desirable results. But it
//should be better than nothing.
#define STB_TEXTEDIT_STRING std::u32string
#define STB_TEXTEDIT_STRINGLEN(str) (static_cast<int>(str->size()))
#define STB_TEXTEDIT_GETCHAR(str, i) ((*str)[i])
#define STB_TEXTEDIT_NEWLINE U'\n'
#define STB_TEXTEDIT_DELETECHARS(str, i, n) (str->erase(i, n))
#define STB_TEXTEDIT_INSERTCHARS(str, i, cs, n) (str->insert(str->begin() + i, cs, cs+n), true)
#define STB_TEXTEDIT_K_SHIFT (1<<31)
#define STB_TEXTEDIT_K_CTRL (1<<30)
#define STB_TEXTEDIT_K_LEFT GLFW_KEY_LEFT
#define STB_TEXTEDIT_K_RIGHT GLFW_KEY_RIGHT
#define STB_TEXTEDIT_K_UP GLFW_KEY_UP
#define STB_TEXTEDIT_K_DOWN GLFW_KEY_DOWN
#define STB_TEXTEDIT_K_LINESTART GLFW_KEY_HOME
#define STB_TEXTEDIT_K_LINEEND GLFW_KEY_END
#define STB_TEXTEDIT_K_TEXTSTART (GLFW_KEY_HOME | STB_TEXTEDIT_K_CTRL)
#define STB_TEXTEDIT_K_TEXTEND (GLFW_KEY_END | STB_TEXTEDIT_K_CTRL)
#define STB_TEXTEDIT_K_DELETE GLFW_KEY_DELETE
#define STB_TEXTEDIT_K_BACKSPACE GLFW_KEY_BACKSPACE
#define STB_TEXTEDIT_K_UNDO (GLFW_KEY_Z | STB_TEXTEDIT_K_CTRL)
#define STB_TEXTEDIT_K_REDO (GLFW_KEY_Y | STB_TEXTEDIT_K_CTRL)
#define STB_TEXTEDIT_K_INSERT GLFW_KEY_INSERT
#define STB_TEXTEDIT_IS_SPACE isspace
#define STB_TEXTEDIT_K_WORDLEFT (GLFW_KEY_LEFT | STB_TEXTEDIT_K_CTRL)
#define STB_TEXTEDIT_K_WORDRIGHT (GLFW_KEY_RIGHT | STB_TEXTEDIT_K_CTRL)

#define STB_TEXTEDIT_IMPLEMENTATION
#include "stb/stb_textedit.h"

LineEdit::LineEdit(int width)
	: width(width)
{
	stb_textedit_initialize_state(&state, true);
}

bool LineEdit::Draw(std::string& text)
{
	auto l = CurLayout().PutSpace({ width, LINEH });
	auto box = l.Box();
	//as far as stb_textedit is concerned
	Vector2i origin = box.corner(AlignedBox2i::BottomLeft)
		+ Vector2i{ TEXT_PADDING, 0 };

	Vector2i mouse = FrameEvents().MousePosSc().cast<int>();
	Vector2f mouseOffs = (mouse - origin).cast<float>();
	//for a multi-line control we would flip y into stb's coordinates, but since
	//this is one line, just stick it inside the line
	mouseOffs.y() = -3;// *= -1;

    bool ret = focus.Draw(box);
    
    //Convert to utf-32 to edit
    u8_u32_convert u8_to_u32;
    std::u32string text32 = u8_to_u32.from_bytes(text);

	bool selectedAll = lastText.size() && state.select_start == 0
        && state.select_end == static_cast<int>(lastText.size());

	if (ret || text32 != lastText)
    {
		stb_textedit_clear_state(&state, true);
        state.cursor = static_cast<int>(text32.size());
    }

	if (selectedAll || focus.tabbedIn) //auto select all
	{
		state.select_start = 0;
		state.select_end = int(text32.size());
	}

	if (FrameEvents().MouseClick(GLFW_MOUSE_BUTTON_LEFT) && box.contains(mouse))
		stb_textedit_click(&text32, &state, mouseOffs.x(), mouseOffs.y());
	else if (focus.focused && FrameEvents().MouseButton(GLFW_MOUSE_BUTTON_LEFT))
		stb_textedit_drag(&text32, &state, mouseOffs.x(), mouseOffs.y());

	if (focus.focused)
	{
		//do keyboard events
		if (FrameEvents().PopKeyEvent({ { GLFW_KEY_ENTER, 0 }, GLFW_PRESS })
			|| FrameEvents().PopKeyEvent({ { GLFW_KEY_ESCAPE, 0 }, GLFW_PRESS }))
		{
			focus.Unfocus();
			ret = true;
		}

		for (auto key : FrameEvents().keyEvents)
		{
			if (key.action == GLFW_RELEASE)
				continue; //only press or repeat

			//translate from glfw to stb
			int stb_key = key.key.key; //lol
			if (key.key.mods & GLFW_MOD_CONTROL)
				stb_key |= STB_TEXTEDIT_K_CTRL;
			if (key.key.mods & GLFW_MOD_SHIFT)
				stb_key |= STB_TEXTEDIT_K_SHIFT;

			stb_textedit_key(&text32, &state, stb_key);
		}
		FrameEvents().keyEvents.clear();

		for (auto ch : FrameEvents().charEvents)
			stb_textedit_char(&text32, &state, ch);
		FrameEvents().charEvents.clear();
	}

	UI::PushZ();
    lastText = text32;
    
    //Now that editing is over, convert back to utf-8
    text = u8_to_u32.to_bytes(text32);
	DrawText(text, origin);

	//draw decorations

	Vector2i ulStart = box.corner(AlignedBox2i::BottomLeft) + Vector2i{ TEXT_PADDING, 0 };
	if (focus.focused)
	{
		DrawHlBox({ ulStart - Vector2i{ 0, 1 },
			ulStart + Vector2i{ width - 2 * TEXT_PADDING, 0 } });

		StbFindState find;

		//blink
		long timeHalfSec = std::chrono::duration_cast<
			std::chrono::duration<long, std::ratio<1, 2>>>(FrameEvents().simTime).count();

		if (timeHalfSec & 1)
		{
			stb_textedit_find_charpos(&find, &text32, state.cursor, true);
			//leave 2 pixels of daylight between the cursor and the underline
			Vector2i cursStart{ ulStart + Vector2i{ find.x - 1, 2 } };
			DrawBox({ cursStart, cursStart + Vector2i{ 0, LINEH - 2 } }, 0, UI::Colors::secondary);
        }

		if (state.select_start != state.select_end)
		{
            stb_textedit_find_charpos(&find, &text32, std::min(state.select_start, state.select_end), true);
			Vector2i selStart{ ulStart + Vector2i{ find.x - 1, 2 } };

			stb_textedit_find_charpos(&find, &text32, std::max(state.select_start, state.select_end), true);
			Vector2i selEnd{ ulStart + Vector2i{ find.x + 1, LINEH } };

            DrawBox({ selStart, selEnd }, UI::Colors::selection, 0);
		}
	}
	else
		DrawBox({ ulStart, ulStart + Vector2i{ width - 2 * TEXT_PADDING, 0 } },
		    0, UI::Colors::divider);
        
    UI::PopZ();

	return ret;
}

FloatEdit::FloatEdit(int width)
	: edit(width), prec(3), editing(false)
{}

void FloatEdit::SetDispVal(float val)
{
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(prec) << val;
	editStr = ss.str();
}

bool FloatEdit::Draw(float& val)
{
	if (!editing)
		SetDispVal(val);

	if (edit.focus.focused)
	{
		float nudge = 0;
		//intercept these keys before the edit uses them
		while (FrameEvents().PopKey({ GLFW_KEY_UP, 0 }))
			nudge += 0.1f;
		while (FrameEvents().PopKey({ GLFW_KEY_DOWN, 0 }))
			nudge -= 0.1f;
		while (FrameEvents().PopKey({ GLFW_KEY_UP, GLFW_MOD_SHIFT }))
			nudge += 0.01f;
		while (FrameEvents().PopKey({ GLFW_KEY_DOWN, GLFW_MOD_SHIFT }))
			nudge -= 0.01f;
		if (nudge != 0)
		{
			SetDispVal(val += nudge);
			editing = true;
		}
	}

	std::string dispStr = editStr;

	bool ret;
	if ((ret = edit.Draw(dispStr) && editing))
		editing = false;

	if (dispStr != editStr) //user changed it
	{
		editing = true;
		editStr = dispStr;

		std::istringstream ss(editStr);
		float typedVal;
		ss >> std::noskipws >> typedVal;
		if (!ss.fail())
			val = typedVal;
	}

	return ret;
}

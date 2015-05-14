#include "stdafx.h"
#include "Elements.hpp"
#include "PixelDraw.hpp"
#include "Text.hpp"
#include "Window.hpp"

#include <iomanip>
#include <cstring>

using namespace UI;

#define STB_TEXTEDIT_STRING std::string
#define STB_TEXTEDIT_STRINGLEN(str) (static_cast<int>(str->size()))
#define STB_TEXTEDIT_GETCHAR(str, i) ((*str)[i])
#define STB_TEXTEDIT_NEWLINE '\n'
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
		+ Vector2i{ TEXT_LEFT_PAD, BASELINE_HEIGHT };

	auto mouse = FrameEvents().MousePosPxl().cast<int>();
	Vector2f mouseOffs = (mouse - origin).cast<float>();
	mouseOffs.y() *= -1;

	if (text != lastText)
		stb_textedit_clear_state(&state, true);

	bool ret = focus.Draw(box);

	if (FrameEvents().MouseClick(GLFW_MOUSE_BUTTON_LEFT) && box.contains(mouse))
		stb_textedit_click(&text, &state, mouseOffs.x(), mouseOffs.y());
	else if (focus.focused && FrameEvents().MouseButton(GLFW_MOUSE_BUTTON_LEFT))
		stb_textedit_drag(&text, &state, mouseOffs.x(), mouseOffs.y());

	if (focus.focused)
	{
		if (FrameEvents().PopKeyEvent({ { GLFW_KEY_ENTER, 0 }, GLFW_PRESS }))
		{
			focus.Unfocus();
			ret = true;
		}

		for (auto key : FrameEvents().keyEvents)
		{
			if (key.action == GLFW_PRESS)
				continue; //wait for release or repeat

			//translate from glfw to stb
			int stb_key = key.key.key; //lol
			if (key.key.mods & GLFW_MOD_CONTROL)
				stb_key |= STB_TEXTEDIT_K_CTRL;
			if (key.key.mods & GLFW_MOD_SHIFT)
				stb_key |= STB_TEXTEDIT_K_SHIFT;

			stb_textedit_key(&text, &state, stb_key);
		}
		FrameEvents().keyEvents.clear();

		for (auto ch : FrameEvents().charEvents)
			stb_textedit_char(&text, &state, ch);
		FrameEvents().charEvents.clear();
	}

	Vector2i ulStart = box.corner(AlignedBox2i::BottomLeft) + Vector2i{ TEXT_LEFT_PAD, 0 };
	if (focus.focused)
		DrawHlBox({ ulStart, ulStart + Vector2i{ width - 2 * TEXT_LEFT_PAD, 1 } });
	else
		DrawBox({ ulStart, ulStart + Vector2i{ width - 2 * TEXT_LEFT_PAD, 1 } },
		0, UI::Colors::divider);

	lastText = text;
	DrawText(text, origin);

	StbFindState find;

	long timeHalfSec = std::chrono::duration_cast<
		std::chrono::duration<long, std::ratio<1, 2>>>(FrameEvents().simTime).count();

	UI::PushZ();
	if (focus.focused && timeHalfSec & 1) //blink
	{
		stb_textedit_find_charpos(&find, &text, state.cursor, true);
		//leave a pixel of daylight between the cursor and the underline
		Vector2i cursStart{ ulStart + Vector2i{ find.x, 2 } };
		DrawBox({ cursStart, cursStart + Vector2i{ 1, LINEH - 2 } }, 0, UI::Colors::secondary);
	}
	UI::PopZ();

	if (state.select_start != state.select_end)
	{
		int start = std::min(state.select_start, state.select_end);
		stb_textedit_find_charpos(&find, &text, start, true);
		Vector2i selStart{ ulStart + Vector2i{ find.x - 1, 2 } };

		int end = std::max(state.select_start, state.select_end);
		stb_textedit_find_charpos(&find, &text, end, true);
		Vector2i selEnd{ ulStart + Vector2i{ find.x + 1, LINEH } };

		DrawBox({ selStart, selEnd }, UI::Colors::selection, 0);
	}

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
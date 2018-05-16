#include "EngineCore.h"
#include "Font.h"
#include "Texture.h"

Font::~Font()
{
	delete tex;
}

bool Font::SplitLine(uint& out_begin, uint& out_end, int& out_width, uint& in_out_index, cstring text, uint text_end, int flags, int width) const
{
	if(in_out_index >= text_end)
		return false;

	out_begin = in_out_index;
	out_width = 0;

	// single line mode, get line width
	if(IS_SET(flags, SingleLine))
	{
		while(in_out_index < text_end)
		{
			char c = text[in_out_index];
			++in_out_index;
			out_width += GetCharWidth(c);
		}
		out_end = in_out_index;

		return true;
	}

	const uint INVALID_INDEX = (uint)-1;
	uint last_space_index = INVALID_INDEX;
	int width_when_last_space;
	for(;;)
	{
		// end of text
		if(in_out_index >= text_end)
		{
			out_end = text_end;
			break;
		}

		char c = text[in_out_index];
		if(c == '\n')
		{
			// end of line
			out_end = in_out_index;
			in_out_index++;
			break;
		}
		else if(c == '\r')
		{
			// end of line, skip \r\n
			out_end = in_out_index;
			in_out_index++;
			if(in_out_index < text_end && text[in_out_index] == '\n')
				in_out_index++;
			break;
		}
		else
		{
			int char_width = GetCharWidth(text[in_out_index]);
			if(out_width + char_width <= width || in_out_index == out_begin)
			{
				// character fits line
				if(c == ' ')
				{
					last_space_index = in_out_index;
					width_when_last_space = out_width;
				}
				out_width += char_width;
				in_out_index++;
			}
			else
			{
				// character don't fit in this line
				if(c == ' ')
				{
					// space - break line
					out_end = in_out_index;
					in_out_index++;
					break;
				}
				else if(in_out_index > out_begin && text[in_out_index - 1] == ' ')
				{
					// previous char was space - break line there
					out_end = last_space_index;
					out_width = width_when_last_space;
					break;
				}
				else
				{
					// break on last space if there was any
					if(last_space_index != INVALID_INDEX)
					{
						// Koniec bêdzie na tej spacji
						out_end = last_space_index;
						in_out_index = last_space_index + 1;
						out_width = width_when_last_space;
						break;
					}
					// Nie by³o spacji - trudno, zawinie siê jak na granicy znaku
				}

				// very long word, break anyway
				out_end = in_out_index;
				break;
			}
		}
	}

	return true;
}

Int2 Font::CalculateSize(Cstring text, int limit_width) const
{
	int len = (int)strlen(text);
	Int2 size(0, 0);
	uint unused, index = 0;
	int width;

	while(SplitLine(unused, unused, width, index, text, len, 0, limit_width))
	{
		if(width > size.x)
			size.x = width;
		size.y += height;
	}

	return size;
}

#pragma once

#include "SoyPixels.h"


class SoyPixelsMeta;
class SoyPixelsImpl;
class TStreamBuffer;


//	stb interfaces which haven't yet had any specific Soy stuff yet
namespace Png
{
	void		Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer);
	void		Read(SoyPixelsImpl& Pixels,const ArrayBridge<char>& Buffer);
	static const char*	FileExtensions[] = {".png"};
}

namespace Jpeg
{
	void		Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer);
	static const char*	FileExtensions[] = {".jpg",".jpeg"};
}

namespace Gif
{
	void		Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer);
	static const char*	FileExtensions[] = {".gif"};
}

namespace Tga
{
	void		Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer);
	static const char*	FileExtensions[] = {".tga"};
}

namespace Bmp
{
	void		Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer);
	static const char*	FileExtensions[] = {".bmp"};
}

namespace Psd
{
	void		Read(SoyPixelsImpl& Pixels,TStreamBuffer& Buffer);
	static const char*	FileExtensions[] = {".psd"};
}



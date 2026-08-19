#ifndef GAME_CLIENT_COMPONENTS_BKW_TF_SVG_ICONS_INC
#define GAME_CLIENT_COMPONENTS_BKW_TF_SVG_ICONS_INC

#include <base/color.h>
#include <base/math.h>
#include <base/vmath.h>

#include <engine/client/client.h>
#include <engine/graphics.h>
#include <engine/storage.h>

#include <game/client/ui.h>

#include <array>
#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

namespace BkwTfSvg
{
enum class EIcon
{
	HOME,
	BACK,
	USER,
	SHOP,
	GIFT,
	SETTINGS,
	TROPHY,
	CHART,
	LEVEL,
	INVENTORY,
	CLAN,
	CROWN,
	MAIL,
	GAME,
	CHECK,
	TOGGLE_OFF,
	CLOSE,
	INFO,
	WARNING,
	COINS,
	CHEVRON_RIGHT,
	REFRESH,
	COUNT,
};

struct SSegment
{
	vec2 m_From;
	vec2 m_To;
};

struct SIconGeometry
{
	std::vector<SSegment> m_vSegments;
};

inline std::array<SIconGeometry, static_cast<size_t>(EIcon::COUNT)> s_aGeometry;
inline bool s_LoadAttempted = false;
inline bool s_Loaded = false;

inline const char *IconId(EIcon Icon)
{
	switch(Icon)
	{
	case EIcon::HOME: return "home";
	case EIcon::BACK: return "back";
	case EIcon::USER: return "user";
	case EIcon::SHOP: return "shop";
	case EIcon::GIFT: return "gift";
	case EIcon::SETTINGS: return "settings";
	case EIcon::TROPHY: return "trophy";
	case EIcon::CHART: return "chart";
	case EIcon::LEVEL: return "level";
	case EIcon::INVENTORY: return "inventory";
	case EIcon::CLAN: return "clan";
	case EIcon::CROWN: return "crown";
	case EIcon::MAIL: return "mail";
	case EIcon::GAME: return "game";
	case EIcon::CHECK: return "check";
	case EIcon::TOGGLE_OFF: return "toggle-off";
	case EIcon::CLOSE: return "close";
	case EIcon::INFO: return "info";
	case EIcon::WARNING: return "warning";
	case EIcon::COINS: return "coins";
	case EIcon::CHEVRON_RIGHT: return "chevron-right";
	case EIcon::REFRESH: return "refresh";
	case EIcon::COUNT: break;
	}
	return "info";
}

// Exact path data from data/bkw/tf_menu/icons.svg. The external SVG stays the
// editable source/override, while these built-in paths guarantee that packaged
// or development clients never fall back to placeholder squares when the data
// search path is unavailable.
inline const char *BuiltInPath(EIcon Icon)
{
	switch(Icon)
	{
	case EIcon::HOME: return "M 3 11 L 12 3 L 21 11 L 21 21 L 15 21 L 15 14 L 9 14 L 9 21 L 3 21 L 3 11 Z";
	case EIcon::BACK: return "M 20 12 L 5 12 M 11 6 L 5 12 L 11 18";
	case EIcon::USER: return "M 9 4 L 12 2 L 15 4 L 16 8 L 14 11 L 10 11 L 8 8 L 9 4 Z M 5 21 L 6 16 L 10 13 L 14 13 L 18 16 L 19 21";
	case EIcon::SHOP: return "M 5 8 L 19 8 L 18 21 L 6 21 L 5 8 Z M 8 9 L 8 6 L 10 3 L 14 3 L 16 6 L 16 9";
	case EIcon::GIFT: return "M 3 9 L 21 9 L 21 21 L 3 21 L 3 9 Z M 12 9 L 12 21 M 2 6 L 22 6 L 22 10 L 2 10 L 2 6 Z M 12 6 L 9 6 L 7 4 L 8 2 L 10 2 L 12 6 Z M 12 6 L 15 6 L 17 4 L 16 2 L 14 2 L 12 6 Z";
	case EIcon::SETTINGS: return "M 12 3 L 14 4 L 15 6 L 18 6 L 20 9 L 18 11 L 18 13 L 20 15 L 18 18 L 15 18 L 14 20 L 12 21 L 10 20 L 9 18 L 6 18 L 4 15 L 6 13 L 6 11 L 4 9 L 6 6 L 9 6 L 10 4 L 12 3 Z M 9 12 L 10 9 L 14 9 L 15 12 L 14 15 L 10 15 L 9 12 Z";
	case EIcon::TROPHY: return "M 7 3 L 17 3 L 16 10 L 14 13 L 10 13 L 8 10 L 7 3 Z M 7 5 L 3 5 L 4 10 L 8 11 M 17 5 L 21 5 L 20 10 L 16 11 M 12 13 L 12 18 M 8 21 L 16 21 M 9 18 L 15 18";
	case EIcon::CHART: return "M 4 3 L 4 21 L 21 21 M 7 16 L 11 12 L 14 14 L 20 7";
	case EIcon::LEVEL: return "M 5 17 L 12 10 L 19 17 M 5 11 L 12 4 L 19 11 M 12 10 L 12 21";
	case EIcon::INVENTORY: return "M 4 7 L 12 3 L 20 7 L 20 18 L 12 22 L 4 18 L 4 7 Z M 4 7 L 12 11 L 20 7 M 12 11 L 12 22 M 8 5 L 16 9";
	case EIcon::CLAN: return "M 9 5 L 12 3 L 15 5 L 15 9 L 13 11 L 11 11 L 9 9 L 9 5 Z M 5 20 L 6 15 L 10 12 L 14 12 L 18 15 L 19 20 M 4 8 L 6 7 L 8 8 M 16 8 L 18 7 L 20 8 M 2 18 L 3 14 L 6 12 M 22 18 L 21 14 L 18 12";
	case EIcon::CROWN: return "M 3 7 L 7 12 L 12 5 L 17 12 L 21 7 L 19 19 L 5 19 L 3 7 Z M 6 22 L 18 22";
	case EIcon::MAIL: return "M 3 5 L 21 5 L 21 19 L 3 19 L 3 5 Z M 3 6 L 12 13 L 21 6";
	case EIcon::GAME: return "M 6 8 L 18 8 L 21 12 L 20 18 L 17 19 L 14 15 L 10 15 L 7 19 L 4 18 L 3 12 L 6 8 Z M 7 11 L 7 15 M 5 13 L 9 13 M 16 12 L 16 12 M 18 14 L 18 14";
	case EIcon::CHECK: return "M 5 12 L 10 17 L 20 7";
	case EIcon::TOGGLE_OFF: return "M 5 4 L 19 4 L 21 6 L 21 18 L 19 20 L 5 20 L 3 18 L 3 6 L 5 4 Z";
	case EIcon::CLOSE: return "M 5 5 L 19 19 M 19 5 L 5 19";
	case EIcon::INFO: return "M 12 2 L 17 4 L 21 9 L 21 15 L 17 20 L 12 22 L 7 20 L 3 15 L 3 9 L 7 4 L 12 2 Z M 12 10 L 12 17 M 12 7 L 12 7";
	case EIcon::WARNING: return "M 12 3 L 22 21 L 2 21 L 12 3 Z M 12 9 L 12 15 M 12 18 L 12 18";
	case EIcon::COINS: return "M 4 6 L 7 4 L 13 4 L 16 6 L 13 8 L 7 8 L 4 6 Z M 4 6 L 4 11 L 7 13 L 13 13 L 16 11 L 16 6 M 8 15 L 11 13 L 17 13 L 20 15 L 17 17 L 11 17 L 8 15 Z M 8 15 L 8 19 L 11 21 L 17 21 L 20 19 L 20 15";
	case EIcon::CHEVRON_RIGHT: return "M 9 5 L 16 12 L 9 19";
	case EIcon::REFRESH: return "M 19 8 L 19 3 L 14 3 M 19 3 L 16 6 L 13 7 L 9 7 L 6 9 L 4 12 M 5 16 L 5 21 L 10 21 M 5 21 L 8 18 L 11 17 L 15 17 L 18 15 L 20 12";
	case EIcon::COUNT: break;
	}
	return "M 5 5 L 19 19 M 19 5 L 5 19";
}

inline void SkipSeparators(const char *&pCursor)
{
	while(*pCursor == ' ' || *pCursor == '\t' || *pCursor == '\r' || *pCursor == '\n' || *pCursor == ',')
		++pCursor;
}

inline bool ParseNumber(const char *&pCursor, float &Value)
{
	SkipSeparators(pCursor);
	if(*pCursor == '\0')
		return false;
	char *pEnd = nullptr;
	Value = std::strtof(pCursor, &pEnd);
	if(pEnd == pCursor)
		return false;
	pCursor = pEnd;
	return true;
}

inline void AddSegment(std::vector<SSegment> &vSegments, vec2 From, vec2 To)
{
	if(From.x == To.x && From.y == To.y)
		return;
	vSegments.push_back({From, To});
}

inline void ParsePath(const std::string &Path, std::vector<SSegment> &vSegments)
{
	const char *pCursor = Path.c_str();
	char Command = 0;
	vec2 Current(0.0f, 0.0f);
	vec2 SubPathStart(0.0f, 0.0f);
	bool HaveCurrent = false;

	while(*pCursor)
	{
		SkipSeparators(pCursor);
		if(*pCursor == '\0')
			break;

		if(std::isalpha(static_cast<unsigned char>(*pCursor)))
		{
			Command = *pCursor++;
			if(Command == 'Z' || Command == 'z')
			{
				if(HaveCurrent)
					AddSegment(vSegments, Current, SubPathStart);
				Current = SubPathStart;
				continue;
			}
		}

		switch(Command)
		{
		case 'M':
		case 'm':
		{
			float X, Y;
			if(!ParseNumber(pCursor, X) || !ParseNumber(pCursor, Y))
				return;
			if(Command == 'm' && HaveCurrent)
			{
				X += Current.x;
				Y += Current.y;
			}
			Current = vec2(X, Y);
			SubPathStart = Current;
			HaveCurrent = true;
			Command = Command == 'M' ? 'L' : 'l';
			break;
		}
		case 'L':
		case 'l':
		{
			float X, Y;
			if(!ParseNumber(pCursor, X) || !ParseNumber(pCursor, Y))
				return;
			vec2 Next(X, Y);
			if(Command == 'l')
				Next += Current;
			if(HaveCurrent)
				AddSegment(vSegments, Current, Next);
			Current = Next;
			HaveCurrent = true;
			break;
		}
		case 'H':
		case 'h':
		{
			float X;
			if(!ParseNumber(pCursor, X))
				return;
			const vec2 Next(Command == 'h' ? Current.x + X : X, Current.y);
			if(HaveCurrent)
				AddSegment(vSegments, Current, Next);
			Current = Next;
			HaveCurrent = true;
			break;
		}
		case 'V':
		case 'v':
		{
			float Y;
			if(!ParseNumber(pCursor, Y))
				return;
			const vec2 Next(Current.x, Command == 'v' ? Current.y + Y : Y);
			if(HaveCurrent)
				AddSegment(vSegments, Current, Next);
			Current = Next;
			HaveCurrent = true;
			break;
		}
		default:
			++pCursor;
			break;
		}
	}
}

inline bool ExtractIconGeometry(const std::string &Svg, const char *pId, SIconGeometry &Geometry)
{
	const std::string IdNeedle = std::string("id=\"") + pId + "\"";
	const size_t IdPos = Svg.find(IdNeedle);
	if(IdPos == std::string::npos)
		return false;
	const size_t SymbolStart = Svg.rfind("<symbol", IdPos);
	const size_t SymbolEnd = Svg.find("</symbol>", IdPos);
	if(SymbolStart == std::string::npos || SymbolEnd == std::string::npos)
		return false;

	size_t Cursor = SymbolStart;
	while(Cursor < SymbolEnd)
	{
		const size_t PathStart = Svg.find("<path", Cursor);
		if(PathStart == std::string::npos || PathStart >= SymbolEnd)
			break;
		const size_t DStart = Svg.find("d=\"", PathStart);
		if(DStart == std::string::npos || DStart >= SymbolEnd)
			break;
		const size_t ValueStart = DStart + 3;
		const size_t ValueEnd = Svg.find('"', ValueStart);
		if(ValueEnd == std::string::npos || ValueEnd > SymbolEnd)
			break;
		ParsePath(Svg.substr(ValueStart, ValueEnd - ValueStart), Geometry.m_vSegments);
		Cursor = ValueEnd + 1;
	}
	return !Geometry.m_vSegments.empty();
}

inline bool EnsureLoaded(CUi *pUi)
{
	if(s_LoadAttempted)
		return s_Loaded;
	if(!pUi)
		return false;

	s_LoadAttempted = true;
	std::string Svg;

	if(pUi->Client())
	{
		CClient *pClient = static_cast<CClient *>(pUi->Client());
		if(IStorage *pStorage = pClient->Storage())
		{
			if(char *pData = pStorage->ReadFileStr("bkw/tf_menu/icons.svg", IStorage::TYPE_ALL))
			{
				Svg = pData;
				std::free(pData);
			}
		}
	}

	bool AnyLoaded = false;
	for(size_t i = 0; i < static_cast<size_t>(EIcon::COUNT); ++i)
	{
		const EIcon Icon = static_cast<EIcon>(i);
		SIconGeometry &Geometry = s_aGeometry[i];
		Geometry.m_vSegments.clear();

		if(!Svg.empty())
			ExtractIconGeometry(Svg, IconId(Icon), Geometry);

		// The built-in SVG path is authoritative fallback. This fixes the old
		// white-square placeholder when the external data file cannot be found.
		if(Geometry.m_vSegments.empty())
			ParsePath(BuiltInPath(Icon), Geometry.m_vSegments);

		AnyLoaded = AnyLoaded || !Geometry.m_vSegments.empty();
	}

	s_Loaded = AnyLoaded;
	return s_Loaded;
}

inline void Render(CUi *pUi, EIcon Icon, const CUIRect &Rect, ColorRGBA Color = ColorRGBA(1.0f, 1.0f, 1.0f, 0.92f))
{
	if(!pUi || !EnsureLoaded(pUi))
		return;

	const SIconGeometry &Geometry = s_aGeometry[static_cast<size_t>(Icon)];
	if(Geometry.m_vSegments.empty())
		return;

	const float Size = minimum(Rect.w, Rect.h);
	const float Scale = Size / 24.0f;
	const float OriginX = Rect.x + (Rect.w - 24.0f * Scale) * 0.5f;
	const float OriginY = Rect.y + (Rect.h - 24.0f * Scale) * 0.5f;
	std::vector<IGraphics::CLineItem> vLines;
	vLines.reserve(Geometry.m_vSegments.size() * 3);

	const float StrokeOffset = maximum(0.16f, pUi->PixelSize() * 0.30f);
	for(const SSegment &Segment : Geometry.m_vSegments)
	{
		const float X0 = OriginX + Segment.m_From.x * Scale;
		const float Y0 = OriginY + Segment.m_From.y * Scale;
		const float X1 = OriginX + Segment.m_To.x * Scale;
		const float Y1 = OriginY + Segment.m_To.y * Scale;
		vLines.emplace_back(X0, Y0, X1, Y1);
		vLines.emplace_back(X0 + StrokeOffset, Y0, X1 + StrokeOffset, Y1);
		vLines.emplace_back(X0, Y0 + StrokeOffset, X1, Y1 + StrokeOffset);
	}

	IGraphics *pGraphics = pUi->Graphics();
	pGraphics->TextureClear();
	pGraphics->LinesBegin();
	pGraphics->SetColor(Color.r, Color.g, Color.b, Color.a);
	pGraphics->LinesDraw(vLines.data(), vLines.size());
	pGraphics->LinesEnd();
}

} // namespace BkwTfSvg

#endif

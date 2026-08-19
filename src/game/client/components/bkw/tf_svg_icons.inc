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
#include <iterator>
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
	case EIcon::INFO: return "info";
	case EIcon::WARNING: return "warning";
	case EIcon::COINS: return "coins";
	case EIcon::CHEVRON_RIGHT: return "chevron-right";
	case EIcon::REFRESH: return "refresh";
	case EIcon::COUNT: break;
	}
	return "info";
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
	if(From == To)
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
	s_LoadAttempted = true;

	if(!pUi || !pUi->Client())
		return false;
	CClient *pClient = static_cast<CClient *>(pUi->Client());
	IStorage *pStorage = pClient->Storage();
	if(!pStorage)
		return false;

	char *pData = pStorage->ReadFileStr("bkw/tf_menu/icons.svg", IStorage::TYPE_ALL);
	if(!pData)
		return false;

	const std::string Svg(pData);
	std::free(pData);

	for(size_t i = 0; i < static_cast<size_t>(EIcon::COUNT); ++i)
	{
		SIconGeometry &Geometry = s_aGeometry[i];
		Geometry.m_vSegments.clear();
		ExtractIconGeometry(Svg, IconId(static_cast<EIcon>(i)), Geometry);
	}
	s_Loaded = true;
	return true;
}

inline void RenderFallback(CUi *pUi, const CUIRect &Rect, ColorRGBA Color)
{
	IGraphics *pGraphics = pUi->Graphics();
	const float Pad = minimum(Rect.w, Rect.h) * 0.22f;
	const CUIRect R{Rect.x + Pad, Rect.y + Pad, Rect.w - Pad * 2.0f, Rect.h - Pad * 2.0f};
	const IGraphics::CLineItem aLines[] = {
		{R.x, R.y, R.x + R.w, R.y},
		{R.x + R.w, R.y, R.x + R.w, R.y + R.h},
		{R.x + R.w, R.y + R.h, R.x, R.y + R.h},
		{R.x, R.y + R.h, R.x, R.y},
	};
	pGraphics->TextureClear();
	pGraphics->LinesBegin();
	pGraphics->SetColor(Color.r, Color.g, Color.b, Color.a);
	pGraphics->LinesDraw(aLines, std::size(aLines));
	pGraphics->LinesEnd();
}

inline void Render(CUi *pUi, EIcon Icon, const CUIRect &Rect, ColorRGBA Color = ColorRGBA(1.0f, 1.0f, 1.0f, 0.92f))
{
	if(!pUi || !EnsureLoaded(pUi))
	{
		if(pUi)
			RenderFallback(pUi, Rect, Color);
		return;
	}

	const SIconGeometry &Geometry = s_aGeometry[static_cast<size_t>(Icon)];
	if(Geometry.m_vSegments.empty())
	{
		RenderFallback(pUi, Rect, Color);
		return;
	}

	const float Size = minimum(Rect.w, Rect.h);
	const float Scale = Size / 24.0f;
	const float OriginX = Rect.x + (Rect.w - 24.0f * Scale) * 0.5f;
	const float OriginY = Rect.y + (Rect.h - 24.0f * Scale) * 0.5f;
	std::vector<IGraphics::CLineItem> vLines;
	vLines.reserve(Geometry.m_vSegments.size() * 3);

	const float StrokeOffset = maximum(0.18f, pUi->PixelSize() * 0.35f);
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

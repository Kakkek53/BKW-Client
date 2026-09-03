/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "ui_rect.h"

#include <engine/graphics.h>

IGraphics *CUIRect::ms_pGraphics = nullptr;
float CUIRect::ms_GlassOpacity = -1.0f;

namespace
{
// An inset ring leaves the filtered center untouched. Match DrawRectExt's
// eight segments per corner so the rim never protrudes beyond the panel.
void DrawGlassRim(IGraphics *pGraphics, const CUIRect &Rect, int Corners, float Radius, float Strength)
{
	const float Width = minimum(0.65f, Radius, minimum(Rect.w, Rect.h) * 0.25f);
	const float InnerRadius = maximum(0.0f, Radius - Width);
	vec2 aOuter[36], aInner[36];
	const int aCorners[] = {IGraphics::CORNER_TL, IGraphics::CORNER_TR, IGraphics::CORNER_BR, IGraphics::CORNER_BL};
	int Count = 0;
	for(int Corner = 0; Corner < 4; ++Corner)
	{
		const bool Right = Corner == 1 || Corner == 2;
		const bool Bottom = Corner >= 2;
		if(!(Corners & aCorners[Corner]))
		{
			aOuter[Count] = vec2(Rect.x + (Right ? Rect.w : 0.0f), Rect.y + (Bottom ? Rect.h : 0.0f));
			aInner[Count] = aOuter[Count] + vec2(Right ? -Width : Width, Bottom ? -Width : Width);
			++Count;
			continue;
		}
		const vec2 Center(Rect.x + (Right ? Rect.w - Radius : Radius), Rect.y + (Bottom ? Rect.h - Radius : Radius));
		for(int Segment = 0; Segment <= 8; ++Segment)
		{
			const float Angle = pi + (Corner + Segment / 8.0f) * pi * 0.5f;
			const vec2 Direction(std::cos(Angle), std::sin(Angle));
			aOuter[Count] = Center + Direction * Radius;
			aInner[Count++] = Center + Direction * InnerRadius;
		}
	}
	pGraphics->TextureClear();
	pGraphics->QuadsBegin();
	for(int i = 0; i < Count; ++i)
	{
		const int Next = (i + 1) % Count;
		const float Height = std::clamp(((aOuter[i].y + aOuter[Next].y) * 0.5f - Rect.y) / Rect.h, 0.0f, 1.0f);
		pGraphics->SetColor(1.0f, 1.0f, 1.0f, Strength * mix(0.22f, 0.04f, Height));
		const IGraphics::CFreeformItem Quad(aOuter[i], aOuter[Next], aInner[i], aInner[Next]);
		pGraphics->QuadsDrawFreeform(&Quad, 1);
	}
	pGraphics->QuadsEnd();
}
}

void CUIRect::HSplitMid(CUIRect *pTop, CUIRect *pBottom, float Spacing) const
{
	CUIRect r = *this;
	const float Cut = r.h / 2;
	const float HalfSpacing = Spacing / 2;

	if(pTop)
	{
		pTop->x = r.x;
		pTop->y = r.y;
		pTop->w = r.w;
		pTop->h = Cut - HalfSpacing;
	}

	if(pBottom)
	{
		pBottom->x = r.x;
		pBottom->y = r.y + Cut + HalfSpacing;
		pBottom->w = r.w;
		pBottom->h = Cut - HalfSpacing;
	}
}

void CUIRect::HSplitTop(float Cut, CUIRect *pTop, CUIRect *pBottom) const
{
	CUIRect r = *this;

	if(pTop)
	{
		pTop->x = r.x;
		pTop->y = r.y;
		pTop->w = r.w;
		pTop->h = Cut;
	}

	if(pBottom)
	{
		pBottom->x = r.x;
		pBottom->y = r.y + Cut;
		pBottom->w = r.w;
		pBottom->h = r.h - Cut;
	}
}

void CUIRect::HSplitBottom(float Cut, CUIRect *pTop, CUIRect *pBottom) const
{
	CUIRect r = *this;

	if(pTop)
	{
		pTop->x = r.x;
		pTop->y = r.y;
		pTop->w = r.w;
		pTop->h = r.h - Cut;
	}

	if(pBottom)
	{
		pBottom->x = r.x;
		pBottom->y = r.y + r.h - Cut;
		pBottom->w = r.w;
		pBottom->h = Cut;
	}
}

void CUIRect::VSplitMid(CUIRect *pLeft, CUIRect *pRight, float Spacing) const
{
	CUIRect r = *this;
	const float Cut = r.w / 2;
	const float HalfSpacing = Spacing / 2;

	if(pLeft)
	{
		pLeft->x = r.x;
		pLeft->y = r.y;
		pLeft->w = Cut - HalfSpacing;
		pLeft->h = r.h;
	}

	if(pRight)
	{
		pRight->x = r.x + Cut + HalfSpacing;
		pRight->y = r.y;
		pRight->w = Cut - HalfSpacing;
		pRight->h = r.h;
	}
}

void CUIRect::VSplitLeft(float Cut, CUIRect *pLeft, CUIRect *pRight) const
{
	CUIRect r = *this;

	if(pLeft)
	{
		pLeft->x = r.x;
		pLeft->y = r.y;
		pLeft->w = Cut;
		pLeft->h = r.h;
	}

	if(pRight)
	{
		pRight->x = r.x + Cut;
		pRight->y = r.y;
		pRight->w = r.w - Cut;
		pRight->h = r.h;
	}
}

void CUIRect::VSplitRight(float Cut, CUIRect *pLeft, CUIRect *pRight) const
{
	CUIRect r = *this;

	if(pLeft)
	{
		pLeft->x = r.x;
		pLeft->y = r.y;
		pLeft->w = r.w - Cut;
		pLeft->h = r.h;
	}

	if(pRight)
	{
		pRight->x = r.x + r.w - Cut;
		pRight->y = r.y;
		pRight->w = Cut;
		pRight->h = r.h;
	}
}

void CUIRect::Margin(vec2 Cut, CUIRect *pOtherRect) const
{
	CUIRect r = *this;

	pOtherRect->x = r.x + Cut.x;
	pOtherRect->y = r.y + Cut.y;
	pOtherRect->w = r.w - 2 * Cut.x;
	pOtherRect->h = r.h - 2 * Cut.y;
}

void CUIRect::Margin(float Cut, CUIRect *pOtherRect) const
{
	Margin(vec2(Cut, Cut), pOtherRect);
}

void CUIRect::VMargin(float Cut, CUIRect *pOtherRect) const
{
	Margin(vec2(Cut, 0.0f), pOtherRect);
}

void CUIRect::HMargin(float Cut, CUIRect *pOtherRect) const
{
	Margin(vec2(0.0f, Cut), pOtherRect);
}

bool CUIRect::Inside(vec2 Point) const
{
	return Point.x >= x && Point.x < x + w && Point.y >= y && Point.y < y + h;
}

void CUIRect::Draw(ColorRGBA Color, int Corners, float Rounding) const
{
	if(ms_GlassOpacity >= 0.0f && Color.a > 0.0f && Color.a < 1.0f && Rounding > 0.0f && w > 0.0f && h > 0.0f)
	{
		Rounding = minimum(Rounding, minimum(w, h) * 0.5f);
		ms_pGraphics->DrawMenuGlassRect(x, y, w, h, Corners, Rounding);
		const float Strength = minimum(1.0f, Color.a * 2.0f);
		// BKW-CLOUD's config card uses a faint diagonal white wash, a clear
		// center and a stronger upper rim. Keep tint separate from reflections.
		Color.a = ms_GlassOpacity * ms_GlassOpacity * Strength;
		ms_pGraphics->DrawRect(x, y, w, h, Color, Corners, Rounding);
		ms_pGraphics->DrawRect4(x, y, w, h,
			ColorRGBA(1.0f, 1.0f, 1.0f, 0.07f * Strength),
			ColorRGBA(1.0f, 1.0f, 1.0f, 0.025f * Strength),
			ColorRGBA(1.0f, 1.0f, 1.0f, 0.015f * Strength),
			ColorRGBA(1.0f, 1.0f, 1.0f, 0.03f * Strength), Corners, Rounding);
		DrawGlassRim(ms_pGraphics, *this, Corners, Rounding, Strength);
		return;
	}
	ms_pGraphics->DrawRect(x, y, w, h, Color, Corners, Rounding);
}

void CUIRect::Draw4(ColorRGBA ColorTopLeft, ColorRGBA ColorTopRight, ColorRGBA ColorBottomLeft, ColorRGBA ColorBottomRight, int Corners, float Rounding) const
{
	ms_pGraphics->DrawRect4(x, y, w, h, ColorTopLeft, ColorTopRight, ColorBottomLeft, ColorBottomRight, Corners, Rounding);
}

void CUIRect::DrawOutline(ColorRGBA Color) const
{
	const IGraphics::CLineItem aArray[] = {
		IGraphics::CLineItem(x, y, x + w, y),
		IGraphics::CLineItem(x + w, y, x + w, y + h),
		IGraphics::CLineItem(x + w, y + h, x, y + h),
		IGraphics::CLineItem(x, y + h, x, y)};
	ms_pGraphics->TextureClear();
	ms_pGraphics->LinesBegin();
	ms_pGraphics->SetColor(Color);
	ms_pGraphics->LinesDraw(aArray, std::size(aArray));
	ms_pGraphics->LinesEnd();
}

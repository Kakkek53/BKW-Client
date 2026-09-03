#ifndef ENGINE_CLIENT_MENU_GLASS_H
#define ENGINE_CLIENT_MENU_GLASS_H

#include <algorithm>

// Shared by both renderers. The last downsample uses a variable-sized region
// of its mip, so every percentage changes the filter footprint, not just a label.
struct SMenuGlassBlur
{
	static constexpr int MIP_LEVELS = 7;
	float m_Scale;
	int m_LastMip = 2;

	explicit SMenuGlassBlur(int Percent) :
		m_Scale(4.0f + 60.0f * (std::clamp(Percent, 1, 100) - 1) / 99.0f)
	{
		while(m_LastMip + 1 < MIP_LEVELS && (1 << (m_LastMip + 1)) <= m_Scale)
			++m_LastMip;
	}

	int Size(int CanvasSize, int Mip) const
	{
		return std::max(1, Mip == m_LastMip ? (int)(CanvasSize / m_Scale) : CanvasSize >> Mip);
	}
};

#endif

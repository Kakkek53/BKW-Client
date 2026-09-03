#ifndef ENGINE_CLIENT_BKW_UPDATE_VERSION_H
#define ENGINE_CLIENT_BKW_UPDATE_VERSION_H

#include <array>
#include <string_view>

namespace BkwUpdate
{
struct CVersion
{
	int m_Channel = -1; // 0 Beta, 1 Release
	std::array<int, 4> m_Numbers{};
	bool Parse(std::string_view Text)
	{
		m_Channel = -1;
		m_Numbers.fill(0);
		if(Text.size() < 2 || Text.size() >= 64)
			return false;
		const int Channel = Text[0] == 'b' || Text[0] == 'B' ? 0 : Text[0] == 'r' || Text[0] == 'R' ? 1 : -1;
		if(Channel < 0)
			return false;
		size_t Part = 0;
		bool Digit = false;
		for(size_t i = 1; i < Text.size(); ++i)
		{
			const char c = Text[i];
			if(c >= '0' && c <= '9')
			{
				if(m_Numbers[Part] > 100000)
					return false;
				m_Numbers[Part] = m_Numbers[Part] * 10 + c - '0';
				Digit = true;
			}
			else if(c == '.' && Digit && Part + 1 < m_Numbers.size())
			{
				++Part;
				Digit = false;
			}
			else
				return false;
		}
		if(!Digit)
			return false;
		m_Channel = Channel;
		return true;
	}
};

inline bool IsNewer(std::string_view Candidate, std::string_view Current, int Channel)
{
	CVersion Next, Installed;
	if(!Next.Parse(Candidate) || !Installed.Parse(Current) || Next.m_Channel != Channel)
		return false;
	// Each channel has its own numbering. An explicit channel switch permits
	// b0.3 -> r0.1; installing the target makes the next check a no-op.
	return Next.m_Channel != Installed.m_Channel || Next.m_Numbers > Installed.m_Numbers;
}
}
#endif

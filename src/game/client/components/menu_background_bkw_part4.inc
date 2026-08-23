	{
		// do little rotation
		float RotPerTick = 360.0f / (float)g_Config.m_ClRotationSpeed * std::clamp(Client()->RenderFrameTime(), 0.0f, 0.1f);
		m_CurrentDirection = rotate(m_CurrentDirection, RotPerTick);
		m_Camera.m_Center = m_RotationCenter + m_CurrentDirection * (float)g_Config.m_ClRotationRadius;
	}
	else
	{
		// positions for the animation
		vec2 DirToCenter;
		if(DistToCenter > 0.5f)
			DirToCenter = normalize(m_AnimationStartPos - m_RotationCenter);
		else
			DirToCenter = vec2(1, 0);
		vec2 TargetPos = m_RotationCenter + DirToCenter * (float)g_Config.m_ClRotationRadius;
		float Distance = distance(m_AnimationStartPos, TargetPos);
		if(Distance > 0.001f)
			m_CurrentDirection = normalize(m_AnimationStartPos - TargetPos);
		else
			m_CurrentDirection = vec2(1.0f, 0.0f);

		// move time
		m_MoveTime += std::clamp(Client()->RenderFrameTime(), 0.0f, 0.1f) * g_Config.m_ClCameraSpeed / 10.0f;
		float XVal = 1 - m_MoveTime;
		XVal = std::pow(XVal, 7.0f);

		m_Camera.m_Center = TargetPos + m_CurrentDirection * (XVal * Distance);
		if(m_CurrentPosition < 0)
		{
			m_AnimationStartPos = m_Camera.m_Center;
			m_MoveTime = 0.0f;
		}

		m_ChangedPosition = false;
	}

	CMapLayers::OnRender();

	m_CurrentPosition = -1;

	return true;
}

CCamera *CMenuBackground::GetCurCamera()
{
	return &m_Camera;
}

void CMenuBackground::ChangePosition(int PositionNumber)
{
	if(PositionNumber != m_CurrentPosition)
	{
		if(PositionNumber >= POS_START && PositionNumber < NUM_POS)
		{
			m_CurrentPosition = PositionNumber;
		}
		else
		{
			m_CurrentPosition = POS_START;
		}

		m_ChangedPosition = true;
	}
	m_AnimationStartPos = m_Camera.m_Center;
	m_RotationCenter = m_aPositions[m_CurrentPosition];
	m_MoveTime = 0.0f;
}

std::vector<CTheme> &CMenuBackground::GetThemes()
{
	if(m_vThemes.empty()) // not loaded yet
	{
		// when adding more here, make sure to change the value of PREDEFINED_THEMES_COUNT too
		m_vThemes.emplace_back("", true, true); // no theme
		LoadThemeIcon(m_vThemes.back());

		m_vThemes.emplace_back("auto", true, true); // auto theme
		LoadThemeIcon(m_vThemes.back());

		m_vThemes.emplace_back("rand", true, true); // random theme
		LoadThemeIcon(m_vThemes.back());

		m_ThemeScanStartTime = time_get_nanoseconds();
		Storage()->ListDirectory(IStorage::TYPE_ALL, "themes", ThemeScan, this);

		std::sort(m_vThemes.begin() + PREDEFINED_THEMES_COUNT, m_vThemes.end());
	}
	return m_vThemes;
}
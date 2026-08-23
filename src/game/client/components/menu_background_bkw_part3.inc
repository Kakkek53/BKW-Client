			if(m_pMap->Load(pMenuMap, Storage(), aBuf, IStorage::TYPE_ALL))
			{
				m_Loaded = true;
			}
		}

		if(!m_Loaded)
		{
			str_format(aBuf, sizeof(aBuf), "themes/%s.map", pMenuMap);
			if(m_pMap->Load(pMenuMap, Storage(), aBuf, IStorage::TYPE_ALL))
			{
				m_Loaded = true;
			}
		}

		if(!m_Loaded && ((HasDayHint && !IsDaytime) || (HasNightHint && IsDaytime)))
		{
			str_format(aBuf, sizeof(aBuf), "themes/%s_%s.map", pMenuMap, IsDaytime ? "night" : "day");
			if(m_pMap->Load(pMenuMap, Storage(), aBuf, IStorage::TYPE_ALL))
			{
				m_Loaded = true;
			}
		}

		if(m_Loaded)
		{
			m_pLayers->Init(m_pMap, true, true);

			m_pImages->LoadBackground(m_pLayers, m_pMap);
			CMapLayers::OnMapLoad();

			// look for custom positions
			CMapItemLayerTilemap *pTLayer = m_pLayers->GameLayer();
			if(pTLayer)
			{
				int DataIndex = pTLayer->m_Data;
				unsigned int Size = m_pLayers->Map()->GetDataSize(DataIndex);
				void *pTiles = m_pLayers->Map()->GetData(DataIndex);
				unsigned int TileSize = sizeof(CTile);

				if(Size >= pTLayer->m_Width * pTLayer->m_Height * TileSize)
				{
					for(int y = 0; y < pTLayer->m_Height; ++y)
					{
						for(int x = 0; x < pTLayer->m_Width; ++x)
						{
							unsigned char Index = ((CTile *)pTiles)[y * pTLayer->m_Width + x].m_Index;
							if(Index >= TILE_TIME_CHECKPOINT_FIRST && Index <= TILE_TIME_CHECKPOINT_LAST)
							{
								int ArrayIndex = std::clamp<int>((Index - TILE_TIME_CHECKPOINT_FIRST), 0, NUM_POS);
								m_aPositions[ArrayIndex] = vec2(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
							}

							x += ((CTile *)pTiles)[y * pTLayer->m_Width + x].m_Skip;
						}
					}
				}
			}
		}
		m_Loading = false;
	}
}

void CMenuBackground::OnMapLoad()
{
}

void CMenuBackground::OnRender()
{
	// Custom vote menus have an early interactive pass in CFastActions. Redraw
	// them here, after chat, finish/info messages, CMenus, tooltips and the game
	// console, so the overlay visually remains above normal ingame UI.
	const bool CustomVoteMenuEnabled =
		(s_BkwTfMenuConnectedToTeeFusion && g_Config.m_BkwTfMenu) ||
		(s_BkwTfMenuConnectedToDdnet && g_Config.m_BkwDdnetVoteMenu);
	if(s_BkwTfMenuOverlayActive && CustomVoteMenuEnabled && Client()->State() == IClient::STATE_ONLINE)
	{
		// The shared close/reset logic needs the active UI for both custom menus.
		s_pBkwTfMenuLastUi = Ui();

		if(s_BkwTfMenuConnectedToDdnet)
			RenderBkwDdnetVoteOverlay(Ui(), GameClient(), GameClient()->m_Voting, false);
		else
			RenderBkwTfMenuOverlay(Ui(), GameClient()->m_Voting, false);

		// No extra perimeter is drawn here. Each BKW menu style owns its border,
		// which removes the old bright top/bottom TeeFusion glow.
		RenderTools()->RenderCursor(Ui()->MousePos(), 24.0f);
	}
}

bool CMenuBackground::Render()
{
	m_BkwMediaBackground.SyncFromConfig();
	m_BkwMediaBackground.Update();
	if(g_Config.m_BcMenuMediaBackground && m_BkwMediaBackground.Render((float)Graphics()->ScreenWidth(), (float)Graphics()->ScreenHeight()))
		return true;

	if(!m_Loaded)
		return false;

	m_Camera.m_Zoom = 0.7f;

	float DistToCenter = distance(m_Camera.m_Center, m_RotationCenter);
	if(!m_ChangedPosition && absolute(DistToCenter - (float)g_Config.m_ClRotationRadius) <= 0.5f)
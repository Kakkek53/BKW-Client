#ifndef GAME_CLIENT_COMPONENTS_BKW_VIDEO_MAP_CONVERTER_INC
#define GAME_CLIENT_COMPONENTS_BKW_VIDEO_MAP_CONVERTER_INC

namespace BkwVideoMap
{
constexpr int ATLAS_MAX_DIMENSION = 2048;
constexpr int ATLAS_GUTTER = 2;
constexpr int MAX_ATLASES = 64;
constexpr size_t MAX_RAW_MAP_IMAGE_BYTES = 192ull * 1024ull * 1024ull;

inline char s_aStatus[256] = "Готов к конвертации MP4 → .map";

inline int SafeFrameDuration(const SMediaRawFrame &Frame)
{
	return std::clamp(Frame.m_DurationMs, 8, 10000);
}

inline void SetStatus(const char *pText)
{
	str_copy(s_aStatus, pText);
}

inline void SetStatusf(const char *pFormat, ...)
{
	va_list Args;
	va_start(Args, pFormat);
	str_format_v(s_aStatus, sizeof(s_aStatus), pFormat, Args);
	va_end(Args);
}

inline void CopyFrameToAtlas(const CImageInfo &Frame, unsigned char *pAtlas, int AtlasWidth, int AtlasHeight, int CellX, int CellY)
{
	if(!pAtlas || !Frame.m_pData || Frame.m_Format != CImageInfo::FORMAT_RGBA)
		return;

	const int FrameW = (int)Frame.m_Width;
	const int FrameH = (int)Frame.m_Height;
	const int X0 = CellX + ATLAS_GUTTER;
	const int Y0 = CellY + ATLAS_GUTTER;
	if(FrameW <= 0 || FrameH <= 0 || X0 < ATLAS_GUTTER || Y0 < ATLAS_GUTTER || X0 + FrameW + ATLAS_GUTTER > AtlasWidth || Y0 + FrameH + ATLAS_GUTTER > AtlasHeight)
		return;

	for(int y = 0; y < FrameH; ++y)
	{
		unsigned char *pDst = pAtlas + ((size_t)(Y0 + y) * AtlasWidth + X0) * 4ull;
		const unsigned char *pSrc = Frame.m_pData + (size_t)y * FrameW * 4ull;
		mem_copy(pDst, pSrc, (size_t)FrameW * 4ull);
	}

	// Extrude the edge pixels into a small gutter. This prevents neighbouring
	// atlas cells from bleeding into each other when texture filtering/mipmaps are active.
	for(int g = 1; g <= ATLAS_GUTTER; ++g)
	{
		const unsigned char *pTop = pAtlas + ((size_t)Y0 * AtlasWidth + X0) * 4ull;
		const unsigned char *pBottom = pAtlas + ((size_t)(Y0 + FrameH - 1) * AtlasWidth + X0) * 4ull;
		unsigned char *pTopDst = pAtlas + ((size_t)(Y0 - g) * AtlasWidth + X0) * 4ull;
		unsigned char *pBottomDst = pAtlas + ((size_t)(Y0 + FrameH - 1 + g) * AtlasWidth + X0) * 4ull;
		mem_copy(pTopDst, pTop, (size_t)FrameW * 4ull);
		mem_copy(pBottomDst, pBottom, (size_t)FrameW * 4ull);

		for(int y = -ATLAS_GUTTER; y < FrameH + ATLAS_GUTTER; ++y)
		{
			unsigned char *pRow = pAtlas + ((size_t)(Y0 + y) * AtlasWidth) * 4ull;
			const unsigned char *pLeft = pRow + (size_t)X0 * 4ull;
			const unsigned char *pRight = pRow + (size_t)(X0 + FrameW - 1) * 4ull;
			mem_copy(pRow + (size_t)(X0 - g) * 4ull, pLeft, 4);
			mem_copy(pRow + (size_t)(X0 + FrameW - 1 + g) * 4ull, pRight, 4);
		}
	}
}

inline CQuad MakeFrameQuad(int WorldWidth, int WorldHeight, float U0, float V0, float U1, float V1, int ColorEnvOffset)
{
	CQuad Quad{};
	const int HalfW = WorldWidth / 2;
	const int HalfH = WorldHeight / 2;
	Quad.m_aPoints[0] = {i2fx(-HalfW), i2fx(-HalfH)};
	Quad.m_aPoints[1] = {i2fx(HalfW), i2fx(-HalfH)};
	Quad.m_aPoints[2] = {i2fx(-HalfW), i2fx(HalfH)};
	Quad.m_aPoints[3] = {i2fx(HalfW), i2fx(HalfH)};
	Quad.m_aPoints[4] = {0, 0};
	Quad.m_aTexcoords[0] = {f2fx(U0), f2fx(V0)};
	Quad.m_aTexcoords[1] = {f2fx(U1), f2fx(V0)};
	Quad.m_aTexcoords[2] = {f2fx(U0), f2fx(V1)};
	Quad.m_aTexcoords[3] = {f2fx(U1), f2fx(V1)};
	for(CColor &Color : Quad.m_aColors)
		Color = {255, 255, 255, 255};
	Quad.m_PosEnv = -1;
	Quad.m_PosEnvOffset = 0;
	Quad.m_ColorEnv = 0;
	Quad.m_ColorEnvOffset = ColorEnvOffset;
	return Quad;
}

inline bool Convert(IGraphics *pGraphics, IStorage *pStorage, const char *pSourcePath, const char *pOutputName, int RequestedFps, int TargetDimension, int MaxSeconds)
{
	if(!pGraphics || !pStorage || !pSourcePath || pSourcePath[0] == '\0')
	{
		SetStatus("Ошибка: укажите путь к MP4/WebM/GIF.");
		return false;
	}
	if(!pOutputName || pOutputName[0] == '\0' || str_find(pOutputName, "/") || str_find(pOutputName, "\\") || str_find(pOutputName, ".."))
	{
		SetStatus("Ошибка: имя .map должно быть простым именем файла без папок и '..'.");
		return false;
	}

	void *pVideoData = nullptr;
	unsigned VideoDataSize = 0;
	if(!pStorage->ReadFile(pSourcePath, IStorage::TYPE_ALL_OR_ABSOLUTE, &pVideoData, &VideoDataSize) || !pVideoData || VideoDataSize == 0)
	{
		SetStatusf("Ошибка: не удалось открыть '%s'.", pSourcePath);
		free(pVideoData);
		return false;
	}

	SMediaDecodedFrames Decoded;
	SMediaDecodeLimits Limits;
	Limits.m_MaxDimension = std::clamp(TargetDimension, 128, 512);
	Limits.m_MaxFrames = 2400;
	Limits.m_MaxTotalBytes = 256ull * 1024ull * 1024ull;
	Limits.m_MaxAnimationDurationMs = 0; // Keep all timing; we crop/resample below instead of falling back to one frame.
	Limits.m_DecodeAllFrames = true;

	const bool DecodeOk = MediaDecoder::DecodeImageWithFfmpegCpu(
		pGraphics,
		static_cast<const unsigned char *>(pVideoData), VideoDataSize,
		pSourcePath, Decoded, Limits);
	free(pVideoData);

	if(!DecodeOk || Decoded.m_vFrames.empty() || Decoded.m_Width <= 0 || Decoded.m_Height <= 0)
	{
		Decoded.Free();
#if !defined(CONF_VIDEORECORDER)
		SetStatus("Ошибка: сборка без FFmpeg/VIDEORECORDER — MP4 декодер недоступен.");
#else
		SetStatus("Ошибка FFmpeg: видео не удалось декодировать.");
#endif
		return false;
	}

	int64_t SourceDurationMs = 0;
	for(const auto &Frame : Decoded.m_vFrames)
		SourceDurationMs += SafeFrameDuration(Frame);
	const int64_t RequestedDurationMs = (int64_t)std::clamp(MaxSeconds, 1, 60) * 1000ll;
	const int64_t TimelineMs = std::max<int64_t>(1, std::min(SourceDurationMs, RequestedDurationMs));

	const int FrameW = Decoded.m_Width;
	const int FrameH = Decoded.m_Height;
	const int CellW = FrameW + ATLAS_GUTTER * 2;
	const int CellH = FrameH + ATLAS_GUTTER * 2;
	const int MaxCols = std::max(1, ATLAS_MAX_DIMENSION / CellW);
	const int MaxRows = std::max(1, ATLAS_MAX_DIMENSION / CellH);
	const int FramesPerAtlas = MaxCols * MaxRows;
	if(CellW > ATLAS_MAX_DIMENSION || CellH > ATLAS_MAX_DIMENSION || FramesPerAtlas <= 0)
	{
		Decoded.Free();
		SetStatus("Ошибка: кадр слишком большой для atlas.");
		return false;
	}

	const size_t ApproxBytesPerFrame = (size_t)CellW * CellH * 4ull;
	const int MaxFramesByMemory = std::max<int>(1, (int)(MAX_RAW_MAP_IMAGE_BYTES / std::max<size_t>(1, ApproxBytesPerFrame)));
	const int MaxFramesByImages = FramesPerAtlas * MAX_ATLASES;
	const int MaxOutputFrames = std::max(1, std::min(MaxFramesByMemory, MaxFramesByImages));

	int EffectiveFps = std::clamp(RequestedFps, 5, 30);
	const int64_t InitiallyRequestedFrames = (TimelineMs * EffectiveFps + 999) / 1000;
	if(InitiallyRequestedFrames > MaxOutputFrames)
		EffectiveFps = std::max(1, (int)((int64_t)MaxOutputFrames * 1000ll / TimelineMs));

	const int FrameDurationMs = std::max(1, round_to_int(1000.0f / (float)EffectiveFps));
	int OutputFrameCount = (int)((TimelineMs + FrameDurationMs - 1) / FrameDurationMs);
	OutputFrameCount = std::clamp(OutputFrameCount, 1, MaxOutputFrames);
	const int TotalLoopMs = FrameDurationMs * OutputFrameCount;

	std::vector<int> vSourceFrameIndices;
	vSourceFrameIndices.reserve(OutputFrameCount);
	int SourceIndex = 0;
	int64_t SourceFrameEndMs = SafeFrameDuration(Decoded.m_vFrames[0]);
	for(int OutputIndex = 0; OutputIndex < OutputFrameCount; ++OutputIndex)
	{
		const int64_t TargetTimeMs = std::min<int64_t>((int64_t)OutputIndex * FrameDurationMs, TimelineMs - 1);
		while(SourceIndex + 1 < (int)Decoded.m_vFrames.size() && TargetTimeMs >= SourceFrameEndMs)
		{
			++SourceIndex;
			SourceFrameEndMs += SafeFrameDuration(Decoded.m_vFrames[SourceIndex]);
		}
		vSourceFrameIndices.push_back(SourceIndex);
	}

	char aOutputPath[IO_MAX_PATH_LENGTH];
	char aBaseName[IO_MAX_PATH_LENGTH];
	fs_split_file_extension(pOutputName, aBaseName, sizeof(aBaseName));
	if(aBaseName[0] == '\0')
		str_copy(aBaseName, "bkw_video");
	str_format(aOutputPath, sizeof(aOutputPath), "maps/%s.map", aBaseName);

	CDataFileWriter Writer;
	if(!Writer.Open(pStorage, aOutputPath, IStorage::TYPE_SAVE))
	{
		Decoded.Free();
		SetStatusf("Ошибка: не удалось создать '%s'.", aOutputPath);
		return false;
	}

	CMapItemVersion Version{};
	Version.m_Version = 1;
	Writer.AddItem(MAPITEMTYPE_VERSION, 0, sizeof(Version), &Version);

	CMapItemInfoSettings Info{};
	Info.m_Version = 1;
	Info.m_Author = Writer.AddDataString("BKW Client MP4 Converter");
	Info.m_MapVersion = Writer.AddDataString("BKW Video Map v1");
	Info.m_Credits = Writer.AddDataString(pSourcePath);
	Info.m_License = -1;
	Info.m_Settings = -1;
	Writer.AddItem(MAPITEMTYPE_INFO, 0, sizeof(Info), &Info);

	const int AtlasCount = (OutputFrameCount + FramesPerAtlas - 1) / FramesPerAtlas;
	std::vector<std::vector<CQuad>> vvAtlasQuads;
	vvAtlasQuads.resize(AtlasCount);

	const float SourceAspect = (float)FrameW / (float)FrameH;
	int WorldWidth = 1600;
	int WorldHeight = 1200;
	if(SourceAspect >= 1600.0f / 1200.0f)
		WorldWidth = std::max(1600, round_to_int(1200.0f * SourceAspect));
	else
		WorldHeight = std::max(1200, round_to_int(1600.0f / SourceAspect));

	for(int AtlasIndex = 0; AtlasIndex < AtlasCount; ++AtlasIndex)
	{
		const int FirstFrame = AtlasIndex * FramesPerAtlas;
		const int FramesThisAtlas = std::min(FramesPerAtlas, OutputFrameCount - FirstFrame);
		const int Cols = std::min(MaxCols, FramesThisAtlas);
		const int Rows = (FramesThisAtlas + Cols - 1) / Cols;
		const int AtlasW = Cols * CellW;
		const int AtlasH = Rows * CellH;
		const size_t AtlasBytes = (size_t)AtlasW * AtlasH * 4ull;
		std::vector<unsigned char> vAtlas(AtlasBytes, 0);
		std::vector<CQuad> &vQuads = vvAtlasQuads[AtlasIndex];
		vQuads.reserve(FramesThisAtlas);

		for(int LocalFrame = 0; LocalFrame < FramesThisAtlas; ++LocalFrame)
		{
			const int GlobalFrame = FirstFrame + LocalFrame;
			const int Col = LocalFrame % Cols;
			const int Row = LocalFrame / Cols;
			const int CellX = Col * CellW;
			const int CellY = Row * CellH;
			const CImageInfo &Frame = Decoded.m_vFrames[vSourceFrameIndices[GlobalFrame]].m_Image;
			CopyFrameToAtlas(Frame, vAtlas.data(), AtlasW, AtlasH, CellX, CellY);

			const float U0 = (float)(CellX + ATLAS_GUTTER) / (float)AtlasW;
			const float V0 = (float)(CellY + ATLAS_GUTTER) / (float)AtlasH;
			const float U1 = (float)(CellX + ATLAS_GUTTER + FrameW) / (float)AtlasW;
			const float V1 = (float)(CellY + ATLAS_GUTTER + FrameH) / (float)AtlasH;
			const int FrameStartMs = GlobalFrame * FrameDurationMs;
			const int EnvOffset = TotalLoopMs > 0 ? (TotalLoopMs - (FrameStartMs % TotalLoopMs)) % TotalLoopMs : 0;
			vQuads.push_back(MakeFrameQuad(WorldWidth, WorldHeight, U0, V0, U1, V1, EnvOffset));
		}

		char aImageName[64];
		str_format(aImageName, sizeof(aImageName), "bkw_video_atlas_%02d", AtlasIndex + 1);
		CMapItemImage Image{};
		Image.m_Version = 1;
		Image.m_Width = AtlasW;
		Image.m_Height = AtlasH;
		Image.m_External = 0;
		Image.m_ImageName = Writer.AddDataString(aImageName);
		Image.m_ImageData = Writer.AddData(AtlasBytes, vAtlas.data(), CDataFileWriter::COMPRESSION_BEST);
		Writer.AddItem(MAPITEMTYPE_IMAGE, AtlasIndex, sizeof(Image), &Image);
	}

	Decoded.Free();

	for(int AtlasIndex = 0; AtlasIndex < AtlasCount; ++AtlasIndex)
	{
		const std::vector<CQuad> &vQuads = vvAtlasQuads[AtlasIndex];
		CMapItemLayerQuads Layer{};
		Layer.m_Version = 2;
		Layer.m_Layer.m_Version = 0;
		Layer.m_Layer.m_Flags = 0;
		Layer.m_Layer.m_Type = LAYERTYPE_QUADS;
		Layer.m_Image = AtlasIndex;
		Layer.m_NumQuads = (int)vQuads.size();
		Layer.m_Data = Writer.AddDataSwapped(vQuads.size() * sizeof(CQuad), vQuads.data());
		char aLayerName[64];
		str_format(aLayerName, sizeof(aLayerName), "BKW Video %02d", AtlasIndex + 1);
		StrToInts(Layer.m_aName, std::size(Layer.m_aName), aLayerName);
		Writer.AddItem(MAPITEMTYPE_LAYER, AtlasIndex, sizeof(Layer), &Layer);
	}

	std::array<CTile, 4> aGameTiles{};
	CMapItemLayerTilemap GameLayer{};
	GameLayer.m_Layer.m_Version = 0;
	GameLayer.m_Layer.m_Flags = 0;
	GameLayer.m_Layer.m_Type = LAYERTYPE_TILES;
	GameLayer.m_Version = 3;
	GameLayer.m_Width = 2;
	GameLayer.m_Height = 2;
	GameLayer.m_Flags = TILESLAYERFLAG_GAME;
	GameLayer.m_Color = {255, 255, 255, 255};
	GameLayer.m_ColorEnv = -1;
	GameLayer.m_ColorEnvOffset = 0;
	GameLayer.m_Image = -1;
	GameLayer.m_Data = Writer.AddData(sizeof(aGameTiles), aGameTiles.data());
	GameLayer.m_Tele = -1;
	GameLayer.m_Speedup = -1;
	GameLayer.m_Front = -1;
	GameLayer.m_Switch = -1;
	GameLayer.m_Tune = -1;
	StrToInts(GameLayer.m_aName, std::size(GameLayer.m_aName), "Game");
	Writer.AddItem(MAPITEMTYPE_LAYER, AtlasCount, sizeof(GameLayer), &GameLayer);

	CMapItemGroup VideoGroup{};
	VideoGroup.m_Version = 3;
	VideoGroup.m_OffsetX = 0;
	VideoGroup.m_OffsetY = 0;
	VideoGroup.m_ParallaxX = 0;
	VideoGroup.m_ParallaxY = 0;
	VideoGroup.m_UseClipping = 0;
	VideoGroup.m_ClipX = VideoGroup.m_ClipY = VideoGroup.m_ClipW = VideoGroup.m_ClipH = 0;
	VideoGroup.m_StartLayer = 0;
	VideoGroup.m_NumLayers = AtlasCount;
	StrToInts(VideoGroup.m_aName, std::size(VideoGroup.m_aName), "BKW Video");
	Writer.AddItem(MAPITEMTYPE_GROUP, 0, sizeof(VideoGroup), &VideoGroup);

	CMapItemGroup GameGroup{};
	GameGroup.m_Version = 3;
	GameGroup.m_OffsetX = 0;
	GameGroup.m_OffsetY = 0;
	GameGroup.m_ParallaxX = 100;
	GameGroup.m_ParallaxY = 100;
	GameGroup.m_UseClipping = 0;
	GameGroup.m_StartLayer = AtlasCount;
	GameGroup.m_NumLayers = 1;
	StrToInts(GameGroup.m_aName, std::size(GameGroup.m_aName), "Game");
	Writer.AddItem(MAPITEMTYPE_GROUP, 1, sizeof(GameGroup), &GameGroup);

	CMapItemEnvelope Envelope{};
	Envelope.m_Version = 2;
	Envelope.m_Channels = 4;
	Envelope.m_StartPoint = 0;
	Envelope.m_NumPoints = 3;
	Envelope.m_Synchronized = 0;
	StrToInts(Envelope.m_aName, std::size(Envelope.m_aName), "BKW Video Step");
	Writer.AddItem(MAPITEMTYPE_ENVELOPE, 0, sizeof(Envelope), &Envelope);

	CEnvPoint aPoints[3]{};
	for(CEnvPoint &Point : aPoints)
	{
		Point.m_Curvetype = CURVETYPE_STEP;
		Point.m_aValues[0] = f2fx(1.0f);
		Point.m_aValues[1] = f2fx(1.0f);
		Point.m_aValues[2] = f2fx(1.0f);
	}
	aPoints[0].m_Time = CFixedTime::FromSeconds(0.0f);
	aPoints[0].m_aValues[3] = f2fx(1.0f);
	aPoints[1].m_Time = CFixedTime::FromSeconds(FrameDurationMs / 1000.0f);
	aPoints[1].m_aValues[3] = 0;
	aPoints[2].m_Time = CFixedTime::FromSeconds(TotalLoopMs / 1000.0f);
	aPoints[2].m_aValues[3] = f2fx(1.0f);
	Writer.AddItem(MAPITEMTYPE_ENVPOINTS, 0, sizeof(aPoints), aPoints);

	Writer.Finish();

	if(EffectiveFps != std::clamp(RequestedFps, 5, 30))
		SetStatusf("Готово: %s • %d кадров • %d atlas • %d FPS (авто-снижено из %d) • %dx%d", aOutputPath, OutputFrameCount, AtlasCount, EffectiveFps, RequestedFps, FrameW, FrameH);
	else
		SetStatusf("Готово: %s • %d кадров • %d atlas • %d FPS • %dx%d", aOutputPath, OutputFrameCount, AtlasCount, EffectiveFps, FrameW, FrameH);
	return true;
}
} // namespace BkwVideoMap

#endif

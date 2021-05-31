#include "RTModelViewer.h"


namespace Graphics
{
	extern EnumVar DebugZoom;
}

void RTModelViewer::Update(float deltaT)
{
	ScopedTimer _prof(L"Update State");

	if (GameInput::IsFirstPressed(GameInput::kLShoulder))
		DebugZoom.Decrement();
	else if (GameInput::IsFirstPressed(GameInput::kRShoulder))
		DebugZoom.Increment();
	else if (GameInput::IsFirstPressed(GameInput::kKey_1))
		rayTracingMode = RTM_OFF;
	else if (GameInput::IsFirstPressed(GameInput::kKey_2))
		rayTracingMode = RTM_OFF_WITH_CAUSTICS;
	else if (GameInput::IsFirstPressed(GameInput::kKey_3))
		rayTracingMode = RTM_BACKWARD_WITH_CAUSTICS;
	else if (GameInput::IsFirstPressed(GameInput::kKey_4))
		rayTracingMode = RTM_CAUSTIC;
	else if (GameInput::IsFirstPressed(GameInput::kKey_5))
		rayTracingMode = RTM_BACKWARD;
	else if (GameInput::IsFirstPressed(GameInput::kKey_6))
		rayTracingMode = RTM_DIFFUSE_WITH_SHADOWMAPS;
	else if (GameInput::IsFirstPressed(GameInput::kKey_7))
		rayTracingMode = RTM_REFLECTIONS;
	else if (GameInput::IsFirstPressed(GameInput::kKey_8))
		rayTracingMode = RTM_SSR;
	else if (GameInput::IsFirstPressed(GameInput::kKey_9))
		rayTracingMode = RTM_TRAVERSAL;
	else if (GameInput::IsFirstPressed(GameInput::kKey_0))
		rayTracingMode = RTM_DIFFUSE_WITH_SHADOWRAYS;


	static bool freezeCamera = false;

	if (GameInput::IsFirstPressed(GameInput::kKey_f))
	{
		freezeCamera = !freezeCamera;
	}

	if (GameInput::IsFirstPressed(GameInput::kKey_z))
	{
		m_CameraPosArrayCurrentPosition = (m_CameraPosArrayCurrentPosition + c_NumCameraPositions - 1) % c_NumCameraPositions;
		SetCameraToPredefinedPosition(m_CameraPosArrayCurrentPosition);
	}
	else if (GameInput::IsFirstPressed(GameInput::kKey_x))
	{
		m_CameraPosArrayCurrentPosition = (m_CameraPosArrayCurrentPosition + 1) % c_NumCameraPositions;
		SetCameraToPredefinedPosition(m_CameraPosArrayCurrentPosition);
	}

	if (!freezeCamera)
	{
		m_CameraController->Update(deltaT);
	}

	GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Update");

	m_ModelInst.Update(gfxContext, deltaT);

	gfxContext.Finish();

	// We use viewport offsets to jitter sample positions from frame to frame (for TAA.)
	// D3D has a design quirk with fractional offsets such that the implicit scissor
	// region of a viewport is floor(TopLeftXY) and floor(TopLeftXY + WidthHeight), so
	// having a negative fractional top left, e.g. (-0.25, -0.25) would also shift the
	// BottomRight corner up by a whole integer.  One solution is to pad your viewport
	// dimensions with an extra pixel.  My solution is to only use positive fractional offsets,
	// but that means that the average sample position is +0.5, which I use when I disable
	// temporal AA.
	TemporalEffects::GetJitterOffset(m_MainViewport.TopLeftX, m_MainViewport.TopLeftY);

	m_MainViewport.Width = (float)g_SceneColorBuffer.GetWidth();
	m_MainViewport.Height = (float)g_SceneColorBuffer.GetHeight();
	m_MainViewport.MinDepth = 0.0f;
	m_MainViewport.MaxDepth = 1.0f;

	m_MainScissor.left = 0;
	m_MainScissor.top = 0;
	m_MainScissor.right = (LONG)g_SceneColorBuffer.GetWidth();
	m_MainScissor.bottom = (LONG)g_SceneColorBuffer.GetHeight();

	UpdateLight();

}

void RTModelViewer::SetCameraToPredefinedPosition(int cameraPosition)
{
	if (cameraPosition < 0 || cameraPosition >= c_NumCameraPositions)
		return;

	((FlyingFPSCamera*)m_CameraController.get())->SetHeadingPitchAndPosition(
		m_CameraPosArray[m_CameraPosArrayCurrentPosition].heading,
		m_CameraPosArray[m_CameraPosArrayCurrentPosition].pitch,
		m_CameraPosArray[m_CameraPosArrayCurrentPosition].position);
}

NumVar g_LightPosX(			"App/LightSource/Pos/PosX",					315,		-10000.0f,	10000.0f,	25.0f);
NumVar g_LightPosY(			"App/LightSource/Pos/PosY",					315.0f,		-10000.0f,	10000.0f,	25.0f);
NumVar g_LightPosZ(			"App/LightSource/Pos/PosZ",					-54.0f,		-10000.0f,	10000.0f,	25.0f);
NumVar g_ConeDirX(			"App/LightSource/Pos/ConeDirX",				-0.86f,		-1.0f,		1.0f,		0.05f);
NumVar g_ConeDirY(			"App/LightSource/Pos/ConeDirY",				-0.31f,		-1.0f,		1.0f,		0.05f);
NumVar g_ConeDirZ(			"App/LightSource/Pos/ConeDirZ",				0.4f,		-1.0f,		1.0f,		0.05f);

NumVar g_LightColorR(		"App/LightSource/Power/ColorR",				1.0f,		0.0f,		1.0f,		0.05f);
NumVar g_LightColorG(		"App/LightSource/Power/ColorG",				0.5f,		0.0f,		1.0f,		0.05f);
NumVar g_LightColorB(		"App/LightSource/Power/ColorB",				1.0f,		0.0f,		1.0f,		0.05f);
NumVar g_LightIntensity(	"App/LightSource/Power/LightIntensity",		0.05f,		0.0f,		300.0f,		0.005f);

NumVar g_ConeInner(			"App/LightSource/Size/ConeInner",			0.2f,		0.0f,		3.1415/2,	0.02f);
NumVar g_ConeOuter(			"App/LightSource/Size/ConeOuter",			0.3f,		0.0f,		3.1415/2,	0.02f);

ExpVar g_LightRadius(		"App/LightSource/Size/LightRadius",			1500.0f,		0.0f,		10000.0f,	0.05f);

LightSource g_LightSource;

void RTModelViewer::UpdateLight()
{
	if (GameInput::IsFirstPressed(GameInput::kKey_l))
	{
		g_LightSource.Pos		= m_Camera.GetPosition();
		g_LightSource.ConeDir	= m_Camera.GetForwardVec();

		g_LightPosX = g_LightSource.Pos.GetX();
		g_LightPosY = g_LightSource.Pos.GetY();
		g_LightPosZ = g_LightSource.Pos.GetZ();

		g_ConeDirX = g_LightSource.ConeDir.GetX();
		g_ConeDirY = g_LightSource.ConeDir.GetY();
		g_ConeDirZ = g_LightSource.ConeDir.GetZ();
	}
	else
	{
		g_LightSource.Pos.SetX(static_cast<float>(g_LightPosX));
		g_LightSource.Pos.SetY(static_cast<float>(g_LightPosY));
		g_LightSource.Pos.SetZ(static_cast<float>(g_LightPosZ));

		g_LightSource.ConeDir.SetX(static_cast<float>(g_ConeDirX));
		g_LightSource.ConeDir.SetY(static_cast<float>(g_ConeDirY));
		g_LightSource.ConeDir.SetZ(static_cast<float>(g_ConeDirZ));

		Math::Normalize(g_LightSource.ConeDir);
		g_ConeDirX = g_LightSource.ConeDir.GetX();
		g_ConeDirY = g_LightSource.ConeDir.GetY();
		g_ConeDirZ = g_LightSource.ConeDir.GetZ();
	}

	g_LightSource.Color.SetX(static_cast<float>(g_LightColorR));
	g_LightSource.Color.SetY(static_cast<float>(g_LightColorG));
	g_LightSource.Color.SetZ(static_cast<float>(g_LightColorB));

	g_LightSource.ConeInner = g_ConeInner;
	g_LightSource.ConeOuter = g_ConeOuter;

	g_LightSource.LightIntensity = g_LightIntensity;
	g_LightSource.LightRadius = g_LightRadius;


	Lighting::UpdateLightData(0, g_LightSource.Pos, g_LightSource.LightRadius, g_LightSource.Color * g_LightSource.LightIntensity, g_LightSource.ConeDir, g_LightSource.ConeInner, g_LightSource.ConeOuter);
	Lighting::UpdateLightBuffer();
}

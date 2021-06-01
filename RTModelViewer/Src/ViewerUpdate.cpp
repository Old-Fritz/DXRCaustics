#include "RTModelViewer.h"
#include <fstream>

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

void UpdateNumVars(EngineVar::ActionType);

NumVar g_SelectedLightSource("App/LightSource/Selected",				0, 0, MAX_LIGHTS - 1, 1, UpdateNumVars);
NumVar g_LightsCount(		"App/LightSource/LigtsCount",				3, 0, MAX_LIGHTS - 1, 1);

LightSource g_LightSource[MAX_LIGHTS];




void UpdateNumVars(EngineVar::ActionType)
{
	uint32_t ind = g_SelectedLightSource;

	g_LightPosX = g_LightSource[ind].Pos.GetX();
	g_LightPosY = g_LightSource[ind].Pos.GetY();
	g_LightPosZ = g_LightSource[ind].Pos.GetZ();

	g_ConeDirX = g_LightSource[ind].ConeDir.GetX();
	g_ConeDirY = g_LightSource[ind].ConeDir.GetY();
	g_ConeDirZ = g_LightSource[ind].ConeDir.GetZ();

	g_LightColorR = g_LightSource[ind].Color.GetX();
	g_LightColorG = g_LightSource[ind].Color.GetY();
	g_LightColorB = g_LightSource[ind].Color.GetZ();


	g_ConeInner = g_LightSource[ind].ConeInner;
	g_ConeOuter = g_LightSource[ind].ConeOuter;

	g_LightIntensity = g_LightSource[ind].LightIntensity;
	g_LightRadius = g_LightSource[ind].LightRadius;
}

void RTModelViewer::UpdateLight()
{
	uint32_t ind = g_SelectedLightSource;
	//UpdateNumVars(ind);

	if (GameInput::IsFirstPressed(GameInput::kKey_k))
	{
		SaveLightsInFile();
	}
	if (GameInput::IsFirstPressed(GameInput::kKey_j))
	{
		LoadLightsFromFile();
	}

	
	if (GameInput::IsFirstPressed(GameInput::kKey_l))
	{

		g_LightSource[ind].Pos = m_Camera.GetPosition();
		g_LightSource[ind].ConeDir = m_Camera.GetForwardVec();

		g_LightPosX = g_LightSource[ind].Pos.GetX();
		g_LightPosY = g_LightSource[ind].Pos.GetY();
		g_LightPosZ = g_LightSource[ind].Pos.GetZ();

		g_ConeDirX = g_LightSource[ind].ConeDir.GetX();
		g_ConeDirY = g_LightSource[ind].ConeDir.GetY();
		g_ConeDirZ = g_LightSource[ind].ConeDir.GetZ();
	}
	else
	{
		g_LightSource[ind].Pos.SetX(static_cast<float>(g_LightPosX));
		g_LightSource[ind].Pos.SetY(static_cast<float>(g_LightPosY));
		g_LightSource[ind].Pos.SetZ(static_cast<float>(g_LightPosZ));

		g_LightSource[ind].ConeDir.SetX(static_cast<float>(g_ConeDirX));
		g_LightSource[ind].ConeDir.SetY(static_cast<float>(g_ConeDirY));
		g_LightSource[ind].ConeDir.SetZ(static_cast<float>(g_ConeDirZ));

		Math::Normalize(g_LightSource[ind].ConeDir);
		g_ConeDirX = g_LightSource[ind].ConeDir.GetX();
		g_ConeDirY = g_LightSource[ind].ConeDir.GetY();
		g_ConeDirZ = g_LightSource[ind].ConeDir.GetZ();
	}

	g_LightSource[ind].Color.SetX(static_cast<float>(g_LightColorR));
	g_LightSource[ind].Color.SetY(static_cast<float>(g_LightColorG));
	g_LightSource[ind].Color.SetZ(static_cast<float>(g_LightColorB));

	g_LightSource[ind].ConeInner = g_ConeInner;
	g_LightSource[ind].ConeOuter = g_ConeOuter;

	g_LightSource[ind].LightIntensity = g_LightIntensity;
	g_LightSource[ind].LightRadius = g_LightRadius;

	Lighting::UpdateLightData(0, g_LightSource[ind].Pos, g_LightSource[ind].LightRadius, g_LightSource[ind].Color * g_LightSource[ind].LightIntensity, g_LightSource[ind].ConeDir, g_LightSource[ind].ConeInner, g_LightSource[ind].ConeOuter);
	Lighting::UpdateLightBuffer();
}




void RTModelViewer::SaveLightsInFile()
{
	std::ofstream file("../Data/Lights.lll");

	for (int ind = 0; ind < MAX_LIGHTS; ++ind)
	{
		file << "LIGHT_" + std::to_string(ind) << std::endl
			<< "LightPosX: "		<<			g_LightSource[ind].Pos.GetX()			<< std::endl
			<< "LightPosY: "		<<			g_LightSource[ind].Pos.GetY()			<< std::endl
			<< "LightPosZ: "		<<			g_LightSource[ind].Pos.GetZ()			<< std::endl
			<< "ConeDirX: "			<<			g_LightSource[ind].ConeDir.GetX()		<< std::endl
			<< "ConeDirY: "			<<			g_LightSource[ind].ConeDir.GetY()		<< std::endl
			<< "ConeDirZ: "			<<			g_LightSource[ind].ConeDir.GetZ()		<< std::endl

			<< "LightColorR: "		<<			g_LightSource[ind].Color.GetX()			<< std::endl
			<< "LightColorG: "		<<			g_LightSource[ind].Color.GetY()			<< std::endl
			<< "LightColorB: "		<<			g_LightSource[ind].Color.GetZ()			<< std::endl
			<< "LightIntensity: "	<<			g_LightSource[ind].LightIntensity		<< std::endl

			<< "ConeInner: "		<<			g_LightSource[ind].ConeInner			<< std::endl
			<< "ConeOuter: "		<<			g_LightSource[ind].ConeOuter			<< std::endl

			<< "LightRadius: "		<<			g_LightSource[ind].LightRadius << std::endl;
	}	
}

void RTModelViewer::LoadLightsFromFile()
{
	std::ifstream file("../Data/Lights.lll");

	std::string temp;
	float posX, posY, posZ, coneX, coneY, coneZ, colX, colY, colZ, inten, inn, out, rad;

	for (int ind = 0; ind < MAX_LIGHTS; ++ind)
	{
		float posX, posY, posZ, coneX, coneY, coneZ, colX, colY, colZ, inten, inn, out, rad;
		file >> temp;
		file	>>	temp	>>	posX	>>	temp	>>	posY	>>	temp	>>	posZ
				>>	temp	>>	coneX	>>	temp	>>	coneY	>>	temp	>>	coneZ
				>>	temp	>>	colX	>>	temp	>>	colY	>>	temp	>>	colZ
				>>	temp	>> g_LightSource[ind].LightIntensity >>	temp	>> g_LightSource[ind].ConeInner		
				>>	temp	>> g_LightSource[ind].ConeOuter >>	temp	>> g_LightSource[ind].LightRadius;

		g_LightSource[ind].Pos.SetX(posX);
		g_LightSource[ind].Pos.SetY(posY);
		g_LightSource[ind].Pos.SetZ(posZ);

		g_LightSource[ind].ConeDir.SetX(coneX);
		g_LightSource[ind].ConeDir.SetY(coneY);
		g_LightSource[ind].ConeDir.SetZ(coneZ);

		Math::Normalize(g_LightSource[ind].ConeDir);

		g_LightSource[ind].Color.SetX(colX);
		g_LightSource[ind].Color.SetY(colY);
		g_LightSource[ind].Color.SetZ(colZ);

	}

	UpdateNumVars(EngineVar::ActionType::Increment);
}
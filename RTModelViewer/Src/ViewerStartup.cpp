#include "RTModelViewer.h"


#include <direct.h> // for _getcwd() to check data root path

void LoadIBLTextures()
{
	char CWD[256];
	_getcwd(CWD, 256);

	Utility::Printf("Loading IBL environment maps\n");

	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile(L"../Data/IBLTextures/*_diffuseIBL.dds", &ffd);

	g_IBLSet.AddEnum(L"None");

	if (hFind != INVALID_HANDLE_VALUE) do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		std::wstring diffuseFile = ffd.cFileName;
		std::wstring baseFile = diffuseFile;
		baseFile.resize(baseFile.rfind(L"_diffuseIBL.dds"));
		std::wstring specularFile = baseFile + L"_specularIBL.dds";

		TextureRef diffuseTex = TextureManager::LoadDDSFromFile(L"../Data/IBLTextures/" + diffuseFile);
		if (diffuseTex.IsValid())
		{
			TextureRef specularTex = TextureManager::LoadDDSFromFile(L"../Data/IBLTextures/" + specularFile);
			if (specularTex.IsValid())
			{
				g_IBLSet.AddEnum(baseFile);
				g_IBLTextures.push_back(std::make_pair(diffuseTex, specularTex));
			}
		}
	}	 while (FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);

	Utility::Printf("Found %u IBL environment map sets\n", g_IBLTextures.size());

	if (g_IBLTextures.size() > 0)
		g_IBLSet.Increment();
}



void RTModelViewer::Startup(void)
{
	MotionBlur::Enable = true;
	TemporalEffects::EnableTAA = true;
	FXAA::Enable = true;
	PostEffects::EnableHDR = true;
	PostEffects::EnableAdaptation = true;
	SSAO::Enable = true;

	Renderer::Initialize();

	LoadIBLTextures();
	m_BlueNoiseRGBA = TextureManager::LoadDDSFromFile(L"../Data/BlueNoiseRGBA.dds");
	g_BlueNoiseRGBA = &m_BlueNoiseRGBA;

	std::wstring gltfFileName;

	bool forceRebuild = false;
	uint32_t rebuildValue;
	if (CommandLineArgs::GetInteger(L"rebuild", rebuildValue))
		forceRebuild = rebuildValue != 0;

	forceRebuild = true;

	if (CommandLineArgs::GetString(L"model", gltfFileName) == false)
	{
		m_ModelInst = Renderer::LoadModel(L"../Data/Sponza/sponza2.gltf", forceRebuild);
		m_ModelInst.Resize(g_ModelScale * m_ModelInst.GetRadius());
		OrientedBox obb = m_ModelInst.GetBoundingBox();
		float modelRadius = Length(obb.GetDimensions()) * 0.5f;
		const Vector3 eye = obb.GetCenter() + Vector3(modelRadius * 0.3f, -modelRadius * 0.1f, 0.0f);
		m_Camera.SetEyeAtUp(eye, Vector3(kZero) + Vector3(0.0, modelRadius * 0.1f, 0.0f), Vector3(kYUnitVector));
	}
	else
	{
		m_ModelInst = Renderer::LoadModel(
			gltfFileName,
			forceRebuild);
		m_ModelInst.LoopAllAnimations();
		m_ModelInst.Resize(g_ModelScale * m_ModelInst.GetRadius());

		MotionBlur::Enable = false;
	}

	m_Camera.SetZRange(1.0f, 10000.0f);
	if (gltfFileName.size() == 0)
		m_CameraController.reset(new FlyingFPSCamera(m_Camera, Vector3(kYUnitVector)));
	else
		m_CameraController.reset(new OrbitCamera(m_Camera, m_ModelInst.GetBoundingSphere(), Vector3(kYUnitVector)));


	InitializeRTModel();

	float lightsRadius = 0.1f ;

	Lighting::CreateRandomLights(
		(m_ModelInst.GetCenter() - m_ModelInst.GetBoundingBox().GetDimensions() / 2.0f) * lightsRadius,
		(m_ModelInst.GetCenter() + m_ModelInst.GetBoundingBox().GetDimensions() / 2.0f) * lightsRadius,
		1);

	SetupPredefinedCameraPositions();
}


void RTModelViewer::SetupPredefinedCameraPositions()
{
	m_CameraPosArrayCurrentPosition = 0;

	// Lion's head
	m_CameraPosArray[0].position = Vector3(-11.f, 1.7f, -0.30f) * g_ModelScale;
	m_CameraPosArray[0].heading = 1.5707f;
	m_CameraPosArray[0].pitch = 0.0f;

	// View of columns
	m_CameraPosArray[1].position = Vector3(2.99f, 2.08f, -2.02f) * g_ModelScale;
	m_CameraPosArray[1].heading = -3.1111f;
	m_CameraPosArray[1].pitch = 0.5953f;

	// Bottom-up view from the floor
	m_CameraPosArray[2].position = Vector3(-12.3761f, 0.806f, -0.2602f) * g_ModelScale;
	m_CameraPosArray[2].heading = -1.5707f;
	m_CameraPosArray[2].pitch = 0.268f;

	// Top-down view from the second floor
	m_CameraPosArray[3].position = Vector3(-9.779f, 5.9505f, -1.9497f) * g_ModelScale;
	m_CameraPosArray[3].heading = -2.077f;
	m_CameraPosArray[3].pitch = -0.450f;

	// View of corridors on the second floor
	m_CameraPosArray[4].position = Vector3(-14.63f, 6.f, 3.9452f) * g_ModelScale;
	m_CameraPosArray[4].heading = -1.236f;
	m_CameraPosArray[4].pitch = 0.0f;
}

void RTModelViewer::Cleanup(void)
{
	m_ModelInst = nullptr;

	g_IBLTextures.clear();
	m_BlueNoiseRGBA = 0;

	Lighting::Shutdown();
	Renderer::Shutdown();
}
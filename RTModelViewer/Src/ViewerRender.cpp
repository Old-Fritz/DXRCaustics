#include "RTModelViewer.h"

using Renderer::MeshSorter;

void RTModelViewer::UpdateGlobalConstants(GlobalConstants& globals)
{
	// Update global constants
	float costheta = cosf(g_SunOrientation);
	float sintheta = sinf(g_SunOrientation);
	float cosphi = cosf(g_SunInclination * 3.14159f * 0.5f);
	float sinphi = sinf(g_SunInclination * 3.14159f * 0.5f);

	Vector3 SunDirection = Normalize(Vector3(costheta * cosphi, sinphi, sintheta * cosphi));
	Vector3 ShadowBounds = Vector3(m_ModelInst.GetRadius());
	m_SunShadowCamera.UpdateMatrix(-SunDirection, m_ModelInst.GetCenter(), ShadowBounds * 1.5f,
		(uint32_t)g_ShadowBuffer.GetWidth(), (uint32_t)g_ShadowBuffer.GetHeight(), 1);
	m_SunShadowCamera.SetIntersectAll(true);


	globals.ViewProjMatrix = m_Camera.GetViewProjMatrix();
	globals.SunShadowMatrix = m_SunShadowCamera.GetShadowMatrix();
	globals.CameraPos = Vector4(m_Camera.GetPosition(), 0.0f);
	globals.SunDirection = Vector4(SunDirection, 0.0f);
	globals.SunIntensity = Vector4(Vector3(Scalar(g_SunLightIntensity)), 0.0f);
	globals.AmbientIntensity = Vector4(1.0f, 1.0f, 1.0f, 0.0f) * g_AmbientIntensity;
	globals.ShadowTexelSize = Vector4(1.0f / g_ShadowBuffer.GetWidth(), 0.0f, 0.0f, 0.0f);
	globals.InvTileDim = Vector4(1.0f / Lighting::LightGridDim, 1.0f / Lighting::LightGridDim, 0.0f, 0.0f);
	globals.TileCount.x = Math::DivideByMultiple(g_SceneColorBuffer.GetWidth(), Lighting::LightGridDim);
	globals.TileCount.y = Math::DivideByMultiple(g_SceneColorBuffer.GetHeight(), Lighting::LightGridDim);
	globals.FirstLightIndex.x = Lighting::m_FirstConeLight;
	globals.FirstLightIndex.y = Lighting::m_FirstConeShadowedLight;
	globals.FrameIndexMod2 = TemporalEffects::GetFrameIndexMod2();
}

void RTModelViewer::RenderLightShadows(GraphicsContext& gfxContext, GlobalConstants& globals)
{
	using namespace Lighting;

	ScopedTimer _prof(L"RenderLightShadows", gfxContext);

	for (int i = 0; i < MaxLights; i++)
	{
		wchar_t str[1024];
		wsprintf(str, L"RenderLightShadows: %d", i);
		ScopedTimer _profLocal(str, gfxContext);
		
		MeshSorter shadowSorter(MeshSorter::kShadows);
		shadowSorter.SetCamera(m_LightShadowCamera[i]);
		m_LightShadowArray.SetArrayIndex(i);
		shadowSorter.SetDepthStencilTarget(m_LightShadowArray);

		m_ModelInst.Render(shadowSorter);
		shadowSorter.Sort();

		shadowSorter.RenderMeshes(MeshSorter::kZPass, gfxContext, globals);
	}
}

void RTModelViewer::RenderSunShadow(GraphicsContext& gfxContext, GlobalConstants& globals)
{
	ScopedTimer _prof(L"Sun Shadow Map", gfxContext);

	MeshSorter shadowSorter(MeshSorter::kShadows);
	shadowSorter.SetCamera(m_SunShadowCamera);
	shadowSorter.SetDepthStencilTarget(g_ShadowBuffer);

	m_ModelInst.Render(shadowSorter);

	shadowSorter.Sort();
	shadowSorter.RenderMeshes(MeshSorter::kZPass, gfxContext, globals);
}

void RTModelViewer::RenderZPass(GraphicsContext& gfxContext, MeshSorter& sorter, GlobalConstants& globals)
{
	// Begin rendering depth
	gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
	gfxContext.ClearDepth(g_SceneDepthBuffer);


	sorter.SetCamera(m_Camera);
	sorter.SetViewport(m_MainViewport);
	sorter.SetScissor(m_MainScissor);
	sorter.SetDepthStencilTarget(g_SceneDepthBuffer);
	sorter.AddRenderTarget(g_SceneColorBuffer);

	m_ModelInst.Render(sorter);

	sorter.Sort();

	// Z pre pass
	{
		ScopedTimer _prof(L"Depth Pre-Pass", gfxContext);
		sorter.RenderMeshes(MeshSorter::kZPass, gfxContext, globals);
	}
}

void RTModelViewer::RenderColor(GraphicsContext& gfxContext, MeshSorter& sorter, GlobalConstants& globals)
{
	gfxContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	gfxContext.ClearColor(g_SceneColorBuffer);

	{
		ScopedTimer _prof(L"Render Color", gfxContext);

		gfxContext.TransitionResource(g_SSAOFullScreen, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);
		gfxContext.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV_DepthReadOnly());
		gfxContext.SetViewportAndScissor(m_MainViewport, m_MainScissor);

		sorter.RenderMeshes(MeshSorter::kOpaque, gfxContext, globals);
	}

	Renderer::DrawSkybox(gfxContext, m_Camera, m_MainViewport, m_MainScissor);

	sorter.RenderMeshes(MeshSorter::kTransparent, gfxContext, globals);
}

void RTModelViewer::RenderPostProces(GraphicsContext& gfxContext)
{
	// Some systems generate a per-pixel velocity buffer to better track dynamic and skinned meshes.  Everything
	// is static in our scene, so we generate velocity from camera motion and the depth buffer.  A velocity buffer
	// is necessary for all temporal effects (and motion blur).
	MotionBlur::GenerateCameraVelocityBuffer(gfxContext, m_Camera, true);

	TemporalEffects::ResolveImage(gfxContext);

	ParticleEffectManager::Render(gfxContext, m_Camera, g_SceneColorBuffer, g_SceneDepthBuffer, g_LinearDepth[TemporalEffects::GetFrameIndexMod2()]);

	// Until I work out how to couple these two, it's "either-or".
	if (DepthOfField::Enable)
		DepthOfField::Render(gfxContext, m_Camera.GetNearClip(), m_Camera.GetFarClip());
	else
		MotionBlur::RenderObjectBlur(gfxContext, g_VelocityBuffer);

}

void RTModelViewer::RenderScene(void)
{
	const bool skipDiffusePass =
		rayTracingMode == RTM_DIFFUSE_WITH_SHADOWMAPS ||
		rayTracingMode == RTM_DIFFUSE_WITH_SHADOWRAYS ||
		rayTracingMode == RTM_TRAVERSAL;

	const bool skipShadowMap =
		rayTracingMode == RTM_DIFFUSE_WITH_SHADOWRAYS ||
		rayTracingMode == RTM_TRAVERSAL ||
		rayTracingMode == RTM_SSR;

	GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");

	ParticleEffectManager::Update(gfxContext.GetComputeContext(), Graphics::GetFrameTime());

	GlobalConstants globals;
	UpdateGlobalConstants(globals);

	MeshSorter sorter(MeshSorter::kDefault);
	RenderZPass(gfxContext, sorter, globals);

	// SSAO
	SSAO::Render(gfxContext, m_Camera);

	if (!SSAO::DebugDraw)
	{
		ScopedTimer _outerprof(L"Main Render", gfxContext);

		if (!skipShadowMap)
		{
			RenderSunShadow(gfxContext, globals);
			RenderLightShadows(gfxContext, globals);
		}

		Lighting::FillLightGrid(gfxContext, m_Camera);

		if (!skipDiffusePass)
		{
			RenderColor(gfxContext, sorter, globals);
		}
	}

	UpdateGlobalConstants(globals);

	RenderRaytrace(gfxContext, globals);

	RenderPostProces(gfxContext);

	gfxContext.Finish();
}

void RTModelViewer::RenderUI(class GraphicsContext& gfxContext)
{
	const UINT framesToAverage = 20;
	static float frameRates[framesToAverage] = {};
	frameRates[Graphics::GetFrameCount() % framesToAverage] = Graphics::GetFrameRate();
	float rollingAverageFrameRate = 0.0;
	for (auto frameRate : frameRates)
	{
		rollingAverageFrameRate += frameRate / framesToAverage;
	}

	float primaryRaysPerSec = g_SceneColorBuffer.GetWidth() * g_SceneColorBuffer.GetHeight() * rollingAverageFrameRate / (1000000.0f);
	TextContext text(gfxContext);
	text.Begin();
	text.DrawFormattedString("\nMillion Primary Rays/s: %7.3f", primaryRaysPerSec);
	text.End();
}

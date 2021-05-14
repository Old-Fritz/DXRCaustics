#include "ModelViewer.h"


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
        rayTracingMode = RTM_TRAVERSAL;
    else if (GameInput::IsFirstPressed(GameInput::kKey_3))
        rayTracingMode = RTM_SSR;
    else if (GameInput::IsFirstPressed(GameInput::kKey_4))
        rayTracingMode = RTM_SHADOWS;
    else if (GameInput::IsFirstPressed(GameInput::kKey_5))
        rayTracingMode = RTM_DIFFUSE_WITH_SHADOWMAPS;
    else if (GameInput::IsFirstPressed(GameInput::kKey_6))
        rayTracingMode = RTM_DIFFUSE_WITH_SHADOWRAYS;
    else if (GameInput::IsFirstPressed(GameInput::kKey_7))
        rayTracingMode = RTM_REFLECTIONS;

    static bool freezeCamera = false;

    if (GameInput::IsFirstPressed(GameInput::kKey_f))
    {
        freezeCamera = !freezeCamera;
    }

    if (GameInput::IsFirstPressed(GameInput::kKey_left))
    {
        m_CameraPosArrayCurrentPosition = (m_CameraPosArrayCurrentPosition + c_NumCameraPositions - 1) % c_NumCameraPositions;
        SetCameraToPredefinedPosition(m_CameraPosArrayCurrentPosition);
    }
    else if (GameInput::IsFirstPressed(GameInput::kKey_right))
    {
        m_CameraPosArrayCurrentPosition = (m_CameraPosArrayCurrentPosition + 1) % c_NumCameraPositions;
        SetCameraToPredefinedPosition(m_CameraPosArrayCurrentPosition);
    }

    if (!freezeCamera)
    {
        m_CameraController->Update(deltaT);
    }

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
}

void RTModelViewer::SetCameraToPredefinedPosition(int cameraPosition)
{
    if (cameraPosition < 0 || cameraPosition >= c_NumCameraPositions)
        return;

    m_CameraController->SetHeadingPitchAndPosition(
        m_CameraPosArray[m_CameraPosArrayCurrentPosition].heading,
        m_CameraPosArray[m_CameraPosArrayCurrentPosition].pitch,
        m_CameraPosArray[m_CameraPosArrayCurrentPosition].position);
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

    uint32_t FrameIndex = TemporalEffects::GetFrameIndexMod2();
    const D3D12_VIEWPORT& viewport = m_MainViewport;
    const D3D12_RECT& scissor = m_MainScissor;

    ParticleEffectManager::Update(gfxContext.GetComputeContext(), Graphics::GetFrameTime());

    Sponza::RenderScene(gfxContext, m_Camera, viewport, scissor, skipDiffusePass, skipShadowMap);

    // Some systems generate a per-pixel velocity buffer to better track dynamic and skinned meshes.  Everything
    // is static in our scene, so we generate velocity from camera motion and the depth buffer.  A velocity buffer
    // is necessary for all temporal effects (and motion blur).
    MotionBlur::GenerateCameraVelocityBuffer(gfxContext, m_Camera, true);

    TemporalEffects::ResolveImage(gfxContext);

    ParticleEffectManager::Render(gfxContext, m_Camera, g_SceneColorBuffer, g_SceneDepthBuffer, g_LinearDepth[FrameIndex]);

    // Until I work out how to couple these two, it's "either-or".
    if (DepthOfField::Enable)
        DepthOfField::Render(gfxContext, m_Camera.GetNearClip(), m_Camera.GetFarClip());
    else
        MotionBlur::RenderObjectBlur(gfxContext, g_VelocityBuffer);

    Raytrace(gfxContext);
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

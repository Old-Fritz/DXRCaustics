#include "RTModelViewer.h"

D3D12_STATE_SUBOBJECT CreateDxilLibrary(LPCWSTR entrypoint, const void* pShaderByteCode, SIZE_T bytecodeLength, D3D12_DXIL_LIBRARY_DESC& dxilLibDesc, D3D12_EXPORT_DESC& exportDesc)
{
	exportDesc = { entrypoint, nullptr, D3D12_EXPORT_FLAG_NONE };
	D3D12_STATE_SUBOBJECT dxilLibSubObject = {};
	dxilLibDesc.DXILLibrary.pShaderBytecode = pShaderByteCode;
	dxilLibDesc.DXILLibrary.BytecodeLength = bytecodeLength;
	dxilLibDesc.NumExports = 1;
	dxilLibDesc.pExports = &exportDesc;
	dxilLibSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	dxilLibSubObject.pDesc = &dxilLibDesc;
	return dxilLibSubObject;
}

void SetPipelineStateStackSize(LPCWSTR raygen, LPCWSTR closestHit, LPCWSTR miss, UINT maxRecursion, ComPtr<ID3D12StateObject> pStateObject)
{
	ComPtr < ID3D12StateObjectProperties> stateObjectProperties = nullptr;
	ThrowIfFailed(pStateObject->QueryInterface(IID_PPV_ARGS_(stateObjectProperties)));
	UINT64 closestHitStackSize = stateObjectProperties->GetShaderStackSize(closestHit);
	UINT64 missStackSize = stateObjectProperties->GetShaderStackSize(miss);
	UINT64 raygenStackSize = stateObjectProperties->GetShaderStackSize(raygen);

	UINT64 totalStackSize = raygenStackSize + std::max(missStackSize, closestHitStackSize) * maxRecursion;
	stateObjectProperties->SetPipelineStackSize(totalStackSize);
}

void RTModelViewer::InitializeRaytracingStateObjects()
{
	ZeroMemory(&g_dynamicCb, sizeof(g_dynamicCb));

	auto& model = m_ModelInst;
	UINT numMeshes = model.GetModel()->m_NumMeshes;

	InitGlobalRootSignature();
	InitLocalRootSignature();

	std::vector<D3D12_STATE_SUBOBJECT> subObjects;

	UINT nodeMask;
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
	InitSubObjectsConfig(subObjects, nodeMask, pipelineConfig);

	ShaderExport exports[SEN_NumExports];
	InitSubObjectsTraceShaders(subObjects, exports);

	LPCWSTR hitGroupNames[HG_NumGroups];
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig;
	D3D12_HIT_GROUP_DESC hitGroupDesc = {};
	InitHitGroups(subObjects, shaderConfig, hitGroupDesc, hitGroupNames, exports);


	D3D12_STATE_OBJECT_DESC stateObject;
	stateObject.NumSubobjects = (UINT)subObjects.size();
	stateObject.pSubobjects = subObjects.data();
	stateObject.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;


	const UINT shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
#define ALIGN(alignment, num) ((((num) + alignment - 1) / alignment) * alignment)
	const UINT offsetToDescriptorHandle = ALIGN(sizeof(D3D12_GPU_DESCRIPTOR_HANDLE), shaderIdentifierSize);
	const UINT offsetToMaterialConstants = ALIGN(sizeof(UINT32), offsetToDescriptorHandle + sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
	const UINT shaderRecordSizeInBytes = ALIGN(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, offsetToMaterialConstants + sizeof(MaterialRootConstant));

	std::vector<byte> pHitShaderTable(shaderRecordSizeInBytes * numMeshes);
	auto GetShaderTable = [=, &model](ComPtr<ID3D12StateObject> pPSO, byte* pShaderTable)
	{
		ComPtr<ID3D12StateObjectProperties> stateObjectProperties = nullptr;
		ThrowIfFailed(pPSO->QueryInterface(IID_PPV_ARGS_(stateObjectProperties)));
		void* pHitGroupIdentifierData = stateObjectProperties->GetShaderIdentifier(hitGroupNames[HG_HitGroup]);

		auto iterator = model.GetModel()->GetMeshIterator();
		for (UINT i = 0; i < numMeshes; i++)
		{
			byte* pShaderRecord = i * shaderRecordSizeInBytes + pShaderTable;
			memcpy(pShaderRecord, pHitGroupIdentifierData, shaderIdentifierSize);

			UINT materialIndex = iterator.GetMesh(i)->materialCBV;
			memcpy(pShaderRecord + offsetToDescriptorHandle, &g_GpuSceneMaterialSrvs[materialIndex].ptr, sizeof(g_GpuSceneMaterialSrvs[materialIndex].ptr));

			MaterialRootConstant material;
			material.MaterialID = i;
			memcpy(pShaderRecord + offsetToMaterialConstants, &material, sizeof(material));
		}
	};

	InitRayTraceInputs(GetShaderTable, stateObject, pHitShaderTable, shaderRecordSizeInBytes, exports);

	for (auto& raytracingPipelineState : g_RaytracingInputs)
	{
		WCHAR hitGroupExportNameClosestHitType[64];
		swprintf_s(hitGroupExportNameClosestHitType, L"%s::closesthit", hitGroupNames[HG_HitGroup]);
		SetPipelineStateStackSize(exports[SEN_RayGen].exportName, hitGroupExportNameClosestHitType, exports[SEN_Miss].exportName, MaxRayRecursion, raytracingPipelineState.m_pPSO);
	}
}

constexpr UINT cBaseColorSamplerRegister			= 0;
constexpr UINT cMetallicRoughnessSamplerRegister	= 1;
constexpr UINT cOcclusionSamplerRegister			= 2;
constexpr UINT cEmissiveSamplerRegster				= 3;
constexpr UINT cNormalSamplerRegister				= 4;

constexpr UINT cDefaultSamplerRegister				= 10;
constexpr UINT cShadowSamplerRegister				= 11;
constexpr UINT cCubeMapSamplerRegister				= 12;
constexpr UINT cNumSamplers							= 8;


void RTModelViewer::InitStaticSamplers(D3D12_STATIC_SAMPLER_DESC* descs)
{
	SamplerDesc defaultSamplerDesc;
	defaultSamplerDesc.MaxAnisotropy = 8;

	SamplerDesc cubeMapSamplerDesc = defaultSamplerDesc;


	auto fillStaticSamplerDesc = [](const SamplerDesc& srcDesc, D3D12_STATIC_SAMPLER_DESC& destDesc, UINT shaderReg)
	{
		destDesc.Filter				= srcDesc.Filter;
		destDesc.AddressU			= srcDesc.AddressU;
		destDesc.AddressV			= srcDesc.AddressV;
		destDesc.AddressW			= srcDesc.AddressW;
		destDesc.MipLODBias			= srcDesc.MipLODBias;
		destDesc.MaxAnisotropy		= srcDesc.MaxAnisotropy;
		destDesc.ComparisonFunc		= srcDesc.ComparisonFunc;
		destDesc.BorderColor		= D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		destDesc.MinLOD				= srcDesc.MinLOD;
		destDesc.MaxLOD				= srcDesc.MaxLOD;
		destDesc.ShaderRegister		= shaderReg;
		destDesc.RegisterSpace		= 0;
		destDesc.ShaderVisibility	= D3D12_SHADER_VISIBILITY_ALL;
	};

	UINT sInd = 0;
	fillStaticSamplerDesc(defaultSamplerDesc,	descs[sInd++], cDefaultSamplerRegister);
	fillStaticSamplerDesc(SamplerShadowDesc,	descs[sInd++], cShadowSamplerRegister);
	fillStaticSamplerDesc(cubeMapSamplerDesc,	descs[sInd++], cCubeMapSamplerRegister);

	fillStaticSamplerDesc(defaultSamplerDesc, descs[sInd++], cBaseColorSamplerRegister);
	fillStaticSamplerDesc(defaultSamplerDesc, descs[sInd++], cMetallicRoughnessSamplerRegister);
	fillStaticSamplerDesc(defaultSamplerDesc, descs[sInd++], cOcclusionSamplerRegister);
	fillStaticSamplerDesc(defaultSamplerDesc, descs[sInd++], cEmissiveSamplerRegster);
	fillStaticSamplerDesc(defaultSamplerDesc, descs[sInd++], cNormalSamplerRegister);
}

void RTModelViewer::InitGlobalRootSignature()
{
	D3D12_STATIC_SAMPLER_DESC staticSamplerDescs[cNumSamplers] = {};
	InitStaticSamplers(staticSamplerDescs);

	// scene srv
	D3D12_DESCRIPTOR_RANGE1 sceneBuffersDescriptorRange = {};
	sceneBuffersDescriptorRange.BaseShaderRegister = 1;
	sceneBuffersDescriptorRange.NumDescriptors = 5;
	sceneBuffersDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	sceneBuffersDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	// lighting
	D3D12_DESCRIPTOR_RANGE1 lightingBuffersDescriptorRange = {};
	lightingBuffersDescriptorRange.BaseShaderRegister = 11;
	lightingBuffersDescriptorRange.NumDescriptors = 9;
	lightingBuffersDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	lightingBuffersDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	// gbuffer
	D3D12_DESCRIPTOR_RANGE1 GBufferDescriptorRange = {};
	GBufferDescriptorRange.BaseShaderRegister = 20;
	GBufferDescriptorRange.NumDescriptors = ((uint32_t)GBTarget::NumTargets + 1) * 2; // gbuffer and array gbuffer 
	GBufferDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	GBufferDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	// outputs
	D3D12_DESCRIPTOR_RANGE1 uavDescriptorRange = {};
	uavDescriptorRange.BaseShaderRegister = 2;
	uavDescriptorRange.NumDescriptors = 10;
	uavDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	uavDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	CD3DX12_ROOT_PARAMETER1 globalRootSignatureParameters[7];
	globalRootSignatureParameters[0].InitAsDescriptorTable(1, &sceneBuffersDescriptorRange); // scene srv
	globalRootSignatureParameters[1].InitAsConstantBufferView(0); // hit CB
	globalRootSignatureParameters[2].InitAsConstantBufferView(1); // dyn CB
	globalRootSignatureParameters[3].InitAsDescriptorTable(1, &lightingBuffersDescriptorRange); // lighting
	globalRootSignatureParameters[4].InitAsDescriptorTable(1, &uavDescriptorRange); // outputs
	globalRootSignatureParameters[5].InitAsDescriptorTable(1, &GBufferDescriptorRange); // gbuffer
	globalRootSignatureParameters[6].InitAsShaderResourceView(0);
	auto globalRootSignatureDesc = CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(
		ARRAYSIZE(globalRootSignatureParameters), globalRootSignatureParameters,
		ARRAYSIZE(staticSamplerDescs), staticSamplerDescs);

	ComPtr<ID3DBlob> pGlobalRootSignatureBlob;
	ComPtr<ID3DBlob> pErrorBlob;
	if (FAILED(D3D12SerializeVersionedRootSignature(&globalRootSignatureDesc, &pGlobalRootSignatureBlob, &pErrorBlob)))
	{
		OutputDebugStringA((LPCSTR)pErrorBlob->GetBufferPointer());
	}
	ASSERT_SUCCEEDED(g_pRaytracingDevice->CreateRootSignature(
		0,
		pGlobalRootSignatureBlob->GetBufferPointer(),
		pGlobalRootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS_(g_GlobalRaytracingRootSignature)));
}

void RTModelViewer::InitLocalRootSignature()
{
	D3D12_DESCRIPTOR_RANGE1 localTextureDescriptorRange = {};
	localTextureDescriptorRange.BaseShaderRegister = 6;
	localTextureDescriptorRange.NumDescriptors = kNumTextures;
	localTextureDescriptorRange.RegisterSpace = 0;
	localTextureDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	localTextureDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	CD3DX12_ROOT_PARAMETER1 localRootSignatureParameters[2];
	UINT sizeOfRootConstantInDwords = (sizeof(MaterialRootConstant) - 1) / sizeof(DWORD) + 1;
	localRootSignatureParameters[0].InitAsDescriptorTable(1, &localTextureDescriptorRange);
	localRootSignatureParameters[1].InitAsConstants(sizeOfRootConstantInDwords, 2);
	auto localRootSignatureDesc = CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(ARRAYSIZE(localRootSignatureParameters), localRootSignatureParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

	ComPtr<ID3DBlob> pLocalRootSignatureBlob;
	D3D12SerializeVersionedRootSignature(&localRootSignatureDesc, &pLocalRootSignatureBlob, nullptr);
	g_pRaytracingDevice->CreateRootSignature(0, pLocalRootSignatureBlob->GetBufferPointer(), pLocalRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS_(g_LocalRaytracingRootSignature));
}

void RTModelViewer::InitSubObjectsConfig(std::vector<D3D12_STATE_SUBOBJECT>& subObjects, UINT& nodeMask, D3D12_RAYTRACING_PIPELINE_CONFIG& pipelineConfig)
{
	D3D12_STATE_SUBOBJECT nodeMaskSubObject;
	nodeMask = 1;
	nodeMaskSubObject.pDesc = &nodeMask;
	nodeMaskSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK;
	subObjects.push_back(nodeMaskSubObject);

	D3D12_STATE_SUBOBJECT rootSignatureSubObject;
	rootSignatureSubObject.pDesc = g_GlobalRaytracingRootSignature.GetAddressOf();
	rootSignatureSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	subObjects.push_back(rootSignatureSubObject);

	D3D12_STATE_SUBOBJECT configurationSubObject;
	pipelineConfig.MaxTraceRecursionDepth = MaxRayRecursion;
	configurationSubObject.pDesc = &pipelineConfig;
	configurationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	subObjects.push_back(configurationSubObject);
}

void RTModelViewer::InitSubObjectsTraceShaders(std::vector<D3D12_STATE_SUBOBJECT>& subObjects, ShaderExport exports[SEN_NumExports])
{
	// Ray Gen shader stuff
	// ----------------------------------------------------------------//
	exports[SEN_RayGen].exportName = L"RayGen";
	D3D12_STATE_SUBOBJECT rayGenDxilLibSubobject = CreateDxilLibrary(exports[SEN_RayGen].exportName, g_pRGS_Diffuse,
		sizeof(g_pRGS_Diffuse), exports[SEN_RayGen].dxilLibDesc, exports[SEN_RayGen].exportDesc);

	subObjects.push_back(rayGenDxilLibSubobject);
	exports[SEN_RayGen].pSubObject = &subObjects.back();

	// Closest Hit shader stuff
	// ----------------------------------------------------------------//
	exports[SEN_ClosestHit].exportName = L"ClosestHit";
	D3D12_STATE_SUBOBJECT closestHitLibSubobject = CreateDxilLibrary(exports[SEN_ClosestHit].exportName, g_pCHS_Default,
		sizeof(g_pCHS_Default), exports[SEN_ClosestHit].dxilLibDesc, exports[SEN_ClosestHit].exportDesc);

	subObjects.push_back(closestHitLibSubobject);
	exports[SEN_ClosestHit].pSubObject = &subObjects.back();

	// Any Hit shader stuff
	// ----------------------------------------------------------------//
	exports[SEN_AnyHit].exportName = L"AnyHit";
	D3D12_STATE_SUBOBJECT anyHitLibSubobject = CreateDxilLibrary(exports[SEN_AnyHit].exportName, g_pAHS_Default,
		sizeof(g_pAHS_Default), exports[SEN_AnyHit].dxilLibDesc, exports[SEN_AnyHit].exportDesc);

	subObjects.push_back(anyHitLibSubobject);
	exports[SEN_AnyHit].pSubObject = &subObjects.back();

	// Miss shader stuff
	// ----------------------------------------------------------------//
	exports[SEN_Miss].exportName = L"Miss";
	D3D12_STATE_SUBOBJECT missLibSubobject = CreateDxilLibrary(exports[SEN_Miss].exportName, g_pMS_Default,
		sizeof(g_pMS_Default), exports[SEN_Miss].dxilLibDesc, exports[SEN_Miss].exportDesc);

	subObjects.push_back(missLibSubobject);
	exports[SEN_Miss].pSubObject = &subObjects.back();
}

void RTModelViewer::InitHitGroups(std::vector<D3D12_STATE_SUBOBJECT>& subObjects, D3D12_RAYTRACING_SHADER_CONFIG& shaderConfig,
	D3D12_HIT_GROUP_DESC& hitGroupDesc, LPCWSTR hitGroupNames[HG_NumGroups], ShaderExport exports[SEN_NumExports])
{
	D3D12_STATE_SUBOBJECT shaderConfigStateObject;
	shaderConfig.MaxAttributeSizeInBytes = 8;
	shaderConfig.MaxPayloadSizeInBytes = 24;
	shaderConfigStateObject.pDesc = &shaderConfig;
	shaderConfigStateObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	subObjects.push_back(shaderConfigStateObject);

	LPCWSTR hitGroupExportName = L"HitGroup";
	hitGroupDesc.AnyHitShaderImport = exports[SEN_AnyHit].exportName;
	hitGroupDesc.ClosestHitShaderImport = exports[SEN_ClosestHit].exportName;
	hitGroupDesc.HitGroupExport = hitGroupExportName;
	D3D12_STATE_SUBOBJECT hitGroupSubobject = {};
	hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	hitGroupSubobject.pDesc = &hitGroupDesc;

	subObjects.push_back(hitGroupSubobject);
	hitGroupNames[HG_HitGroup] = hitGroupExportName;

	D3D12_STATE_SUBOBJECT localRootSignatureSubObject;
	localRootSignatureSubObject.pDesc = g_LocalRaytracingRootSignature.GetAddressOf();
	localRootSignatureSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	subObjects.push_back(localRootSignatureSubObject);
}

void RTModelViewer::InitRayTraceInputs(std::function<void(ComPtr<ID3D12StateObject>, byte*)> GetShaderTable, D3D12_STATE_OBJECT_DESC& stateObject,
	std::vector<byte>& pHitShaderTable, UINT shaderRecordSizeInBytes, ShaderExport exports[SEN_NumExports])
{
	{
		ComPtr<ID3D12StateObject> pbarycentricPSO;
		g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS_(pbarycentricPSO));
		GetShaderTable(pbarycentricPSO, pHitShaderTable.data());
		g_RaytracingInputs[Primarybarycentric] = RaytracingDispatchRayInputs(*g_pRaytracingDevice.Get(), pbarycentricPSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), exports[SEN_RayGen].exportName, exports[SEN_Miss].exportName);
	}

	{
		*exports[SEN_RayGen].pSubObject = CreateDxilLibrary(exports[SEN_RayGen].exportName, g_pRGS_SSR, sizeof(g_pRGS_SSR), exports[SEN_RayGen].dxilLibDesc, exports[SEN_RayGen].exportDesc);
		ComPtr<ID3D12StateObject> pReflectionbarycentricPSO;
		HRESULT res = g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS_(pReflectionbarycentricPSO));
		ThrowIfFailed(res);
		GetShaderTable(pReflectionbarycentricPSO, pHitShaderTable.data());
		g_RaytracingInputs[Reflectionbarycentric] = RaytracingDispatchRayInputs(*g_pRaytracingDevice.Get(), pReflectionbarycentricPSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), exports[SEN_RayGen].exportName, exports[SEN_Miss].exportName);
	}

	{
		*exports[SEN_RayGen].pSubObject = CreateDxilLibrary(exports[SEN_RayGen].exportName, g_pRGS_Shadows, sizeof(g_pRGS_Shadows), exports[SEN_RayGen].dxilLibDesc, exports[SEN_RayGen].exportDesc);
		*exports[SEN_Miss].pSubObject = CreateDxilLibrary(exports[SEN_Miss].exportName, g_pMS_Shadows, sizeof(g_pMS_Shadows), exports[SEN_Miss].dxilLibDesc, exports[SEN_Miss].exportDesc);

		ComPtr<ID3D12StateObject> pShadowsPSO;
		HRESULT res = g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS_(pShadowsPSO));
		ThrowIfFailed(res);
		GetShaderTable(pShadowsPSO, pHitShaderTable.data());
		g_RaytracingInputs[Shadows] = RaytracingDispatchRayInputs(*(g_pRaytracingDevice).Get(), pShadowsPSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), exports[SEN_RayGen].exportName, exports[SEN_Miss].exportName);
	}

	{
		*exports[SEN_RayGen].pSubObject = CreateDxilLibrary(exports[SEN_RayGen].exportName, g_pRGS_Diffuse, sizeof(g_pRGS_Diffuse), exports[SEN_RayGen].dxilLibDesc, exports[SEN_RayGen].exportDesc);
		*exports[SEN_AnyHit].pSubObject = CreateDxilLibrary(exports[SEN_AnyHit].exportName, g_pAHS_Default, sizeof(g_pAHS_Default), exports[SEN_AnyHit].dxilLibDesc, exports[SEN_AnyHit].exportDesc);
		*exports[SEN_ClosestHit].pSubObject = CreateDxilLibrary(exports[SEN_ClosestHit].exportName, g_pCHS_Diffuse, sizeof(g_pCHS_Diffuse), exports[SEN_ClosestHit].dxilLibDesc, exports[SEN_ClosestHit].exportDesc);
		*exports[SEN_Miss].pSubObject = CreateDxilLibrary(exports[SEN_Miss].exportName, g_pMS_Default, sizeof(g_pMS_Default), exports[SEN_Miss].dxilLibDesc, exports[SEN_Miss].exportDesc);

		ComPtr<ID3D12StateObject> pDiffusePSO;
		HRESULT res = g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS_(pDiffusePSO));
		ThrowIfFailed(res);
		GetShaderTable(pDiffusePSO, pHitShaderTable.data());
		g_RaytracingInputs[DiffuseHitShader] = RaytracingDispatchRayInputs(*g_pRaytracingDevice.Get(), pDiffusePSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), exports[SEN_RayGen].exportName, exports[SEN_Miss].exportName);
	}

	{
		*exports[SEN_RayGen].pSubObject = CreateDxilLibrary(exports[SEN_RayGen].exportName, g_pRGS_SSR, sizeof(g_pRGS_SSR), exports[SEN_RayGen].dxilLibDesc, exports[SEN_RayGen].exportDesc);
		*exports[SEN_AnyHit].pSubObject = CreateDxilLibrary(exports[SEN_AnyHit].exportName, g_pAHS_Default, sizeof(g_pAHS_Default), exports[SEN_AnyHit].dxilLibDesc, exports[SEN_AnyHit].exportDesc);
		*exports[SEN_ClosestHit].pSubObject = CreateDxilLibrary(exports[SEN_ClosestHit].exportName, g_pCHS_Diffuse, sizeof(g_pCHS_Diffuse), exports[SEN_ClosestHit].dxilLibDesc, exports[SEN_ClosestHit].exportDesc);
		*exports[SEN_Miss].pSubObject = CreateDxilLibrary(exports[SEN_Miss].exportName, g_pMS_Default, sizeof(g_pMS_Default), exports[SEN_Miss].dxilLibDesc, exports[SEN_Miss].exportDesc);

		ComPtr<ID3D12StateObject> pReflectionPSO;
		HRESULT res = g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS_(pReflectionPSO));
		ThrowIfFailed(res);
		GetShaderTable(pReflectionPSO, pHitShaderTable.data());
		g_RaytracingInputs[Reflection] = RaytracingDispatchRayInputs(*g_pRaytracingDevice.Get(), pReflectionPSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), exports[SEN_RayGen].exportName, exports[SEN_Miss].exportName);
	}

	{
		*exports[SEN_RayGen].pSubObject = CreateDxilLibrary(exports[SEN_RayGen].exportName, g_pRGS_Backward, sizeof(g_pRGS_Backward), exports[SEN_RayGen].dxilLibDesc, exports[SEN_RayGen].exportDesc);
		*exports[SEN_AnyHit].pSubObject = CreateDxilLibrary(exports[SEN_AnyHit].exportName, g_pAHS_Backward, sizeof(g_pAHS_Backward), exports[SEN_AnyHit].dxilLibDesc, exports[SEN_AnyHit].exportDesc);
		*exports[SEN_ClosestHit].pSubObject = CreateDxilLibrary(exports[SEN_ClosestHit].exportName, g_pCHS_Backward, sizeof(g_pCHS_Backward), exports[SEN_ClosestHit].dxilLibDesc, exports[SEN_ClosestHit].exportDesc);
		*exports[SEN_Miss].pSubObject = CreateDxilLibrary(exports[SEN_Miss].exportName, g_pMS_Backward, sizeof(g_pMS_Backward), exports[SEN_Miss].dxilLibDesc, exports[SEN_Miss].exportDesc);

		ComPtr<ID3D12StateObject> pBackwardPSO;
		HRESULT res = g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS_(pBackwardPSO));
		ThrowIfFailed(res);
		GetShaderTable(pBackwardPSO, pHitShaderTable.data());
		g_RaytracingInputs[Backward] = RaytracingDispatchRayInputs(*g_pRaytracingDevice.Get(), pBackwardPSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), exports[SEN_RayGen].exportName, exports[SEN_Miss].exportName);
	}

	{
		*exports[SEN_RayGen].pSubObject = CreateDxilLibrary(exports[SEN_RayGen].exportName, g_pRGS_Caustic, sizeof(g_pRGS_Caustic), exports[SEN_RayGen].dxilLibDesc, exports[SEN_RayGen].exportDesc);
		*exports[SEN_AnyHit].pSubObject = CreateDxilLibrary(exports[SEN_AnyHit].exportName, g_pAHS_Caustic, sizeof(g_pAHS_Caustic), exports[SEN_AnyHit].dxilLibDesc, exports[SEN_AnyHit].exportDesc);
		*exports[SEN_ClosestHit].pSubObject = CreateDxilLibrary(exports[SEN_ClosestHit].exportName, g_pCHS_Caustic, sizeof(g_pCHS_Caustic), exports[SEN_ClosestHit].dxilLibDesc, exports[SEN_ClosestHit].exportDesc);
		*exports[SEN_Miss].pSubObject = CreateDxilLibrary(exports[SEN_Miss].exportName, g_pMS_Caustic, sizeof(g_pMS_Caustic), exports[SEN_Miss].dxilLibDesc, exports[SEN_Miss].exportDesc);

		ComPtr<ID3D12StateObject> pCausticPSO;
		HRESULT res = g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS_(pCausticPSO));
		ThrowIfFailed(res);
		GetShaderTable(pCausticPSO, pHitShaderTable.data());
		g_RaytracingInputs[Caustic] = RaytracingDispatchRayInputs(*g_pRaytracingDevice.Get(), pCausticPSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), exports[SEN_RayGen].exportName, exports[SEN_Miss].exportName);
	}
}

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

void SetPipelineStateStackSize(LPCWSTR raygen, LPCWSTR closestHit, LPCWSTR miss, UINT maxRecursion, ID3D12StateObject* pStateObject)
{
	ID3D12StateObjectProperties* stateObjectProperties = nullptr;
	ThrowIfFailed(pStateObject->QueryInterface(IID_PPV_ARGS(&stateObjectProperties)));
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
	auto GetShaderTable = [=, &model](ID3D12StateObject* pPSO, byte* pShaderTable)
	{
		ID3D12StateObjectProperties* stateObjectProperties = nullptr;
		ThrowIfFailed(pPSO->QueryInterface(IID_PPV_ARGS(&stateObjectProperties)));
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

void RTModelViewer::InitStaticSamplers(D3D12_STATIC_SAMPLER_DESC* descs)
{
	D3D12_STATIC_SAMPLER_DESC& defaultSampler = descs[0];
	defaultSampler.Filter = D3D12_FILTER_ANISOTROPIC;
	defaultSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	defaultSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	defaultSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	defaultSampler.MipLODBias = 0.0f;
	defaultSampler.MaxAnisotropy = 16;
	defaultSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	defaultSampler.MinLOD = 0.0f;
	defaultSampler.MaxLOD = D3D12_FLOAT32_MAX;
	defaultSampler.MaxAnisotropy = 8;
	defaultSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	defaultSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	defaultSampler.ShaderRegister = 0;

	D3D12_STATIC_SAMPLER_DESC& shadowSampler = descs[1];
	shadowSampler = descs[0];
	shadowSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	shadowSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	shadowSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	shadowSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	shadowSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	shadowSampler.ShaderRegister = 1;
}

void RTModelViewer::InitGlobalRootSignature()
{
	D3D12_STATIC_SAMPLER_DESC staticSamplerDescs[2] = {};
	InitStaticSamplers(staticSamplerDescs);

	// scene srv
	D3D12_DESCRIPTOR_RANGE1 sceneBuffersDescriptorRange = {};
	sceneBuffersDescriptorRange.BaseShaderRegister = 1;
	sceneBuffersDescriptorRange.NumDescriptors = 6;
	sceneBuffersDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	sceneBuffersDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	// depth/normals
	D3D12_DESCRIPTOR_RANGE1 srvDescriptorRange = {};
	srvDescriptorRange.BaseShaderRegister = 12;
	srvDescriptorRange.NumDescriptors = 2;
	srvDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	// outputs
	D3D12_DESCRIPTOR_RANGE1 uavDescriptorRange = {};
	uavDescriptorRange.BaseShaderRegister = 2;
	uavDescriptorRange.NumDescriptors = 10;
	uavDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	uavDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	CD3DX12_ROOT_PARAMETER1 globalRootSignatureParameters[8];
	globalRootSignatureParameters[0].InitAsDescriptorTable(1, &sceneBuffersDescriptorRange); // scene srv
	globalRootSignatureParameters[1].InitAsConstantBufferView(0); // hit CB
	globalRootSignatureParameters[2].InitAsConstantBufferView(1); // dyn CB
	globalRootSignatureParameters[3].InitAsDescriptorTable(1, &srvDescriptorRange); // d/n
	globalRootSignatureParameters[4].InitAsDescriptorTable(1, &uavDescriptorRange); // outputs
	globalRootSignatureParameters[5].InitAsUnorderedAccessView(0);
	globalRootSignatureParameters[6].InitAsUnorderedAccessView(1);
	globalRootSignatureParameters[7].InitAsShaderResourceView(0);
	auto globalRootSignatureDesc = CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(
		ARRAYSIZE(globalRootSignatureParameters), globalRootSignatureParameters,
		ARRAYSIZE(staticSamplerDescs), staticSamplerDescs);

	CComPtr<ID3DBlob> pGlobalRootSignatureBlob;
	CComPtr<ID3DBlob> pErrorBlob;
	if (FAILED(D3D12SerializeVersionedRootSignature(&globalRootSignatureDesc, &pGlobalRootSignatureBlob, &pErrorBlob)))
	{
		OutputDebugStringA((LPCSTR)pErrorBlob->GetBufferPointer());
	}
	ASSERT_SUCCEEDED(g_pRaytracingDevice->CreateRootSignature(
		0,
		pGlobalRootSignatureBlob->GetBufferPointer(),
		pGlobalRootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&g_GlobalRaytracingRootSignature)));
}

void RTModelViewer::InitLocalRootSignature()
{
	D3D12_DESCRIPTOR_RANGE1 localTextureDescriptorRange = {};
	localTextureDescriptorRange.BaseShaderRegister = 7;
	localTextureDescriptorRange.NumDescriptors = kNumTextures;
	localTextureDescriptorRange.RegisterSpace = 0;
	localTextureDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	localTextureDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	CD3DX12_ROOT_PARAMETER1 localRootSignatureParameters[2];
	UINT sizeOfRootConstantInDwords = (sizeof(MaterialRootConstant) - 1) / sizeof(DWORD) + 1;
	localRootSignatureParameters[0].InitAsDescriptorTable(1, &localTextureDescriptorRange);
	localRootSignatureParameters[1].InitAsConstants(sizeOfRootConstantInDwords, 3);
	auto localRootSignatureDesc = CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(ARRAYSIZE(localRootSignatureParameters), localRootSignatureParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

	CComPtr<ID3DBlob> pLocalRootSignatureBlob;
	D3D12SerializeVersionedRootSignature(&localRootSignatureDesc, &pLocalRootSignatureBlob, nullptr);
	g_pRaytracingDevice->CreateRootSignature(0, pLocalRootSignatureBlob->GetBufferPointer(), pLocalRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&g_LocalRaytracingRootSignature));
}

void RTModelViewer::InitSubObjectsConfig(std::vector<D3D12_STATE_SUBOBJECT>& subObjects, UINT& nodeMask, D3D12_RAYTRACING_PIPELINE_CONFIG& pipelineConfig)
{
	D3D12_STATE_SUBOBJECT nodeMaskSubObject;
	nodeMask = 1;
	nodeMaskSubObject.pDesc = &nodeMask;
	nodeMaskSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK;
	subObjects.push_back(nodeMaskSubObject);

	D3D12_STATE_SUBOBJECT rootSignatureSubObject;
	rootSignatureSubObject.pDesc = &g_GlobalRaytracingRootSignature.p;
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
	D3D12_STATE_SUBOBJECT rayGenDxilLibSubobject = CreateDxilLibrary(exports[SEN_RayGen].exportName, g_pRayGenerationShaderLib,
		sizeof(g_pRayGenerationShaderLib), exports[SEN_RayGen].dxilLibDesc, exports[SEN_RayGen].exportDesc);

	subObjects.push_back(rayGenDxilLibSubobject);
	exports[SEN_RayGen].pSubObject = &subObjects.back();

	// Hit Group shader stuff
	// ----------------------------------------------------------------//
	exports[SEN_Hit].exportName = L"Hit";
	D3D12_STATE_SUBOBJECT hitGroupLibSubobject = CreateDxilLibrary(exports[SEN_Hit].exportName, g_phitShaderLib,
		sizeof(g_phitShaderLib), exports[SEN_Hit].dxilLibDesc, exports[SEN_Hit].exportDesc);

	subObjects.push_back(hitGroupLibSubobject);
	exports[SEN_Hit].pSubObject = &subObjects.back();

	// Miss shader stuff
	// ----------------------------------------------------------------//
	exports[SEN_Miss].exportName = L"Miss";
	D3D12_STATE_SUBOBJECT missLibSubobject = CreateDxilLibrary(exports[SEN_Miss].exportName, g_pmissShaderLib,
		sizeof(g_pmissShaderLib), exports[SEN_Miss].dxilLibDesc, exports[SEN_Miss].exportDesc);

	subObjects.push_back(missLibSubobject);
	exports[SEN_Miss].pSubObject = &subObjects.back();
}

void RTModelViewer::InitHitGroups(std::vector<D3D12_STATE_SUBOBJECT>& subObjects, D3D12_RAYTRACING_SHADER_CONFIG& shaderConfig,
	D3D12_HIT_GROUP_DESC& hitGroupDesc, LPCWSTR hitGroupNames[HG_NumGroups], ShaderExport exports[SEN_NumExports])
{
	D3D12_STATE_SUBOBJECT shaderConfigStateObject;
	shaderConfig.MaxAttributeSizeInBytes = 8;
	shaderConfig.MaxPayloadSizeInBytes = 8;
	shaderConfigStateObject.pDesc = &shaderConfig;
	shaderConfigStateObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	subObjects.push_back(shaderConfigStateObject);

	LPCWSTR hitGroupExportName = L"HitGroup";
	hitGroupDesc.ClosestHitShaderImport = exports[SEN_Hit].exportName;
	hitGroupDesc.HitGroupExport = hitGroupExportName;
	D3D12_STATE_SUBOBJECT hitGroupSubobject = {};
	hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	hitGroupSubobject.pDesc = &hitGroupDesc;

	subObjects.push_back(hitGroupSubobject);
	hitGroupNames[HG_HitGroup] = hitGroupExportName;

	D3D12_STATE_SUBOBJECT localRootSignatureSubObject;
	localRootSignatureSubObject.pDesc = &g_LocalRaytracingRootSignature.p;
	localRootSignatureSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	subObjects.push_back(localRootSignatureSubObject);
}

void RTModelViewer::InitRayTraceInputs(std::function<void(ID3D12StateObject*, byte*)> GetShaderTable, D3D12_STATE_OBJECT_DESC& stateObject,
	std::vector<byte>& pHitShaderTable, UINT shaderRecordSizeInBytes, ShaderExport exports[SEN_NumExports])
{
	{
		CComPtr<ID3D12StateObject> pbarycentricPSO;
		g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS(&pbarycentricPSO));
		GetShaderTable(pbarycentricPSO, pHitShaderTable.data());
		g_RaytracingInputs[Primarybarycentric] = RaytracingDispatchRayInputs(*g_pRaytracingDevice, pbarycentricPSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), exports[SEN_RayGen].exportName, exports[SEN_Miss].exportName);
	}

	{
		*exports[SEN_RayGen].pSubObject = CreateDxilLibrary(exports[SEN_RayGen].exportName, g_pRayGenerationShaderSSRLib, sizeof(g_pRayGenerationShaderSSRLib), exports[SEN_RayGen].dxilLibDesc, exports[SEN_RayGen].exportDesc);
		CComPtr<ID3D12StateObject> pReflectionbarycentricPSO;
		g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS(&pReflectionbarycentricPSO));
		GetShaderTable(pReflectionbarycentricPSO, pHitShaderTable.data());
		g_RaytracingInputs[Reflectionbarycentric] = RaytracingDispatchRayInputs(*g_pRaytracingDevice, pReflectionbarycentricPSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), exports[SEN_RayGen].exportName, exports[SEN_Miss].exportName);
	}

	{
		*exports[SEN_RayGen].pSubObject = CreateDxilLibrary(exports[SEN_RayGen].exportName, g_pRayGenerationShadowsLib, sizeof(g_pRayGenerationShadowsLib), exports[SEN_RayGen].dxilLibDesc, exports[SEN_RayGen].exportDesc);
		*exports[SEN_Miss].pSubObject = CreateDxilLibrary(exports[SEN_Miss].exportName, g_pmissShadowsLib, sizeof(g_pmissShadowsLib), exports[SEN_Miss].dxilLibDesc, exports[SEN_Miss].exportDesc);

		CComPtr<ID3D12StateObject> pShadowsPSO;
		g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS(&pShadowsPSO));
		GetShaderTable(pShadowsPSO, pHitShaderTable.data());
		g_RaytracingInputs[Shadows] = RaytracingDispatchRayInputs(*g_pRaytracingDevice, pShadowsPSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), exports[SEN_RayGen].exportName, exports[SEN_Miss].exportName);
	}

	{
		*exports[SEN_RayGen].pSubObject = CreateDxilLibrary(exports[SEN_RayGen].exportName, g_pRayGenerationShaderLib, sizeof(g_pRayGenerationShaderLib), exports[SEN_RayGen].dxilLibDesc, exports[SEN_RayGen].exportDesc);
		*exports[SEN_Hit].pSubObject = CreateDxilLibrary(exports[SEN_Hit].exportName, g_pDiffuseHitShaderLib, sizeof(g_pDiffuseHitShaderLib), exports[SEN_Hit].dxilLibDesc, exports[SEN_Hit].exportDesc);
		*exports[SEN_Miss].pSubObject = CreateDxilLibrary(exports[SEN_Miss].exportName, g_pmissShaderLib, sizeof(g_pmissShaderLib), exports[SEN_Miss].dxilLibDesc, exports[SEN_Miss].exportDesc);

		CComPtr<ID3D12StateObject> pDiffusePSO;
		g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS(&pDiffusePSO));
		GetShaderTable(pDiffusePSO, pHitShaderTable.data());
		g_RaytracingInputs[DiffuseHitShader] = RaytracingDispatchRayInputs(*g_pRaytracingDevice, pDiffusePSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), exports[SEN_RayGen].exportName, exports[SEN_Miss].exportName);
	}

	{
		*exports[SEN_RayGen].pSubObject = CreateDxilLibrary(exports[SEN_RayGen].exportName, g_pRayGenerationShaderSSRLib, sizeof(g_pRayGenerationShaderSSRLib), exports[SEN_RayGen].dxilLibDesc, exports[SEN_RayGen].exportDesc);
		*exports[SEN_Hit].pSubObject = CreateDxilLibrary(exports[SEN_Hit].exportName, g_pDiffuseHitShaderLib, sizeof(g_pDiffuseHitShaderLib), exports[SEN_Hit].dxilLibDesc, exports[SEN_Hit].exportDesc);
		*exports[SEN_Miss].pSubObject = CreateDxilLibrary(exports[SEN_Miss].exportName, g_pmissShaderLib, sizeof(g_pmissShaderLib), exports[SEN_Miss].dxilLibDesc, exports[SEN_Miss].exportDesc);

		CComPtr<ID3D12StateObject> pReflectionPSO;
		g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS(&pReflectionPSO));
		GetShaderTable(pReflectionPSO, pHitShaderTable.data());
		g_RaytracingInputs[Reflection] = RaytracingDispatchRayInputs(*g_pRaytracingDevice, pReflectionPSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), exports[SEN_RayGen].exportName, exports[SEN_Miss].exportName);
	}
}

#pragma once
#include "modules/gx2/gx2.h"
#include "modules/gx2/gx2_draw.h"
#include "modules/gx2/gx2_context.h"
#include "modules/gx2/gx2_renderstate.h"
#include "modules/gx2/gx2_texture.h"
#include "modules/gx2/gx2_shaders.h"
#include "hostlookup.h"
#include "dx12.h"
#include "dx12_heap.h"
#include "dx12_dynbuffer.h"

struct DXScanBufferData;
struct DXColorBufferData;
struct DXDepthBufferData;
struct DXTextureData;
class DXPipelineMgr;

struct DXState {
   DXState()
   {

   }

   static const UINT FrameCount = 2;

   // DX Basics
   ComPtr<IDXGISwapChain3> swapChain;
   ComPtr<ID3D12Device> device;
   ComPtr<ID3D12Resource> renderTargets[FrameCount];
   ComPtr<ID3D12CommandAllocator> commandAllocator[FrameCount];
   ComPtr<ID3D12CommandQueue> commandQueue;
   ComPtr<ID3D12RootSignature> rootSignature;
   DXHeap *srvHeap;
   DXHeap *rtvHeap;
   DXHeap *dsvHeap;
   ComPtr<ID3D12PipelineState> emuPipelineState;
   ComPtr<ID3D12GraphicsCommandList> commandList;
   DXHeapItemPtr scanbufferRtv[FrameCount];
   DXHeapItemPtr curScanbufferRtv;
   D3D12_VIEWPORT viewport;
   D3D12_RECT scissorRect;
   ComPtr<ID3D12Resource> vertexBuffer;
   D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
   DXDynBuffer *ppcVertexBuffer;
   DXPipelineMgr *pipelineMgr;

   // DX Synchronization objects.
   UINT frameIndex;
   HANDLE fenceEvent;
   ComPtr<ID3D12Fence> fence;
   UINT swapCount;

   // Emulator Objects
   DXScanBufferData *tvScanBuffer;
   DXScanBufferData *drcScanBuffer;

   GX2ColorBuffer *activeColorBuffer[GX2_NUM_MRT_BUFFER];
   GX2DepthBuffer *activeDepthBuffer;

   struct ContextState
   {
      GX2ColorBuffer *colorBuffer[GX2_NUM_MRT_BUFFER];
      GX2DepthBuffer *depthBuffer;
      struct {
         uint32_t size;
         uint32_t stride;
         void *buffer;
      } attribBuffers[32];
      GX2FetchShader *fetchShader;
      GX2VertexShader *vertexShader;
      GX2PixelShader *pixelShader;
      GX2GeometryShader *geomShader;
      GX2PixelSampler *pixelSampler[GX2_NUM_TEXTURE_UNIT];
      struct {
         GX2LogicOp::Op logicOp;
         uint8_t blendEnabled;
         float constColor[4];
         bool alphaTestEnabled;
         GX2CompareFunction::Func alphaFunc;
         float alphaRef;
      } blendState;
      struct {
         GX2BlendMode::Mode colorSrcBlend;
         GX2BlendMode::Mode colorDstBlend;
         GX2BlendCombineMode::Mode colorCombine;
         GX2BlendMode::Mode alphaSrcBlend;
         GX2BlendMode::Mode alphaDstBlend;
         GX2BlendCombineMode::Mode alphaCombine;
      } targetBlendState[8];
      float uniforms[256 * 4];
   } state;
   static_assert(sizeof(ContextState) < sizeof(GX2ContextState::stateStore), "ContextState must be smaller than GX2ContextState::stateStore");

   GX2ContextState *activeContextState;

};

extern DXState gDX;

namespace dx {

   void initialise();
   void renderScanBuffers();
   void updateRenderTargets();
   void updatePipeline();
   void updateBuffers();
   void updateTextures();

   DXColorBufferData * getColorBuffer(GX2ColorBuffer *buffer);
   DXDepthBufferData * getDepthBuffer(GX2DepthBuffer *buffer);
   DXTextureData * getTexture(GX2Texture *buffer);

   void _beginFrame();
   void _endFrame();

}
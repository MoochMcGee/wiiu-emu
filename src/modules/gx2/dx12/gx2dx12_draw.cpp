#include "../gx2.h"
#ifdef GX2_DX12

#include "../gx2_draw.h"
#include "dx12_state.h"
#include "dx12_fetchshader.h"
#include "dx12_colorbuffer.h"
#include "dx12_depthbuffer.h"

void
GX2SetClearDepthStencil(GX2DepthBuffer *depthBuffer,
   float depth,
   uint8_t stencil)
{
   // TODO: GX2SetClearDepthStencil
}

void
GX2ClearBuffersEx(GX2ColorBuffer *colorBuffer,
   GX2DepthBuffer *depthBuffer,
   float red, float green, float blue, float alpha,
   float depth,
   uint8_t unk1,
   GX2ClearFlags::Flags flags)
{
   // TODO: GX2ClearBuffersEx depth/stencil clearing

   auto hostColorBuffer = dx::getColorBuffer(colorBuffer);

   const float clearColor[] = { red, green, blue, alpha };
   gDX.commandList->ClearRenderTargetView(*hostColorBuffer->rtv, clearColor, 0, nullptr);
}

void
GX2SetAttribBuffer(
   uint32_t index,
   uint32_t size,
   uint32_t stride,
   void *buffer)
{
   auto& attribData = gDX.state.attribBuffers[index];
   attribData.size = size;
   attribData.stride = stride;
   attribData.buffer = buffer;
}

void
GX2DrawEx(
   GX2PrimitiveMode::Mode mode,
   uint32_t numVertices,
   uint32_t offset,
   uint32_t numInstances)
{
   // TODO: GX2DrawEx

   dx::updateRenderTargets();
   dx::updatePipeline();
   dx::updateBuffers();

   switch (mode) {
   case GX2PrimitiveMode::Triangles:
      gDX.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      break;
   case GX2PrimitiveMode::TriangleStrip:
      gDX.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
      break;
   default:
      assert(0);
   }
   
   gDX.commandList->DrawInstanced(numVertices, numInstances, offset, 0);

}

#endif

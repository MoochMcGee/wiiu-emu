#include "gx2.h"
#include "gx2_context.h"

void
GX2Init(be_val<uint32_t> *attributes)
{
   // TODO: GX2Init set current thread as the graphics thread
   auto log = gLog->debug();
   log << "GX2Init attributes: ";

   while (attributes && *attributes) {
      uint32_t attrib = *attributes;
      log << " " << attrib;
      ++attributes;
   }
}

void
GX2Shutdown()
{
   // TODO: GX2Shutdown
}

void
GX2Flush()
{
   // TODO: GX2Flush
}

void
GX2Invalidate(GX2InvalidateMode::Mode mode,
              void *buffer,
              uint32_t size)
{
   // TODO: GX2Invalidate
}

void
GX2SetupContextState(GX2ContextState *state)
{
   GX2SetupContextStateEx(state, TRUE);
}

void
GX2SetupContextStateEx(GX2ContextState *state,
                       BOOL unk1)
{
   state->displayListSize = 0x300;
   GX2BeginDisplayListEx(reinterpret_cast<GX2DisplayList*>(&state->displayList), state->displayListSize, unk1);
}

void
GX2GetContextStateDisplayList(GX2ContextState *state,
                              be_val<uint32_t> *outDisplayList,
                              be_val<uint32_t> *outSize)
{
   *outDisplayList = gMemory.untranslate(&state->displayList);
   *outSize = state->displayListSize;
}

void
GX2SetContextState(GX2ContextState *state)
{
   // TODO: GX2SetContextState
}

void GX2::registerContextFunctions()
{
   RegisterKernelFunction(GX2Init);
   RegisterKernelFunction(GX2Shutdown);
   RegisterKernelFunction(GX2Flush);
   RegisterKernelFunction(GX2Invalidate);
   RegisterKernelFunction(GX2SetupContextState);
   RegisterKernelFunction(GX2SetupContextStateEx);
   RegisterKernelFunction(GX2GetContextStateDisplayList);
   RegisterKernelFunction(GX2SetContextState);
}
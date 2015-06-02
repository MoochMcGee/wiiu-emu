#include "coreinit.h"
#include "coreinit_mutex.h"

void
OSInitMutex(p32<OSMutex> pMutex)
{
   auto mutex = p32_direct(pMutex);
   mutex->tag = OSMutex::Tag;
   mutex->name = nullptr;
   mutex->unk1 = 0;
   mutex->unk2 = 0;
   mutex->unk3 = 0;
   OSInitThreadQueueEx(make_p32(&mutex->queue), make_p32<void>(mutex));
}

void
OSInitMutexEx(p32<OSMutex> pMutex, p32<char> pName)
{
   auto mutex = p32_direct(pMutex);
   OSInitMutex(pMutex);
   mutex->name = pName;
}

void
CoreInit::registerMutexFunctions()
{
   RegisterSystemFunction(OSInitMutex);
}
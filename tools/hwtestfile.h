#pragma once
#include "be_val.h"
#include "testfile.h"

struct HardwareRegisterState
{
   be_val<uint32_t> xer;
   be_val<uint32_t> cr;
   be_val<uint32_t> fpscr;
   be_val<uint32_t> ctr;
   be_val<uint32_t> r3;
   be_val<uint32_t> r4;
   be_val<uint32_t> r5;
   be_val<uint32_t> r6;
   be_val<double> fr1;
   be_val<double> fr2;
   be_val<double> fr3;
   be_val<double> fr4;
};

struct HardwareTestData
{
   Instruction instr;
   RegisterState input;
   HardwareRegisterState output;

   template <class Archive>
   void serialize(Archive & ar)
   {
      ar(instr.value);
      ar(input);
      ar(output);
   }
};

struct HardwareTestFile
{
   std::vector<HardwareTestData> test;

   template <class Archive>
   void serialize(Archive & ar)
   {
      ar(instr.value);
      ar(input);
      ar(output);
   }
};

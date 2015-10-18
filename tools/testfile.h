#pragma once
#include <vector>
#include <cereal/types/vector.hpp>
#include <cereal/archives/binary.hpp>
#include "be_val.h"
#include "cpu/state.h"
#include "cpu/instructiondata.h"

struct RegisterState
{
   xer_t xer;
   cr_t cr;
   fpscr_t fpscr;
   uint32_t ctr;
   uint32_t gpr[4];
   double fr[4];

   template <class Archive>
   void serialize(Archive & ar)
   {
      ar(xer.value);
      ar(cr.value);
      ar(fpscr.value);
      ar(ctr);

      for (auto i = 0; i < 4; ++i) {
         ar(gpr[i]);
      }

      for (auto i = 0; i < 4; ++i) {
         ar(fr[i]);
      }
   }
};

struct TestData
{
   TestData()
   {
      memset(this, 0, sizeof(TestData));
   }

   Instruction instr;
   RegisterState input;
   RegisterState output;

   template <class Archive>
   void serialize(Archive & ar)
   {
      ar(instr.value);
      ar(input);
      ar(output);
   }
};

struct TestFile
{
   std::string name;
   std::vector<TestData> tests;

   template <class Archive>
   void serialize(Archive & ar)
   {
      ar(tests);
   }
};

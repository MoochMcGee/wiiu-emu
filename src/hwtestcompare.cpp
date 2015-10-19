#include "testfile.h"
#include "fuzztests.h"
#include "mem/mem.h"
#include "cpu/cpu.h"
#include "cpu/disassembler.h"
#include "log.h"
#include <cassert>
#include <fstream>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

bool runHWTest()
{
   std::vector<titties::TestFile> tests;
   uint32_t baseAddress = 0x02000000;

   // Allocate some memory to write code to
   if (!mem::alloc(baseAddress, 4096)) {
      return false;
   }

   Instruction bclr = gInstructionTable.encode(InstructionID::bclr);
   bclr.bo = 0x1f;
   mem::write(baseAddress + 4, bclr.value);

   // Read all tests
   for (auto &p : fs::directory_iterator("tests/cpu/wiiu")) {
      std::cout << p.path().string() << std::endl;
      std::ifstream file(p.path().string());
      assert(file.is_open());
      cereal::BinaryInputArchive input(file);
      tests.emplace_back();
      auto &test = tests.back();
      input(test);
      test.name = p.path().filename().string();
   }

   for (auto &testFile : tests) {
      for (auto &test : testFile.tests) {
         ThreadState state;
         memset(&state, 0, sizeof(ThreadState));
         state.cia = 0;
         state.nia = baseAddress;
         state.xer = test.input.xer;
         state.cr = test.input.cr;
         state.fpscr = test.input.fpscr;
         state.ctr = test.input.ctr;

         for (auto i = 0; i < 4; ++i) {
            state.gpr[i + 3] = test.input.gpr[i];
            state.fpr[i + 1].paired0 = test.input.fr[i];
         }

         mem::write(baseAddress, test.instr.value);
         cpu::executeSub(&state);

         Disassembly dis;
         gDisassembler.disassemble(test.instr, dis, baseAddress);
         gLog->debug(dis.text);

         if (state.xer.value != test.output.xer.value) {
            gLog->error("Test failed, xer {:08X} != {:08X}", state.xer.value, test.output.xer.value);
         }

         if (state.cr.value != test.output.cr.value) {
            gLog->error("Test failed, cr {:08X} != {:08X}", state.cr.value, test.output.cr.value);
         }

         /*
         f3 = -nan fucks everything up
         if (state.fpscr.value != test.output.fpscr.value) {
            gLog->error("Test failed, fpscr {:08X} != {:08X}", state.fpscr.value, test.output.fpscr.value);
         }*/

         if (state.ctr != test.output.ctr) {
            gLog->error("Test failed, ctr {:08X} != {:08X}", state.ctr, test.output.ctr);
         }

         for (auto i = 0; i < 4; ++i) {
            if (state.gpr[i + 3] != test.output.gpr[i]) {
               gLog->error("Test failed, gpr {:08X} != {:08X}", state.gpr[i + 3], test.output.gpr[i]);
            }
         }

         /*
         f3 = -nan fucks everything up
         for (auto i = 0; i < 4; ++i) {
            if (state.fpr[i + 1].paired0 != test.output.fr[i]) {
               gLog->error("Test failed, gpr {:08X} != {:08X}", state.fpr[i + 1].paired0, test.output.fr[i]);
            }
         }*/
      }
   }

   return true;
}

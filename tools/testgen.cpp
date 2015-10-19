#define NOMINMAX
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <spdlog/spdlog.h>
#include "be_val.h"
#include "bitutils.h"
#include "cpu/state.h"
#include "cpu/instructiondata.h"
#include "testfile.h"

using namespace titties;

static const auto GPR_BASE = 3;
static const auto FPR_BASE = 1;
static const auto CRF_BASE = 2;
static const auto CRB_BASE = 8;

std::vector<InstructionID> gIntegerArithmetic =
{
   InstructionID::add,
   InstructionID::addc,
   InstructionID::adde,
   InstructionID::addi,
   InstructionID::addic,
   InstructionID::addicx,
   InstructionID::addis,
   InstructionID::addme,
   InstructionID::addze,
   InstructionID::divw,
   InstructionID::divwu,
   InstructionID::mulhw,
   InstructionID::mulhwu,
   InstructionID::mulli,
   InstructionID::mullw,
   InstructionID::neg,
   InstructionID::subf,
   InstructionID::subfc,
   InstructionID::subfe,
   InstructionID::subfic,
   InstructionID::subfme,
   InstructionID::subfze,
};

std::vector<InstructionID> gIntegerLogical =
{
   InstructionID::and_,
   InstructionID::andc,
   InstructionID::andi,
   InstructionID::andis,
   InstructionID::cntlzw,
   InstructionID::eqv,
   InstructionID::extsb,
   InstructionID::extsh,
   InstructionID::nand,
   InstructionID::nor,
   InstructionID::or_,
   InstructionID::orc,
   InstructionID::ori,
   InstructionID::oris,
   InstructionID::xor_,
   InstructionID::xori,
   InstructionID::xoris,
};

std::vector<InstructionID> gIntegerCompare =
{
   InstructionID::cmp,
   InstructionID::cmpi,
   InstructionID::cmpl,
   InstructionID::cmpli,
};

std::vector<InstructionID> gIntegerShift =
{
   InstructionID::slw,
   InstructionID::sraw,
   InstructionID::srawi,
   InstructionID::srw,
};

std::vector<InstructionID> gIntegerRotate =
{
   InstructionID::rlwimi,
   InstructionID::rlwinm,
   InstructionID::rlwnm,
};

std::vector<InstructionID> gConditionRegisterLogical =
{
   InstructionID::crand,
   InstructionID::crandc,
   InstructionID::creqv,
   InstructionID::crnand,
   InstructionID::crnor,
   InstructionID::cror,
   InstructionID::crorc,
   InstructionID::crxor,
   //InstructionID::mcrf,
};

std::vector<InstructionID> gFloatArithmetic =
{
   InstructionID::fadd,
   InstructionID::fadds,
   InstructionID::fdiv,
   InstructionID::fdivs,
   InstructionID::fmul,
   InstructionID::fmuls,
   InstructionID::fres,
   InstructionID::fsub,
   InstructionID::fsubs,
   InstructionID::fsel,
};

std::vector<InstructionID> gFloatArithmeticMuladd =
{
   InstructionID::fmadd,
   InstructionID::fmadds,
   InstructionID::fmsub,
   InstructionID::fmsubs,
   InstructionID::fnmadd,
   InstructionID::fnmadds,
   InstructionID::fnmsub,
   InstructionID::fnmsubs,
};

std::vector<InstructionID> gFloatRound =
{
   InstructionID::fctiw,
   InstructionID::fctiwz,
   InstructionID::frsp,
};

std::vector<InstructionID> gFloatMove =
{
   InstructionID::fabs,
   InstructionID::fmr,
   InstructionID::fnabs,
   InstructionID::fneg,
};

std::vector<InstructionID> gFloatCompare =
{
   InstructionID::fcmpo,
   InstructionID::fcmpu,
};

auto gTestInstructions =
{
   gIntegerArithmetic,
   gIntegerLogical,
   gIntegerCompare,
   gIntegerShift,
   gIntegerRotate,
   gConditionRegisterLogical,
   gFloatArithmetic,
   gFloatArithmeticMuladd,
   gFloatRound,
   gFloatMove,
};

std::vector<uint32_t> gValuesCRB =
{
   0,
   1,
};

std::vector<uint32_t> gValuesGPR =
{
   0,
   1,
   static_cast<uint32_t>(-1),
   static_cast<uint32_t>(std::numeric_limits<int32_t>::min()),
   static_cast<uint32_t>(std::numeric_limits<int32_t>::max()),
};

std::vector<int16_t> gValuesSIMM =
{
   0,
   1,
   -1,
   std::numeric_limits<int16_t>::min(),
   std::numeric_limits<int16_t>::max(),
};

std::vector<uint16_t> gValuesUIMM =
{
   0,
   1,
   static_cast<uint16_t>(-1),
   static_cast<uint16_t>(std::numeric_limits<int16_t>::min()),
   static_cast<uint16_t>(std::numeric_limits<int16_t>::max()),
};

std::vector<double> gValuesFPR =
{
   0.0,
   1.0,
   -1.0,
   std::numeric_limits<double>::min(),
   std::numeric_limits<double>::max(),
   std::numeric_limits<double>::lowest(),
   std::numeric_limits<double>::infinity(),
   std::numeric_limits<double>::quiet_NaN(),
   std::numeric_limits<double>::signaling_NaN(),
   std::numeric_limits<double>::denorm_min(),
   std::numeric_limits<double>::epsilon(),
};

std::vector<uint32_t> gValuesXERC =
{
   0,
   1
};

std::vector<uint32_t> gValuesXERSO =
{
   0,
   1
};

std::vector<uint32_t> gValuesSH =
{
   0, 15, 23, 31
};

std::vector<uint32_t> gValuesMB =
{
   0, 15, 23, 31
};

std::vector<uint32_t> gValuesME =
{
   0, 15, 23, 31
};

void
setCRB(RegisterState &state, uint32_t bit, uint32_t value)
{
   state.cr.value = set_bit_value(state.cr.value, 31 - bit, value);
}

void genTests(InstructionData *data)
{
   std::vector<size_t> indexCur, indexMax;
   std::vector<bool> flagSet;
   auto complete = false;
   auto completeIndices = false;
   TestFile testFile;

   for (auto i = 0; i < data->read.size(); ++i) {
      auto &field = data->read[i];
      indexCur.push_back(0);

      switch (field) {
      case Field::rA:
      case Field::rB:
      case Field::rS:
         indexMax.push_back(gValuesGPR.size());
         break;
      case Field::frA:
      case Field::frB:
      case Field::frC:
      case Field::frS:
         indexMax.push_back(gValuesFPR.size());
         break;
      case Field::crbA:
      case Field::crbB:
         indexMax.push_back(gValuesCRB.size());
         break;
      case Field::simm:
         indexMax.push_back(gValuesSIMM.size());
         break;
      case Field::sh:
         indexMax.push_back(gValuesSH.size());
         break;
      case Field::mb:
         indexMax.push_back(gValuesMB.size());
         break;
      case Field::me:
         indexMax.push_back(gValuesME.size());
         break;
      case Field::uimm:
         indexMax.push_back(gValuesUIMM.size());
         break;
      case Field::XERC:
         indexMax.push_back(gValuesXERC.size());
         break;
      case Field::XERSO:
         indexMax.push_back(gValuesXERSO.size());
         break;
      default:
         assert(false);
      }
   }

   for (auto i = 0; i < data->flags.size(); ++i) {
      flagSet.push_back(false);
   }

   while (!complete) {
      TestData test;
      uint32_t gpr = 0;
      uint32_t fpr = 0;
      uint32_t crf = 0;
      uint32_t crb = 0;

      test.instr = gInstructionTable.encode(data->id);

      for (auto i = 0; i < data->read.size(); ++i) {
         auto index = indexCur[i];

         switch (data->read[i]) {
         case Field::rA:
            test.instr.rA = gpr + GPR_BASE;
            test.input.gpr[gpr++] = gValuesGPR[index];
            break;
         case Field::rB:
            test.instr.rB = gpr + GPR_BASE;
            test.input.gpr[gpr++] = gValuesGPR[index];
            break;
         case Field::rS:
            test.instr.rS = gpr + GPR_BASE;
            test.input.gpr[gpr++] = gValuesGPR[index];
            break;
         case Field::frA:
            test.instr.frA = fpr + FPR_BASE;
            test.input.fr[fpr++] = gValuesFPR[index];
            break;
         case Field::frB:
            test.instr.frB = fpr + FPR_BASE;
            test.input.fr[fpr++] = gValuesFPR[index];
            break;
         case Field::frC:
            test.instr.frC = fpr + FPR_BASE;
            test.input.fr[fpr++] = gValuesFPR[index];
            break;
         case Field::frS:
            test.instr.frS = fpr + FPR_BASE;
            test.input.fr[fpr++] = gValuesFPR[index];
            break;
         case Field::crbA:
            test.instr.crbA = (crb++) + CRB_BASE;
            setCRB(test.input, test.instr.crbA, gValuesCRB[index]);
            break;
         case Field::crbB:
            test.instr.crbB = (crb++) + CRB_BASE;
            setCRB(test.input, test.instr.crbB, gValuesCRB[index]);
            break;
         case Field::simm:
            test.instr.simm = gValuesSIMM[index];
            break;
         case Field::sh:
            test.instr.sh = gValuesSH[index];
            break;
         case Field::mb:
            test.instr.mb = gValuesMB[index];
            break;
         case Field::me:
            test.instr.me = gValuesME[index];
            break;
         case Field::uimm:
            test.instr.uimm = gValuesUIMM[index];
            break;
         case Field::XERC:
            test.input.xer.ca = gValuesXERC[index];
            break;
         case Field::XERSO:
            test.input.xer.so = gValuesXERSO[index];
            break;
         default:
            assert(false);
         }
      }

      for (auto i = 0; i < data->write.size(); ++i) {
         switch (data->write[i]) {
         case Field::rA:
            test.instr.rA = gpr + GPR_BASE;
            gpr++;
            break;
         case Field::rD:
            test.instr.rD = gpr + GPR_BASE;
            gpr++;
            break;
         case Field::frD:
            test.instr.frD = gpr + GPR_BASE;
            fpr++;
            break;
         case Field::crfD:
            test.instr.crfD = crf + CRF_BASE;
            crf++;
            break;
         case Field::crbD:
            test.instr.crbD = crb + CRB_BASE;
            crb++;
            break;
         case Field::XERC:
         case Field::XERSO:
         case Field::FCRISI:
         case Field::FCRZDZ:
         case Field::FCRIDI:
         case Field::FCRSNAN:
            break;
         default:
            assert(false);
         }
      }

      // Execute state/instr
      testFile.tests.emplace_back(test);

      // Increase indices
      for (auto i = 0; i < indexCur.size(); ++i) {
         indexCur[i]++;

         if (indexCur[i] < indexMax[i]) {
            break;
         } else if (indexCur[i] == indexMax[i]) {
            indexCur[i] = 0;

            if (i == indexCur.size() - 1) {
               completeIndices = true;
            }
         }
      }

      if (completeIndices) {
         if (flagSet.size() == 0) {
            complete = true;
            break;
         }

         completeIndices = false;

         // Do next flag!
         for (auto i = 0; i < flagSet.size(); ++i) {
            if (!flagSet[i]) {
               flagSet[i] = true;
               break;
            } else {
               flagSet[i] = false;

               if (i == flagSet.size() - 1) {
                  complete = true;
               }
            }
         }
      }
   }

   std::ofstream out { std::string("tests/cpu/input/") + data->name, std::ofstream::out };
   cereal::BinaryOutputArchive archive(out);
   archive(testFile);
}

int main(int argc, char **argv)
{
   gInstructionTable.initialise();

   for (auto &group : gTestInstructions) {
      for (auto id : group) {
         auto data = gInstructionTable.find(id);
         genTests(data);
      }
   }

   return 0;
}

/*
// Floating-Point Status and Control Register
INS(mcrfs, (crfD), (crfS), (), (opcd == 63, xo1 == 64, !_9_10, !_14_15, !_16_20, !_31), "")
INS(mffs, (frD), (), (rc), (opcd == 63, xo1 == 583, !_11_15, !_16_20), "")
INS(mtfsf, (), (fm, frB), (rc), (opcd == 63, xo1 == 711, !_6, !_15), "")
INS(mtfsfi, (crfD), (), (rc, imm), (opcd == 63, xo1 == 134, !_9_10, !_11_15, !_20), "")

// Integer Load
INS(lbz, (rD), (rA, d), (), (opcd == 34), "Load Byte and Zero")
INS(lbzu, (rD, rA), (rA, d), (), (opcd == 35), "Load Byte and Zero with Update")
INS(lbzx, (rD), (rA, rB), (), (opcd == 31, xo1 == 87, !_31), "Load Byte and Zero Indexed")
INS(lbzux, (rD, rA), (rA, rB), (), (opcd == 31, xo1 == 119, !_31), "Load Byte and Zero with Update Indexed")
INS(lha, (rD), (rA, d), (), (opcd == 42), "Load Half Word Algebraic")
INS(lhau, (rD, rA), (rA, d), (), (opcd == 43), "Load Half Word Algebraic with Update")
INS(lhax, (rD), (rA, rB), (), (opcd == 31, xo1 == 343, !_31), "Load Half Word Algebraic Indexed")
INS(lhaux, (rD, rA), (rA, rB), (), (opcd == 31, xo1 == 375, !_31), "Load Half Word Algebraic with Update Indexed")
INS(lhz, (rD), (rA, d), (), (opcd == 40), "Load Half Word and Zero")
INS(lhzu, (rD, rA), (rA, d), (), (opcd == 41), "Load Half Word and Zero with Update")
INS(lhzx, (rD), (rA, rB), (), (opcd == 31, xo1 == 279, !_31), "Load Half Word and Zero Indexed")
INS(lhzux, (rD, rA), (rA, rB), (), (opcd == 31, xo1 == 311, !_31), "Load Half Word and Zero with Update Indexed")
INS(lwz, (rD), (rA, d), (), (opcd == 32), "Load Word and Zero")
INS(lwzu, (rD, rA), (rA, d), (), (opcd == 33), "Load Word and Zero with Update")
INS(lwzx, (rD), (rA, rB), (), (opcd == 31, xo1 == 23, !_31), "Load Word and Zero Indexed")
INS(lwzux, (rD, rA), (rA, rB), (), (opcd == 31, xo1 == 55, !_31), "Load Word and Zero with Update Indexed")

// Integer Store
INS(stb, (), (rS, rA, d), (), (opcd == 38), "Store Byte")
INS(stbu, (rA), (rS, rA, d), (), (opcd == 39), "Store Byte with Update")
INS(stbx, (), (rS, rA, rB), (), (opcd == 31, xo1 == 215, !_31), "Store Byte Indexed")
INS(stbux, (rA), (rS, rA, rB), (), (opcd == 31, xo1 == 247, !_31), "Store Byte with Update Indexed")
INS(sth, (), (rS, rA, d), (), (opcd == 44), "Store Half Word")
INS(sthu, (rA), (rS, rA, d), (), (opcd == 45), "Store Half Word with Update")
INS(sthx, (), (rS, rA, rB), (), (opcd == 31, xo1 == 407, !_31), "Store Half Word Indexed")
INS(sthux, (rA), (rS, rA, rB), (), (opcd == 31, xo1 == 439, !_31), "Store Half Word with Update Indexed")
INS(stw, (), (rS, rA, d), (), (opcd == 36), "Store Word")
INS(stwu, (rA), (rS, rA, d), (), (opcd == 37), "Store Word with Update")
INS(stwx, (), (rS, rA, rB), (), (opcd == 31, xo1 == 151, !_31), "Store Word Indexed")
INS(stwux, (rA), (rS, rA, rB), (), (opcd == 31, xo1 == 183, !_31), "Store Word with Update Indexed")

// Integer Load and Store with Byte Reverse
INS(lhbrx, (rD), (rA, rB), (), (opcd == 31, xo1 == 790, !_31), "Load Half Word Byte-Reverse Indexed")
INS(lwbrx, (rD), (rA, rB), (), (opcd == 31, xo1 == 534, !_31), "Load Word Byte-Reverse Indexed")
INS(sthbrx, (), (rS, rA, rB), (), (opcd == 31, xo1 == 918, !_31), "Store Half Word Byte-Reverse Indexed")
INS(stwbrx, (), (rS, rA, rB), (), (opcd == 31, xo1 == 662, !_31), "Store Word Byte-Reverse Indexed")

// Integer Load and Store Multiple
INS(lmw, (rD), (rA, d), (), (opcd == 46), "Load Multiple Words")
INS(stmw, (), (rS, rA, d), (), (opcd == 47), "Store Multiple Words")

// Integer Load and Store String
INS(lswi, (rD), (rA, nb), (), (opcd == 31, xo1 == 597, !_31), "Load String Word Immediate")
INS(lswx, (rD), (rA, rB), (), (opcd == 31, xo1 == 533, !_31), "Load String Word Indexed")
INS(stswi, (), (rS, rA, nb), (), (opcd == 31, xo1 == 725, !_31), "Store String Word Immediate")
INS(stswx, (), (rS, rA, rB), (), (opcd == 31, xo1 == 661, !_31), "Store String Word Indexed")

// Memory Synchronisation
INS(eieio, (), (), (), (opcd == 31, xo1 == 854, !_6_10, !_11_15, !_16_20, !_31), "Enforce In-Order Execution of I/O")
INS(isync, (), (), (), (opcd == 19, xo1 == 150, !_6_10, !_11_15, !_16_20, !_31), "Instruction Synchronise")
INS(lwarx, (rD, RSRV), (rA, rB), (), (opcd == 31, xo1 == 20, !_31), "Load Word and Reserve Indexed")
INS(stwcx, (RSRV), (rS, rA, rB), (), (opcd == 31, xo1 == 150, _31 == 1), "Store Word Conditional Indexed")
INS(sync, (), (), (), (opcd == 31, xo1 == 598, !_6_10, !_11_15, !_16_20, !_31), "Synchronise")

// Floating-Point Load
INS(lfd, (frD), (rA, d), (), (opcd == 50), "Load Floating-Point Double")
INS(lfdu, (frD, rA), (rA, d), (), (opcd == 51), "Load Floating-Point Double with Update")
INS(lfdx, (frD), (rA, rB), (), (opcd == 31, xo1 == 599, !_31), "Load Floating-Point Double Indexed")
INS(lfdux, (frD, rA), (rA, rB), (), (opcd == 31, xo1 == 631, !_31), "Load Floating-Point Double with Update Indexed")
INS(lfs, (frD), (rA, d), (), (opcd == 48), "Load Floating-Point Single")
INS(lfsu, (frD, rA), (rA, d), (), (opcd == 49), "Load Floating-Point Single with Update")
INS(lfsx, (frD), (rA, rB), (), (opcd == 31, xo1 == 535, !_31), "Load Floating-Point Single Indexed")
INS(lfsux, (frD, rA), (rA, rB), (), (opcd == 31, xo1 == 567, !_31), "Load Floating-Point Single with Update Indexed")

// Floating-Point Store
INS(stfd, (), (frS, rA, d), (), (opcd == 54), "Store Floating-Point Double")
INS(stfdu, (rA), (frS, rA, d), (), (opcd == 55), "Store Floating-Point Double with Update")
INS(stfdx, (), (frS, rA, rB), (), (opcd == 31, xo1 == 727, !_31), "Store Floating-Point Double Indexed")
INS(stfdux, (rA), (frS, rA, rB), (), (opcd == 31, xo1 == 759, !_31), "Store Floating-Point Double with Update Indexed")
INS(stfiwx, (), (frS, rA, rB), (), (opcd == 31, xo1 == 983, !_31), "Store Floating-Point as Integer Word Indexed")
INS(stfs, (), (frS, rA, d), (), (opcd == 52), "Store Floating-Point Single")
INS(stfsu, (rA), (frS, rA, d), (), (opcd == 53), "Store Floating-Point Single with Update")
INS(stfsx, (), (frS, rA, rB), (), (opcd == 31, xo1 == 663, !_31), "Store Floating-Point Single Indexed")
INS(stfsux, (rA), (frS, rA, rB), (), (opcd == 31, xo1 == 695, !_31), "Store Floating-Point Single with Update Indexed")

// Branch
INS(b, (), (li), (aa, lk), (opcd == 18), "Branch")
INS(bc, (bo), (bi, bd), (aa, lk), (opcd == 16), "Branch Conditional")
INS(bcctr, (bo), (bi, CTR), (lk), (opcd == 19, xo1 == 528, !_16_20), "Branch Conditional to CTR")
INS(bclr, (bo), (bi, LR), (lk), (opcd == 19, xo1 == 16, !_16_20), "Branch Conditional to LR")

// System Linkage
INS(rfi, (), (), (), (opcd == 19, xo1 == 50, !_6_10, !_11_15, !_16_20, !_31), "")
INS(sc, (), (), (), (opcd == 17, !_6_10, !_11_15, !_16_29, _30 == 1, !_31), "Syscall")
INS(kc, (), (kcn), (), (opcd == 1), "krncall")

// Trap
INS(tw, (), (to, rA, rB), (), (opcd == 31, xo1 == 4, !_31), "")
INS(twi, (), (to, rA, simm), (), (opcd == 3), "")

// Processor Control
INS(mcrxr, (crfD), (XERO), (), (opcd == 31, xo1 == 512, !_9_10, !_11_15, !_16_20, !_31), "Move to Condition Register from XERO")
INS(mfcr, (rD), (), (), (opcd == 31, xo1 == 19, !_11_15, !_16_20, !_31), "Move from Condition Register")
INS(mfmsr, (rD), (), (), (opcd == 31, xo1 == 83, !_11_15, !_16_20, !_31), "Move from Machine State Register")
INS(mfspr, (rD), (spr), (), (opcd == 31, xo1 == 339, !_31), "Move from Special Purpose Register")
INS(mftb, (rD), (tbr), (), (opcd == 31, xo1 == 371, !_31), "Move from Time Base Register")
INS(mtcrf, (crm), (rS), (), (opcd == 31, xo1 == 144, !_11, !_20, !_31), "Move to Condition Register Fields")
INS(mtmsr, (), (rS), (), (opcd == 31, xo1 == 146, !_11_15, !_16_20, !_31), "Move to Machine State Register")
INS(mtspr, (spr), (rS), (), (opcd == 31, xo1 == 467, !_31), "Move to Special Purpose Register")

// Cache Management
INS(dcbf, (), (rA, rB), (), (opcd == 31, xo1 == 86, !_6_10, !_31), "")
INS(dcbi, (), (rA, rB), (), (opcd == 31, xo1 == 470, !_6_10, !_31), "")
INS(dcbst, (), (rA, rB), (), (opcd == 31, xo1 == 54, !_6_10, !_31), "")
INS(dcbt, (), (rA, rB), (), (opcd == 31, xo1 == 278, !_6_10, !_31), "")
INS(dcbtst, (), (rA, rB), (), (opcd == 31, xo1 == 246, !_6_10, !_31), "")
INS(dcbz, (), (rA, rB), (), (opcd == 31, xo1 == 1014, !_6_10, !_31), "")
INS(icbi, (), (rA, rB), (), (opcd == 31, xo1 == 982, !_6_10, !_31), "")
INS(dcbz_l, (), (rA, rB), (), (opcd == 4, xo1 == 1014, !_6_10, !_31), "")

// Segment Register Manipulation
INS(mfsr, (rD), (sr), (), (opcd == 31, xo1 == 595, !_11, !_16_20, !_31), "Move from Segment Register")
INS(mfsrin, (rD), (rB), (), (opcd == 31, xo1 == 659, !_11_15, !_31), "Move from Segment Register Indirect")
INS(mtsr, (), (rD, sr), (), (opcd == 31, xo1 == 210, !_11, !_16_20, !_31), "Move to Segment Register")
INS(mtsrin, (), (rD, rB), (), (opcd == 31, xo1 == 242, !_11_15, !_31), "Move to Segment Register Indirect")

// Lookaside Buffer Management
INS(tlbie, (), (rB), (), (opcd == 31, xo1 == 306, !_6_10, !_11_15, !_31), "")
INS(tlbsync, (), (), (), (opcd == 31, xo1 == 566, !_6_10, !_11_15, !_16_20, !_31), "")

// External Control
INS(eciwx, (rD), (rA, rB), (), (opcd == 31, xo1 == 310, !_31), "")
INS(ecowx, (rD), (rA, rB), (), (opcd == 31, xo1 == 438, !_31), "")

// Paired-Single Load and Store
INS(psq_l, (frD), (rA, qd), (w, i), (opcd == 56), "Paired Single Load")
INS(psq_lu, (frD), (rA, qd), (w, i), (opcd == 57), "Paired Single Load with Update")
INS(psq_lx, (frD), (rA, rB), (qw, qi), (opcd == 4, xo3 == 6, !_31), "Paired Single Load Indexed")
INS(psq_lux, (frD), (rA, rB), (qw, qi), (opcd == 4, xo3 == 38, !_31), "Paired Single Load with Update Indexed")
INS(psq_st, (frD), (rA, qd), (w, i), (opcd == 60), "Paired Single Store")
INS(psq_stu, (frD), (rA, qd), (w, i), (opcd == 61), "Paired Single Store with Update")
INS(psq_stx, (frS), (rA, rB), (qw, qi), (opcd == 4, xo3 == 7, !_31), "Paired Single Store Indexed")
INS(psq_stux, (frS), (rA, rB), (qw, qi), (opcd == 4, xo3 == 39, !_31), "Paired Single Store with Update Indexed")

// Paired-Single Floating Point Arithmetic
INS(ps_add, (frD, FPSCR), (frA, frB), (rc), (opcd == 4, xo4 == 21, !_21_25), "Paired Single Add")
INS(ps_div, (frD, FPSCR), (frA, frB), (rc), (opcd == 4, xo4 == 18, !_21_25), "Paired Single Divide")
INS(ps_mul, (frD, FPSCR), (frA, frC), (rc), (opcd == 4, xo4 == 25, !_16_20), "Paired Single Multiply")
INS(ps_sub, (frD, FPSCR), (frA, frB), (rc), (opcd == 4, xo4 == 20, !_21_25), "Paired Single Subtract")
INS(ps_abs, (frD), (frB), (rc), (opcd == 4, xo1 == 264, !_11_15), "Paired Single Absolute")
INS(ps_nabs, (frD), (frB), (rc), (opcd == 4, xo1 == 136, !_11_15), "Paired Single Negate Absolute")
INS(ps_neg, (frD), (frB), (rc), (opcd == 4, xo1 == 40, !_11_15), "Paired Single Negate")
INS(ps_sel, (frD), (frA, frB, frC), (rc), (opcd == 4, xo4 == 23), "Paired Single Select")
INS(ps_res, (frD, FPSCR), (frB), (rc), (opcd == 4, xo4 == 24, !_11_15, !_21_25), "Paired Single Reciprocal")
INS(ps_rsqrte, (frD, FPSCR), (frB), (rc), (opcd == 4, xo4 == 26, !_11_15, !_21_25), "Paired Single Reciprocal Square Root Estimate")
INS(ps_msub, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 28), "Paired Single Multiply and Subtract")
INS(ps_madd, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 29), "Paired Single Multiply and Add")
INS(ps_nmsub, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 30), "Paired Single Negate Multiply and Subtract")
INS(ps_nmadd, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 31), "Paired Single Negate Multiply and Add")
INS(ps_mr, (frD), (frB), (rc), (opcd == 4, xo1 == 72, !_11_15), "Paired Single Move Register")
INS(ps_sum0, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 10), "Paired Single Sum High")
INS(ps_sum1, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 11), "Paired Single Sum Low")
INS(ps_muls0, (frD, FPSCR), (frA, frC), (rc), (opcd == 4, xo4 == 12, !_16_20), "Paired Single Multiply Scalar High")
INS(ps_muls1, (frD, FPSCR), (frA, frC), (rc), (opcd == 4, xo4 == 13, !_16_20), "Paired Single Multiply Scalar Low")
INS(ps_madds0, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 14), "Paired Single Multiply and Add Scalar High")
INS(ps_madds1, (frD, FPSCR), (frA, frB, frC), (rc), (opcd == 4, xo4 == 15), "Paired Single Multiply and Add Scalar Low")
INS(ps_cmpu0, (crfD, FPSCR), (frA, frB), (), (opcd == 4, xo1 == 0, !_9_10, !_31), "Paired Single Compare Unordered High")
INS(ps_cmpo0, (crfD, FPSCR), (frA, frB), (), (opcd == 4, xo1 == 32, !_9_10, !_31), "Paired Single Compare Ordered High")
INS(ps_cmpu1, (crfD, FPSCR), (frA, frB), (), (opcd == 4, xo1 == 64, !_9_10, !_31), "Paired Single Compare Unordered Low")
INS(ps_cmpo1, (crfD, FPSCR), (frA, frB), (), (opcd == 4, xo1 == 96, !_9_10, !_31), "Paired Single Compare Ordered Low")
INS(ps_merge00, (frD), (frA, frB), (rc), (opcd == 4, xo1 == 528), "Paired Single Merge High")
INS(ps_merge01, (frD), (frA, frB), (rc), (opcd == 4, xo1 == 560), "Paired Single Merge Direct")
INS(ps_merge10, (frD), (frA, frB), (rc), (opcd == 4, xo1 == 592), "Paired Single Merge Swapped")
INS(ps_merge11, (frD), (frA, frB), (rc), (opcd == 4, xo1 == 624), "Paired Single Merge Low")
*/

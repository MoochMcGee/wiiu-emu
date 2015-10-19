#include <cassert>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <ovsocket/socket.h>
#include <ovsocket/networkthread.h>
#include <experimental/filesystem>
#include "be_val.h"
#include "testfile.h"
#include "cpu/instructiondata.h"

using namespace titties;
namespace fs = std::experimental::filesystem;

std::vector<TestFile> gTestSet;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "ovsocket.lib")

using namespace ovs;
using namespace std::placeholders;

struct HWRegisterState
{
   be_val<uint32_t> xer;
   be_val<uint32_t> cr;
   be_val<uint32_t> fpscr;
   be_val<uint32_t> ctr;
   be_val<uint32_t> r3;
   be_val<uint32_t> r4;
   be_val<uint32_t> r5;
   be_val<uint32_t> r6;
   be_val<double> f1;
   be_val<double> f2;
   be_val<double> f3;
   be_val<double> f4;
};

struct PacketHeader
{
   enum Commands
   {
      Version = 1,
      ExecuteGeneralTest = 10,
      ExecutePairedTest = 20,
   };

   be_val<uint16_t> size;
   be_val<uint16_t> command;
};

struct VersionPacket : PacketHeader
{
   VersionPacket(uint32_t value)
   {
      size = sizeof(VersionPacket);
      command = PacketHeader::Version;
      version = value;
   }

   be_val<uint32_t> version;
};

struct ExecuteGeneralTestPacket : PacketHeader
{
   ExecuteGeneralTestPacket()
   {
      size = sizeof(ExecuteGeneralTestPacket);
      command = PacketHeader::ExecuteGeneralTest;
      memset(&state, 0, sizeof(HWRegisterState));
   }

   be_val<uint32_t> instr;
   HWRegisterState state;
};

class TestServer
{
   static const uint32_t Version = 1;

public:
   TestServer(Socket *socket) :
      mSocket(socket)
   {
      mSocket->addErrorListener(std::bind(&TestServer::onSocketError, this, _1, _2));
      mSocket->addDisconnectListener(std::bind(&TestServer::onSocketDisconnect, this, _1));
      mSocket->addReceiveListener(std::bind(&TestServer::onSocketReceive, this, _1, _2, _3));

      // Send version
      VersionPacket version { Version };
      mSocket->send(reinterpret_cast<const char *>(&version), sizeof(VersionPacket));

      // Read first packet
      mSocket->recvFill(sizeof(PacketHeader));

      // Initialise test iterators
      mTestFile = gTestSet.begin();
      mTestData = mTestFile->tests.begin();
   }

   std::vector<TestFile>::iterator mTestFile;
   std::vector<TestData>::iterator mTestData;

private:
   void sendNextTest()
   {
      if (mTestData == mTestFile->tests.end()) {
         std::ofstream out("tests/cpu/wiiu/" + mTestFile->name);
         cereal::BinaryOutputArchive ar(out);
         ar(*mTestFile);
         ++mTestFile;

         if (mTestFile == gTestSet.end()) {
            mSocket->disconnect();
            return;
         } else {
            mTestData = mTestFile->tests.begin();
         }
      }

      TestData &test = *mTestData;
      ExecuteGeneralTestPacket packet;
      std::cout << "Send test for " << test.instr.value << std::endl;
      packet.instr = test.instr.value;
      packet.state.xer = test.input.xer.value;
      packet.state.cr = test.input.cr.value;
      packet.state.fpscr = test.input.fpscr.value;
      packet.state.ctr = test.input.ctr;
      packet.state.r3 = test.input.gpr[0];
      packet.state.r4 = test.input.gpr[1];
      packet.state.r5 = test.input.gpr[2];
      packet.state.r6 = test.input.gpr[3];
      packet.state.f1 = test.input.fr[0];
      packet.state.f2 = test.input.fr[1];
      packet.state.f3 = test.input.fr[2];
      packet.state.f4 = test.input.fr[3];

      mSocket->send(reinterpret_cast<const char *>(&packet), sizeof(ExecuteGeneralTestPacket));
   }

   void handleTestResult(ExecuteGeneralTestPacket *result)
   {
      TestData &test = *mTestData;
      assert(test.instr.value == result->instr.value());

      std::cout << "Received test result for instruction " << result->instr << std::endl;

      test.output.xer.value = result->state.xer;
      test.output.cr.value = result->state.cr;
      test.output.fpscr.value = result->state.fpscr;
      test.output.ctr = result->state.ctr;
      test.output.gpr[0] = result->state.r3;
      test.output.gpr[1] = result->state.r4;
      test.output.gpr[2] = result->state.r5;
      test.output.gpr[3] = result->state.r6;
      test.output.fr[0] = result->state.f1;
      test.output.fr[1] = result->state.f2;
      test.output.fr[2] = result->state.f3;
      test.output.fr[3] = result->state.f4;

      mTestData++;
      sendNextTest();
   }

   void onReceivePacket(PacketHeader *packet)
   {
      switch (packet->command) {
      case PacketHeader::Version:
      {
         auto version = reinterpret_cast<VersionPacket *>(packet);
         std::cout << "Server Version: " << Version << ", Client Version: " << version->version << std::endl;
         sendNextTest();
         break;
      }
      case PacketHeader::ExecuteGeneralTest:
      {
         auto result = reinterpret_cast<ExecuteGeneralTestPacket *>(packet);
         handleTestResult(result);
         break;
      }
      case PacketHeader::ExecutePairedTest:
         // result of paired single test
         break;
      }
   }

   void onSocketError(Socket *socket, int code)
   {
      assert(mSocket == socket);
      std::cout << "Socket error: " << code << std::endl;
   }

   void onSocketDisconnect(Socket *socket)
   {
      assert(mSocket == socket);
      std::cout << "Socket Disconnected" << std::endl;
   }

   void onSocketReceive(Socket *socket, const char *buffer, size_t size)
   {
      PacketHeader *header;
      assert(mSocket == socket);

      if (mCurrentPacket.size() == 0) {
         assert(size == sizeof(PacketHeader));

         // Copy packet to buffer
         mCurrentPacket.resize(size);
         header = reinterpret_cast<PacketHeader *>(mCurrentPacket.data());
         std::memcpy(header, buffer, size);

         // Read rest of packet
         auto read = header->size - size;
         socket->recvFill(read);
      } else {
         // Check we have read rest of packet
         header = reinterpret_cast<PacketHeader *>(mCurrentPacket.data());
         auto totalSize = size + sizeof(PacketHeader);
         assert(totalSize == header->size);

         // Read rest of packet
         mCurrentPacket.resize(totalSize);
         header = reinterpret_cast<PacketHeader *>(mCurrentPacket.data());
         std::memcpy(mCurrentPacket.data() + sizeof(PacketHeader), buffer, size);

         onReceivePacket(header);

         // Read next packet
         mCurrentPacket.clear();
         socket->recvFill(sizeof(PacketHeader));
      }
   }

private:
   std::vector<char> mCurrentPacket;
   Socket *mSocket;
};

std::vector<std::unique_ptr<TestServer>> gTestServers;

void addTestServer(Socket *socket)
{
   gTestServers.emplace_back(new TestServer(socket));
}

void loadTests()
{
   fs::create_directories("tests/cpu/wiiu");

   // Read all tests
   for (auto &p : fs::directory_iterator("tests/cpu/input")) {
      std::cout << p.path().string() << std::endl;
      std::ifstream file(p.path().string());
      assert(file.is_open());
      cereal::BinaryInputArchive input(file);
      gTestSet.emplace_back();
      auto &test = gTestSet.back();
      input(test);
      test.name = p.path().filename().string();
   }
}

int main(int argc, char **argv)
{
   WSADATA wsaData;
   WSAStartup(MAKEWORD(2, 2), &wsaData);

   loadTests();
   gInstructionTable.initialise();

   NetworkThread thread;
   auto socket = new Socket();
   auto ip = "0.0.0.0";
   auto port = "8008";

   // On socket error
   socket->addErrorListener([](Socket *socket, int code) {
      std::cout << "Listen Socket Error: " << code << std::endl;
   });

   socket->addDisconnectListener([](Socket *socket) {
      std::cout << "Listen Socket Disconnected" << std::endl;
   });

   // On socket connected, accept pls
   socket->addAcceptListener([&thread](Socket *socket) {
      auto newSock = socket->accept();

      if (!newSock) {
         std::cout << "Failed to accept new connection" << std::endl;
         return;
      } else {
         std::cout << "New Connection Accepted" << std::endl;
      }

      addTestServer(newSock);
      thread.addSocket(newSock);
   });

   if (!socket->listen(ip, port)) {
      std::cout << "Error starting connect!" << std::endl;
      return 0;
   }

   std::cout << "Listening on " << ip << ":" << port << std::endl;
   thread.addSocket(socket);
   thread.start();
   WSACleanup();
   return 0;
}

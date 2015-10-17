#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include <ovsocket/socket.h>
#include <ovsocket/networkthread.h>
#include "be_val.h"
#include "instructiondata.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "ovsocket.lib")

using namespace ovs;
using namespace std::placeholders;

struct TestState
{
   be_val<uint32_t> xer;
   be_val<uint32_t> cr;
   be_val<uint32_t> fpscr;
   be_val<uint32_t> __pad;
   be_val<uint32_t> r3;
   be_val<uint32_t> r4;
   be_val<uint32_t> r5;
   be_val<uint32_t> r6;
   be_val<double> fr1;
   be_val<double> fr2;
   be_val<double> fr3;
   be_val<double> fr4;
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
      memset(&state, 0, sizeof(TestState));
   }

   be_val<uint32_t> instr;
   TestState state;
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
   }

private:
   void sendNextTest()
   {
      ExecuteGeneralTestPacket test;
      test.instr = 0x38600539;

      // r3 = r4 + 337, r4 = 1000
      auto addi = gInstructionTable.encode(InstructionID::addi);
      addi.rA = 4;
      addi.rD = 3;
      addi.simm = 337;
      test.state.r4 = 1000;
      mSocket->send(reinterpret_cast<const char *>(&test), sizeof(ExecuteGeneralTestPacket));
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
         std::cout << "Received test result for instruction " << result->instr << std::endl;
         std::cout << "xer = " << result->state.xer << std::endl;
         std::cout << "cr = " << result->state.cr << std::endl;
         std::cout << "fpscr = " << result->state.fpscr << std::endl;
         std::cout << "r3 = " << result->state.r3 << std::endl;
         std::cout << "r4 = " << result->state.r4 << std::endl;
         std::cout << "r5 = " << result->state.r5 << std::endl;
         std::cout << "r6 = " << result->state.r6 << std::endl;
         std::cout << "fr1 = " << result->state.fr1 << std::endl;
         std::cout << "fr2 = " << result->state.fr2 << std::endl;
         std::cout << "fr3 = " << result->state.fr3 << std::endl;
         std::cout << "fr4 = " << result->state.fr4 << std::endl;
         // TODO: sendNextTest
         mSocket->disconnect();
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
      std::cout << "Receive size " << size << ": " << buffer << std::endl;

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

int main(int argc, char **argv)
{
   WSADATA wsaData;
   WSAStartup(MAKEWORD(2, 2), &wsaData);
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

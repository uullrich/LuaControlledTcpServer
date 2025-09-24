#include <can/CanMessage.h>
#include <lua/LuaBinding.h>
#include <tcp/TcpServer.h>

#include <asio.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char *argv[]) {
  std::cout << "Starting Lua-controlled TCP Server..." << std::endl;

  try {
    asio::io_context ioContext;
    LuaBinding luaBinding(ioContext);

    std::string scriptPath = "scripts/server.lua";
    if (argc > 1) {
      scriptPath = argv[1];
    }

    if (!luaBinding.loadScript(scriptPath)) {
      std::cerr << "Failed to load Lua script: " << scriptPath << std::endl;
      return 1;
    }

    if (!luaBinding.executeScript()) {
      std::cerr << "Failed to execute Lua script" << std::endl;
      return 1;
    }

    std::thread ioThread([&ioContext]() {
      std::cout << "Starting IO context..." << std::endl;
      ioContext.run();
      std::cout << "IO context stopped" << std::endl;
    });

    std::cout << "Server running. Press Enter to stop..." << std::endl;
    std::cin.get();

    std::cout << "Shutting down..." << std::endl;
    ioContext.stop();

    if (ioThread.joinable()) {
      ioThread.join();
    }

    std::cout << "Server stopped successfully" << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
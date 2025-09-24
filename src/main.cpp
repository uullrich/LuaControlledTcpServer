#include <can/CanMessage.h>
#include <lua/LuaBinding.h>
#include <tcp/TcpServer.h>

#include <asio.hpp>
#include <spdlog/spdlog.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char *argv[]) {
  spdlog::info("Starting Lua-controlled TCP Server...");

  try {
    asio::io_context ioContext;
    LuaBinding luaBinding(ioContext);

    std::string scriptPath = "scripts/server.lua";
    if (argc > 1) {
      scriptPath = argv[1];
    }

    if (!luaBinding.loadScript(scriptPath)) {
      spdlog::error("Failed to load Lua script: {}", scriptPath);
      return 1;
    }

    if (!luaBinding.executeScript()) {
      spdlog::error("Failed to execute Lua script");
      return 1;
    }

    std::thread ioThread([&ioContext]() {
      spdlog::info("Starting IO context...");
      ioContext.run();
      spdlog::info("IO context stopped");
    });

    spdlog::info("Server running. Press Enter to stop...");
    std::cin.get();

    spdlog::info("Shutting down...");
    ioContext.stop();

    if (ioThread.joinable()) {
      ioThread.join();
    }

    spdlog::info("Server stopped successfully");

  } catch (const std::exception &exception) {
    spdlog::error("Error: {}", exception.what());
    return 1;
  }

  return 0;
}
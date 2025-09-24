#include "LuaBinding.h"

#include <spdlog/spdlog.h>

#include <chrono>

LuaBinding::LuaBinding(asio::io_context &ioContext)
    : m_ioContext(ioContext),
      m_workGuard(std::make_unique<
                  asio::executor_work_guard<asio::io_context::executor_type>>(
          ioContext.get_executor())),
      m_queueProcessing(true) {

  // Initialize Lua
  m_lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine,
                       sol::lib::string, sol::lib::math, sol::lib::table,
                       sol::lib::io, sol::lib::os);

  registerFunctions();
}

LuaBinding::~LuaBinding() {
  stopServer();
  m_queueProcessing = false;
  m_queueCV.notify_all();
  m_workGuard.reset();
}

bool LuaBinding::loadScript(const std::string &filename) {
  try {
    // Load and execute the script file to register functions
    auto result = m_lua.script_file(filename);

    if (!result.valid()) {
      sol::error err = result;
      spdlog::error("Error loading script: {}", err.what());
      return false;
    }

    spdlog::info("Script loaded successfully: {}", filename);
    return true;
  } catch (const sol::error &error) {
    spdlog::error("Error loading script: {}", error.what());
    return false;
  }
}

bool LuaBinding::executeScript() {
  try {
    // Check if main function exists
    sol::optional<sol::function> mainFunc = m_lua["main"];
    if (!mainFunc) {
      spdlog::error("main() function not found in loaded script");
      return false;
    }

    // Call main function
    auto result = mainFunc.value()();

    if (!result.valid()) {
      sol::error error = result;
      spdlog::error("Error executing main(): {}", error.what());
      return false;
    }

    spdlog::info("Script executed successfully");
    return true;
  } catch (const sol::error &e) {
    spdlog::error("Error executing script: {}", e.what());
    return false;
  }
}

void LuaBinding::registerFunctions() {
  // TCP Server functions
  m_lua.set_function("startServer", &LuaBinding::startServer, this);
  m_lua.set_function("stopServer", &LuaBinding::stopServer, this);
  m_lua.set_function("createCANMessage", &LuaBinding::createCanMessage, this);
  m_lua.set_function("sendCANMessage", &LuaBinding::sendCanMessage, this);
  m_lua.set_function("broadcastCANMessage", &LuaBinding::broadcastCanMessage,
                     this);
  m_lua.set_function("getConnectedClients", &LuaBinding::getConnectedClients,
                     this);

  // Logging
  m_lua.set_function("log", &LuaBinding::log, this);
  m_lua.set_function("logError", &LuaBinding::logError, this);

  // Waiting
  m_lua.set_function("wait", &LuaBinding::wait, this);

  // Register event callbacks that will be called from Lua
  m_lua["onClientConnected"] = sol::lua_nil;
  m_lua["onClientDisconnected"] = sol::lua_nil;
  m_lua["onMessageReceived"] = sol::lua_nil;
}

void LuaBinding::startServer(uint16_t port) {
  if (m_server) {
    spdlog::error("Server already running");
    return;
  }

  try {
    m_server = std::make_unique<TcpServer>(m_ioContext, port);

    m_server->setConnectCallback(
        std::bind(&LuaBinding::onClientConnected, this, std::placeholders::_1));

    m_server->setDisconnectCallback(std::bind(&LuaBinding::onClientDisconnected,
                                              this, std::placeholders::_1));

    m_server->setMessageCallback(std::bind(&LuaBinding::onMessageReceived, this,
                                           std::placeholders::_1,
                                           std::placeholders::_2));

    m_server->start();
    spdlog::info("Server started on port {}", port);
  } catch (const asio::system_error &error) {
    spdlog::error("Failed to start server: {}", error.what());
  }
}

void LuaBinding::stopServer() {
  if (!m_server)
    return;

  m_server->stop();
  m_server.reset();
  spdlog::info("Server stopped");
}

std::string LuaBinding::createCanMessage(uint32_t id, const sol::table &data,
                                         bool extended, bool rtr) {
  std::vector<uint8_t> bytes;
  bytes.reserve(data.size());

  // Convert table to byte vector
  for (int i = 1; i <= data.size(); ++i) {
    sol::object value = data[i];
    if (value.is<int>()) {
      auto byte = static_cast<uint8_t>(value.as<int>() & 0xFF);
      bytes.push_back(byte);
    }
  }

  // Create CAN message
  CanMessage message(id, bytes, extended, rtr);

  // Generate a unique ID for this message
  std::string messageId = "msg_" + std::to_string(id) + "_" +
                          std::to_string(reinterpret_cast<uintptr_t>(&message));

  // Store the message
  {
    std::scoped_lock lock(m_messagesMutex);
    m_canMessages[messageId] = message;
  }

  return messageId;
}

bool LuaBinding::sendCanMessage(const std::string &clientId,
                                const std::string &messageId) {
  if (!m_server) {
    spdlog::error("Server not running");
    return false;
  }

  CanMessage message;
  {
    std::scoped_lock lock(m_messagesMutex);
    auto it = m_canMessages.find(messageId);
    if (it == m_canMessages.end()) {
      spdlog::error("Message not found: {}", messageId);
      return false;
    }
    message = it->second;
  }

  bool success = m_server->sendMessage(clientId, message);
  if (success) {
    spdlog::info("Sent message to client {}: {}", clientId, message.toString());
  } else {
    spdlog::error("Failed to send message to client {}", clientId);
  }

  return success;
}

bool LuaBinding::broadcastCanMessage(const std::string &messageId) {
  if (!m_server) {
    spdlog::error("Server not running");
    return false;
  }

  CanMessage message;
  {
    std::scoped_lock lock(m_messagesMutex);
    auto it = m_canMessages.find(messageId);
    if (it == m_canMessages.end()) {
      spdlog::error("Message not found: {}", messageId);
      return false;
    }
    message = it->second;
  }

  m_server->broadcastMessage(message);
  spdlog::info("Broadcast message: {}", message.toString());

  return true;
}

sol::table LuaBinding::getConnectedClients() {
  sol::table result = m_lua.create_table();

  if (m_server) {
    auto clients = m_server->getConnectedClients();
    for (size_t i = 0; i < clients.size(); ++i) {
      result[i + 1] = clients[i];
    }
  }

  return result;
}

void LuaBinding::log(const std::string &message) const {
  spdlog::info("[LUA] - {}", message);
}

void LuaBinding::logError(const std::string &message) const {
  spdlog::error("[LUA] - {}", message);
}

void LuaBinding::wait(int milliseconds) {
  // For non-blocking wait, we use a boost timer
  asio::steady_timer timer(m_ioContext,
                           std::chrono::milliseconds(milliseconds));
  timer.wait();
}

void LuaBinding::onClientConnected(const std::string &clientId) {
  // Add to queue for Lua to process
  {
    std::scoped_lock lock(m_queueMutex);
    m_connectedClients.push(clientId);
  }

  // Notify about new event
  m_queueCV.notify_one();

  // Check if Lua has a callback for this event
  sol::protected_function callback = m_lua["onClientConnected"];
  if (callback.valid()) {
    try {
      sol::protected_function_result result = callback(clientId);
      if (!result.valid()) {
        sol::error error = result;
        spdlog::error("Error in onClientConnected callback: {}", error.what());
      }
    } catch (const sol::error &error) {
      spdlog::error("Exception in onClientConnected callback: {}",
                    error.what());
    }
  }
}

void LuaBinding::onClientDisconnected(const std::string &clientId) {
  // Add to queue for Lua to process
  {
    std::scoped_lock lock(m_queueMutex);
    m_disconnectedClients.push(clientId);
  }

  // Notify about new event
  m_queueCV.notify_one();

  // Check if Lua has a callback for this event
  sol::protected_function callback = m_lua["onClientDisconnected"];
  if (callback.valid()) {
    try {
      sol::protected_function_result result = callback(clientId);
      if (!result.valid()) {
        sol::error error = result;
        spdlog::error("Error in onClientDisconnected callback: {}",
                      error.what());
      }
    } catch (const sol::error &e) {
      spdlog::error("Exception in onClientDisconnected callback: {}", e.what());
    }
  }
}

void LuaBinding::onMessageReceived(const std::string &clientId,
                                   const CanMessage &message) {
  // Add to queue for Lua to process
  {
    std::scoped_lock lock(m_queueMutex);
    m_receivedMessages.push({clientId, message});
  }

  // Notify about new event
  m_queueCV.notify_one();

  // Check if Lua has a callback for this event
  sol::protected_function callback = m_lua["onMessageReceived"];
  if (callback.valid()) {
    try {
      // Convert CAN message data to Lua table
      sol::table dataTable = m_lua.create_table();
      const auto &data = message.getData();
      for (size_t i = 0; i < data.size(); ++i) {
        dataTable[i + 1] = static_cast<int>(data[i]);
      }

      sol::protected_function_result result =
          callback(clientId, message.getID(), dataTable, message.isExtended(),
                   message.isRTR());

      if (!result.valid()) {
        sol::error error = result;
        spdlog::error("Error in onMessageReceived callback: {}", error.what());
      }
    } catch (const sol::error &error) {
      spdlog::error("Exception in onMessageReceived callback: ", error.what());
    }
  }
}
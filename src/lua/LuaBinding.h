#pragma once

#include <tcp/TcpServer.h>
#include <can/CanMessage.h>

#include <sol/sol.hpp>
#include <asio.hpp>

#include <string>
#include <memory>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>

class LuaBinding {
public:
    explicit LuaBinding(asio::io_context& ioContext);
    ~LuaBinding();

    // Initialize and load a Lua script
    bool loadScript(const std::string& filename);
    
    // Execute a Lua script
    bool executeScript();
    
    // Register C++ functions with Lua
    void registerFunctions();

private:
    // TCP server management
    void startServer(uint16_t port);
    void stopServer();
    std::string createCanMessage(uint32_t id, const sol::table& data, bool extended, bool rtr);
    bool sendCanMessage(const std::string& clientId, const std::string& messageId);
    bool broadcastCanMessage(const std::string& messageId);
    sol::table getConnectedClients();
    
    // Logging
    void log(const std::string& message);
    void logError(const std::string& message);
    
    // Waiting
    void wait(int milliseconds);
    
    // Message callbacks
    void onClientConnected(const std::string& clientId);
    void onClientDisconnected(const std::string& clientId);
    void onMessageReceived(const std::string& clientId, const CanMessage& message);

    // IO context and work guard to keep IO running
    asio::io_context& m_ioContext;
    std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> m_workGuard;
    
    // Lua state
    sol::state m_lua;
    
    // TCP Server
    std::unique_ptr<TcpServer> m_server;
    
    // Message storage
    std::unordered_map<std::string, CanMessage> m_canMessages;
    std::mutex m_messagesMutex;
    
    // Message queues for async handling
    struct ReceivedMessage {
        std::string clientId;
        CanMessage message;
    };
    std::queue<ReceivedMessage> m_receivedMessages;
    std::queue<std::string> m_connectedClients;
    std::queue<std::string> m_disconnectedClients;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;
    bool m_queueProcessing;
};
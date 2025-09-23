#pragma once

#include <can/CanMessage.h>

#include <asio.hpp>

#include <functional>
#include <vector>
#include <string>

class ITcpServer {
public:
    using tcp = asio::ip::tcp;
    using SessionId = std::string;
    using MessageCallback = std::function<void(const SessionId&, const CanMessage&)>;
    using ConnectCallback = std::function<void(const SessionId&)>;
    using DisconnectCallback = std::function<void(const SessionId&)>;

    virtual ~ITcpServer() = default;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual bool sendMessage(const std::string& clientId, const CanMessage& message) = 0;
    virtual void broadcastMessage(const CanMessage& message) = 0;
    virtual std::vector<std::string> getConnectedClients() const = 0;

    virtual void setMessageCallback(MessageCallback callback) = 0;
    virtual void setConnectCallback(ConnectCallback callback) = 0;
    virtual void setDisconnectCallback(DisconnectCallback callback) = 0;
};
#pragma once

#include <tcp/ITcpServer.h>
#include <can/CanMessage.h>

#include <asio.hpp>

#include <memory>
#include <unordered_map>
#include <string>
#include <atomic>

class TcpServer : public ITcpServer {
public:
    explicit TcpServer(asio::io_context& ioContext, uint16_t port);
    ~TcpServer() override;

    void start() override;
    void stop() override;

    bool sendMessage(const SessionId& sessionId, const CanMessage& message) override;
    void broadcastMessage(const CanMessage& message) override;
    std::vector<SessionId> getConnectedClients() const override;
    
    void setMessageCallback(MessageCallback callback) override;
    void setConnectCallback(ConnectCallback callback) override;
    void setDisconnectCallback(DisconnectCallback callback) override;

private:
    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(tcp::socket socket, TcpServer& server, SessionId id);
        void start();
        void stop();
        bool send(const CanMessage& message);
        SessionId getId() const;

    private:
        void doRead();
        void processMessage();
        
        tcp::socket m_socket;
        TcpServer& m_server;
        SessionId m_id;
        std::vector<uint8_t> m_readBuffer;
        std::size_t m_bytesNeeded;
        std::vector<uint8_t> m_messageBuffer;
    };

    void doAccept();
    void removeSession(const SessionId& id);

    asio::io_context& m_ioContext;
    tcp::acceptor m_acceptor;
    std::unordered_map<SessionId, std::shared_ptr<Session>> m_sessions;
    std::atomic<bool> m_running;
    uint64_t m_nextId;
    
    MessageCallback m_messageCallback;
    ConnectCallback m_connectCallback;
    DisconnectCallback m_disconnectCallback;
};
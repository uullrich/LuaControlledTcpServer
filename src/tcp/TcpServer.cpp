#include "TcpServer.h"

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/streambuf.hpp>

#include <iostream>

TcpServer::TcpServer(asio::io_context &ioContext, uint16_t port)
    : m_ioContext(ioContext),
      m_acceptor(ioContext, tcp::endpoint(tcp::v4(), port)), m_running(false),
      m_nextId(1) {}

TcpServer::~TcpServer() { TcpServer::stop(); }

void TcpServer::start() {
  m_running = true;
  doAccept();
  std::cout << "TCP Server started on port "
            << m_acceptor.local_endpoint().port() << std::endl;
}

void TcpServer::stop() {
  if (!m_running)
    return;

  m_running = false;

  std::error_code ec;
  m_acceptor.close(ec);

  for (const auto &[id, session] : m_sessions) {
    session->stop();
  }
  m_sessions.clear();

  std::cout << "TCP Server stopped" << std::endl;
}

bool TcpServer::sendMessage(const std::string &sessionId,
                            const CanMessage &message) {
  auto it = m_sessions.find(sessionId);
  if (it == m_sessions.end()) {
    return false;
  }

  return it->second->send(message);
}

void TcpServer::broadcastMessage(const CanMessage &message) {
  for (const auto &[id, session] : m_sessions) {
    session->send(message);
  }
}

std::vector<std::string> TcpServer::getConnectedClients() const {
  std::vector<std::string> clients;
  clients.reserve(m_sessions.size());

  for (const auto &[id, session] : m_sessions) {
    clients.push_back(id);
  }

  return clients;
}

void TcpServer::setMessageCallback(MessageCallback callback) {
  m_messageCallback = std::move(callback);
}

void TcpServer::setConnectCallback(ConnectCallback callback) {
  m_connectCallback = std::move(callback);
}

void TcpServer::setDisconnectCallback(DisconnectCallback callback) {
  m_disconnectCallback = std::move(callback);
}

void TcpServer::doAccept() {
  m_acceptor.async_accept([this](std::error_code ec, tcp::socket socket) {
    if (!ec) {
      m_nextId++;
      std::string id = std::to_string(m_nextId);
      auto session = std::make_shared<Session>(std::move(socket), *this, id);
      m_sessions[id] = session;
      session->start();

      if (m_connectCallback) {
        m_connectCallback(id);
      }
    }

    if (m_running) {
      doAccept();
    }
  });
}

void TcpServer::removeSession(const std::string &id) {
  auto it = m_sessions.find(id);
  if (it != m_sessions.end()) {
    m_sessions.erase(it);

    if (m_disconnectCallback) {
      m_disconnectCallback(id);
    }
  }
}

TcpServer::Session::Session(tcp::socket socket, TcpServer &server,
                            std::string id)
    : m_socket(std::move(socket)), m_server(server), m_id(std::move(id)),
      m_bytesNeeded(4) { // First need 4 bytes for length header
  m_readBuffer.resize(1024);
}

void TcpServer::Session::start() { doRead(); }

void TcpServer::Session::stop() {
  std::error_code ec;
  m_socket.close(ec);
}

bool TcpServer::Session::send(const CanMessage &message) {
  try {
    auto data = message.serialize();

    // Prepend with a 4-byte length header
    std::vector<uint8_t> buffer;
    buffer.reserve(data.size() + 4);

    auto size = static_cast<uint32_t>(data.size());
    buffer.push_back(static_cast<uint8_t>((size >> 24) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((size >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((size >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(size & 0xFF));

    buffer.insert(buffer.end(), data.begin(), data.end());

    // Synchronous write for simplicity
    asio::write(m_socket, asio::buffer(buffer));
    return true;
  } catch (const asio::system_error &exception) {
    std::cerr << "Error sending to client " << m_id << ": " << exception.what()
              << std::endl;
    return false;
  }
}

std::string TcpServer::Session::getId() const { return m_id; }

void TcpServer::Session::doRead() {
  auto self = shared_from_this();
  m_socket.async_read_some(
      asio::buffer(m_readBuffer),
      [this, self](std::error_code ec, std::size_t bytesRead) {
        if (!ec) {
          // Process read data
          for (size_t i = 0; i < bytesRead; ++i) {
            m_messageBuffer.push_back(m_readBuffer[i]);

            if (m_messageBuffer.size() == m_bytesNeeded) {
              if (m_bytesNeeded == 4) {
                // We have the length header, now get the message body
                uint32_t msgLength =
                    static_cast<uint32_t>(m_messageBuffer[0]) << 24 |
                    static_cast<uint32_t>(m_messageBuffer[1]) << 16 |
                    static_cast<uint32_t>(m_messageBuffer[2]) << 8 |
                    static_cast<uint32_t>(m_messageBuffer[3]);

                m_bytesNeeded += msgLength;
              } else {
                // We have the full message
                processMessage();

                // Reset for next message
                m_messageBuffer.clear();
                m_bytesNeeded = 4;
              }
            }
          }

          // Continue reading
          doRead();
        } else {
          // Handle errors
          if (ec != asio::error::eof && ec != asio::error::connection_reset) {
            std::cerr << "Error reading from client " << m_id << ": "
                      << ec.message() << std::endl;
          }

          // Remove this session
          m_server.removeSession(m_id);
        }
      });
}

void TcpServer::Session::processMessage() {
  // First 4 bytes are the length header, skip them
  std::vector<uint8_t> canData(m_messageBuffer.begin() + 4,
                               m_messageBuffer.end());

  auto CanMessage = CanMessage::deserialize(canData);

  if (m_server.m_messageCallback) {
    m_server.m_messageCallback(m_id, CanMessage);
  }
}
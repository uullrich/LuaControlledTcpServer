# Lua-Controlled TCP Server

This project provides a TCP server that can be controlled and configured via Lua scripts.

## Building the Project

```bash
mkdir build && cd build
cmake ..
make
```

## Running the Server

### Default Script

```bash
./LuaControlledTcpServer
```

This will run with the default `scripts/server.lua`.

### Custom Script

```bash
./LuaControlledTcpServer scripts/simple.lua
```

## Available Lua Functions

### Server Control

- `startServer(port)` - Start TCP server on specified port
- `stopServer()` - Stop the TCP server
- `log(message)` - Print log message
- `logError(message)` - Print error message
- `wait(milliseconds)` - Wait for specified time

### CAN Message Handling

- `createCANMessage(id, data, extended, rtr)` - Create a CAN message
  - `id`: CAN ID (number)
  - `data`: Array of bytes (table)
  - `extended`: Extended frame flag (boolean)
  - `rtr`: Remote transmission request flag (boolean)
- `sendCANMessage(clientId, messageId)` - Send message to specific client
- `broadcastCANMessage(messageId)` - Send message to all connected clients
- `getConnectedClients()` - Get list of connected client IDs

### Event Callbacks

Define these functions in your Lua script to handle events:

- `onClientConnected(clientId)` - Called when client connects
- `onClientDisconnected(clientId)` - Called when client disconnects
- `onMessageReceived(clientId, canId, data, extended, rtr)` - Called when message received

## Example Script Structure

```lua
function main()
    -- Your setup code here
    startServer(8080)

    -- Define event handlers
    function onClientConnected(clientId)
        log("Client connected: " .. clientId)
    end

    function onMessageReceived(clientId, canId, data, extended, rtr)
        -- Handle incoming messages
        local response = createCANMessage(canId + 1, data, false, false)
        sendCANMessage(clientId, response)
    end

    return true
end
```

## Testing with Telnet

You can test the server using telnet:

```bash
telnet localhost 8080
```

The server expects CAN message format:

- 4-byte message length (little-endian)
- 4-byte CAN ID (little-endian)
- 1-byte flags (extended/RTR)
- 1-byte data length
- N bytes of CAN data

-- Simple TCP server test script
local PORT = 3337

function onClientConnected(clientId)
    log("NEW CONNECTION: " .. clientId)

    -- Send a greeting message: Greet
    local greeting = createCANMessage(0x001, {0x47, 0x72, 0x65, 0x65, 0x74}, false, false)
    sendCANMessage(clientId, greeting)
end

function onClientDisconnected(clientId)
    log("DISCONNECTED: " .. clientId)
end

function onMessageReceived(clientId, canId, data, extended, rtr)
    log("MESSAGE from " .. clientId .. ": CAN ID 0x" .. string.format("%03X", canId))

    -- Simple echo with incremented CAN ID
    local response = createCANMessage(canId + 0x100, data, extended, rtr)
    sendCANMessage(clientId, response)
end

function main()
    log("=== Simple TCP Server Test ===")
    startServer(PORT)
    log("Server listening on port " .. PORT)
    log("Server configured and running!")
    return true
end

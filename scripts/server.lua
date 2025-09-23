local SERVER_PORT = 3337

-- Main function called by C++
function main()
    log("Starting TCP Server from Lua script...")

    setupCallbacks()

    startServer(SERVER_PORT)

    log("Server started on port " .. SERVER_PORT)
    log("Waiting for connections...")

    return true
end

function setupCallbacks()
    function onClientConnected(clientId)
        log("Client connected: " .. clientId)

        -- Send welcome message to new client: Hello
        local welcomeMsg = createCANMessage(0x100, {0x48, 0x65, 0x6c, 0x6c, 0x6f}, false, false)
        sendCANMessage(clientId, welcomeMsg)
    end

    function onClientDisconnected(clientId)
        log("Client disconnected: " .. clientId)
    end

    function onMessageReceived(clientId, canId, data, extended, rtr)
        log("Received message from " .. clientId .. " - CAN ID: 0x" .. string.format("%X", canId))

        -- Print the data bytes
        local dataStr = "Data: ["
        for i = 1, #data do
            dataStr = dataStr .. "0x" .. string.format("%02X", data[i])
            if i < #data then
                dataStr = dataStr .. ", "
            end
        end
        dataStr = dataStr .. "]"
        log(dataStr)

        -- Echo the message back to sender
        local echoMsg = createCANMessage(canId + 1, data, extended, rtr)
        sendCANMessage(clientId, echoMsg)

        -- If it's a broadcast request (CAN ID 0x200), send to all clients
        if canId == 0x200 then
            log("Broadcasting message to all clients")
            broadcastCANMessage(echoMsg)
        end
    end
end

-- Example function to demonstrate periodic messages - unused
function sendPeriodicMessages()
    local counter = 0

    while true do
        wait(5000)

        local clients = getConnectedClients()
        if #clients > 0 then
            log("Sending periodic message to " .. #clients .. " clients")

            -- Create periodic status message
            local statusData = {counter & 0xFF, (counter >> 8) & 0xFF, 0xAA, 0xBB}
            local statusMsg = createCANMessage(0x300 + counter, statusData, false, false)

            broadcastCANMessage(statusMsg)
            counter = counter + 1
        end
    end
end

log("Lua script loaded successfully")

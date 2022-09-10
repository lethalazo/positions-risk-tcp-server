#include "../include/risk_server/server.hpp"

/*
* Add a master socket to the server's socket descriptor set. Also add child 
* sockets to the socket descriptor set and update maxDescriptor for select.
*/
void RiskServer::addMasterAndChildSockets()
{
    FD_ZERO(&socketDescriptorSet);

    FD_SET(masterSocket, &socketDescriptorSet);
    maxDescriptor = masterSocket;

    for (auto it = clientSocket.begin(); it != clientSocket.end(); it++)
    {
        int socketDescriptor = *it;

        // Check socket validity.
        if (socketDescriptor > 0)
            FD_SET(socketDescriptor, &socketDescriptorSet);

        maxDescriptor = std::max(maxDescriptor, socketDescriptor);
    }
}

/*
* Add a new user's socket descriptor to the client sockets and update the 
* user's id to order id map with user's socket descriptor.
*
* Parameters
* ----------
* newSocket : uint64_t
*     The client's socket descriptor.
*/
void RiskServer::addUser(uint64_t newSocket)
{
    clientSocket.insert(newSocket);
    userId2Order[newSocket] = new std::vector<uint64_t>();
}

/*
* LOG user's client details, remove user data from the server and close the 
* connection to the socket descriptor.
*
* Parameters
* ----------
* socketDescriptor : int
*     The client's socket descriptor.
*/
void RiskServer::closeConnection(int socketDescriptor)
{
    struct sockaddr_in address;
    int addressLen;
    getpeername(socketDescriptor, (struct sockaddr *)&address, (socklen_t *)&addressLen);

    printf("LOG Disconnected %s:%d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

    removeUser(socketDescriptor);
    close(socketDescriptor);
}

/*
* Read provided header and message to create a new order and update the user's 
* position data. 
*
* Respond with updated OrderResponse with OrderResponse::Status::ACCEPTED or 
* OrderResponse::Status::REJECTED.
*
* Parameters
* ----------
* socketDescriptor : int
*     The client's socket descriptor.
* buffer : char*
*     The message buffer.
* header : Header
*     Reference to the message header.
* orderResponse : OrderResponse
*     Reference to the order response to update.
*/
void RiskServer::createNewOrder(int socketDescriptor, char *buffer, Header &header, OrderResponse &orderResponse)
{
    orderResponse.messageType = OrderResponse::MESSAGE_TYPE;
    if (header.payloadSize != sizeof(NewOrder))
    {
        orderResponse.status = OrderResponse::Status::REJECTED;
        std::cerr << ERR_INVALID_DATA << std::endl;
        return;
    }

    NewOrder newOrder;
    std::memcpy(&newOrder, buffer, header.payloadSize);
    orderResponse.orderId = newOrder.orderId;

    if (newOrder.orderPrice <= 0 || newOrder.orderQuantity == 0)
    {
        orderResponse.status = OrderResponse::Status::REJECTED;
        std::cerr << ERR_INVALID_DATA << std::endl;
    }
    else if (orderId2Order.count(newOrder.orderId))
    {
        orderResponse.status = OrderResponse::Status::REJECTED;
        std::cerr << ERR_ORDER_ALREADY_EXISTS << std::endl;
    }
    else
    {
        std::shared_ptr<Order> order(new Order(newOrder.orderId, newOrder.listingId, newOrder.orderQuantity, newOrder.orderPrice, newOrder.side));

        if (!instrumentId2PositionData.count(newOrder.listingId))
        {
            std::shared_ptr<PositionData> pos(new PositionData());
            instrumentId2PositionData[newOrder.listingId] = pos;
        }
        bool added = instrumentId2PositionData[newOrder.listingId]->addPosition(order, BUY_THRESHOLD, SELL_THRESHOLD);
        if (added)
        {
            orderId2Order[order->orderId] = order;
            userId2Order[socketDescriptor]->push_back(order->orderId);
            orderResponse.status = OrderResponse::Status::ACCEPTED;
            std::cout << SUCC_NEW_ORDER_CREATED << std::endl;
        }
        else
        {
            orderResponse.status = OrderResponse::Status::REJECTED;
            std::cout << WARN_NEW_ORDER_REJECTED << std::endl;
        }
    }
}

/*
* Read provided header and message to delete an order and update the user's 
* position data. 
*
* Parameters
* ----------
* buffer : char*
*     The message buffer.
* header : Header
*     Reference to the message header.
*/
void RiskServer::deleteExistingOrder(char *buffer, Header &header)
{
    if (header.payloadSize != sizeof(DeleteOrder))
    {
        std::cerr << ERR_INVALID_DATA << std::endl;
        return;
    }
    DeleteOrder deleteOrder;
    std::memcpy(&deleteOrder, buffer, header.payloadSize);
    auto it = orderId2Order.find(deleteOrder.orderId);
    if (it == orderId2Order.end())
    {
        std::cerr << ERR_ORDER_DOES_NOT_EXIST << std::endl;
    }
    else
    {
        std::shared_ptr<Order> order = it->second;
        std::shared_ptr<PositionData> pos = instrumentId2PositionData.find(order->financialInstrumentId)->second;
        pos->rollbackPosition(order);
        orderId2Order.erase(it);
        std::cout << SUCC_ORDER_DELETED << " ORDER_ID=" << order->orderId << std::endl;
    }
}

/*
* Read provided header and message to execute trade.
*
* Parameters
* ----------
* buffer : char*
*     The message buffer.
* header : Header
*     Reference to the message header.
*/
void RiskServer::executeTrade(char *buffer, Header &header)
{
    if (header.payloadSize != sizeof(Trade))
    {
        std::cerr << ERR_INVALID_DATA << std::endl;
        return;
    }

    Trade trade;
    std::memcpy(&trade, buffer, header.payloadSize);
    if (trade.tradePrice <= 0 || trade.tradeQuantity == 0)
    {
        std::cerr << ERR_INVALID_DATA << std::endl;
        return;
    }

    auto it = orderId2Order.find(trade.tradeId);
    if (it == orderId2Order.end())
    {
        std::cerr << ERR_ORDER_DOES_NOT_EXIST << std::endl;
    }
    else
    {
        std::shared_ptr<PositionData> pos = instrumentId2PositionData.find(trade.listingId)->second;
        pos->trade(trade.tradeQuantity);
        std::cout << SUCC_TRADE_EXECUTED << std::endl;
    }
}

/*
* Handle the socket operations for each client socket, keep track of closed
* sockets to erase and handle any new messages.
*/
void RiskServer::handleClientSocketIOOperations()
{
    std::vector<int> closedSockets;
    for (auto it = clientSocket.begin(); it != clientSocket.end(); it++)
    {
        int socketDescriptor = *it;

        if (FD_ISSET(socketDescriptor, &socketDescriptorSet))
        {
            char buffer[1024];
            int valread;

            // Check if client socket is closing.
            if ((valread = read(socketDescriptor, buffer, sizeof(Header))) <= 0)
            {
                closeConnection(socketDescriptor);
                closedSockets.push_back(socketDescriptor);
            }
            // Handle message and respond if required.
            else
            {
                OrderResponse orderResponse;
                Header header;
                bool reply = handleMessage(socketDescriptor, orderResponse, buffer, header);
                if (reply)
                {
                    Header responseHeader;
                    responseHeader.payloadSize = sizeof(OrderResponse);
                    responseHeader.sequenceNumber = header.sequenceNumber + 1;
                    responseHeader.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

                    u_long headerSize = sizeof(Header);
                    char *message = new char[headerSize + responseHeader.payloadSize];
                    std::memcpy(message, &responseHeader, headerSize);
                    std::memcpy(message + headerSize, &orderResponse, responseHeader.payloadSize);
                    send(socketDescriptor, message, headerSize + responseHeader.payloadSize, 0);
                }
            }
        }
    }
    for (int closed : closedSockets) clientSocket.erase(closed);
}

/*
* Read provided header and message type to handle the message and reponse.
*
* Parameters
* ----------
* socketDescriptor : int
*     The client's socket descriptor.
* orderResponse : OrderResponse
*     Reference to the order response to update.
* buffer : char*
*     The message buffer.
* header : Header
*     Reference to the message header.
*
* Returns
* -------
* reply : bool
*     true if client is expecting a reply, false otherwise.
*/
bool RiskServer::handleMessage(int socketDescriptor, OrderResponse &orderResponse, char *buffer, Header &header)
{
    bool reply;
    std::memcpy(&header, buffer, sizeof(Header));
    int valread = read(socketDescriptor, buffer, header.payloadSize);
    uint16_t *messageType = (uint16_t *)buffer;
    switch (*messageType)
    {
    case NewOrder::MESSAGE_TYPE:
    {
        createNewOrder(socketDescriptor, buffer, header, orderResponse);
        reply = true;
        break;
    }
    case DeleteOrder::MESSAGE_TYPE:
    {
        deleteExistingOrder(buffer, header);
        reply = false;
        break;
    }
    case ModifyOrderQuantity::MESSAGE_TYPE:
    {
        modifyExistingOrder(buffer, header, orderResponse);
        reply = true;
        break;
    }
    case Trade::MESSAGE_TYPE:
    {
        executeTrade(buffer, header);
        reply = false;
        break;
    }
    }
    return reply;
}


/*
* Handle a new connection.
*
* Parameters
* ----------
* socketDescriptor : int
*     The client's socket descriptor.
* address : sockaddr_in
*     The client's address details.
*/
void RiskServer::handleNewConnection(int newSocket, struct sockaddr_in address)
{
    if (newSocket < 0)
    {
        std::cerr << "accept" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "LOG New Connection " << inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port) << " SOCK FD" << newSocket << std::endl;
    addUser(newSocket);
}

/*
* Initialize a master socket and address, bind socket to PORT.
* Listen master and client sockets for activity and handle any connections/
* operations.
*/
void RiskServer::initListenerSocket()
{
    int opt = 1;
    // Create and set master socket to allow multiple connections.
    if ((masterSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        std::cerr << "socket failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
    {
        std::cerr << "setsockopt" << std::endl;
        exit(EXIT_FAILURE);
    }

    // TCP socket.
    mAddress.sin_family = AF_INET;
    mAddress.sin_addr.s_addr = INADDR_ANY;
    mAddress.sin_port = htons(PORT);
    mAddressLen = sizeof(mAddress);

    // Bind the socket to PORT.
    if (bind(masterSocket, (struct sockaddr *)&mAddress, sizeof(mAddress)) < 0)
    {
        std::cerr << "ERR 00 <MASTER_SOCKET_BINDING>" << std::endl;
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);

    // Maximum 3 pending sockets to listen.
    if (listen(masterSocket, 3) < 0)
    {
        std::cerr << "ERR 00 <MASTER_SOCKET_LISTEN>" << std::endl;
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        addMasterAndChildSockets();

        // Wait indefinitely for an activity on one of the sockets in the socket descriptor set.
        int activity = select(maxDescriptor + 1, &socketDescriptorSet, NULL, NULL, NULL);

        // Invalid socket selected.
        if ((activity < 0) && (errno != EINTR))
        {
            std::cerr << "ERR 00 <SELECTING_SOCKET>" << std::endl;
        }

        // If new connection on master socket...
        if (FD_ISSET(masterSocket, &socketDescriptorSet))
        {
            struct sockaddr_in address;
            int addressLen;
            int newSocket = accept(masterSocket, (struct sockaddr *)&address, (socklen_t *)&addressLen);
            handleNewConnection(newSocket, address);
        }

        // Hnadle IO operations for all client sockets.
        handleClientSocketIOOperations();
    }
}

/*
* Read provided header and message to modify an existing order and update the 
* user's position data. 
*
* Respond with updated OrderResponse with OrderResponse::Status::ACCEPTED or 
* OrderResponse::Status::REJECTED.
*
* Parameters
* ----------
* buffer : char*
*     The message buffer.
* header : Header
*     Reference to the message header.
* orderResponse : OrderResponse
*     Reference to the order response to update.
*/
void RiskServer::modifyExistingOrder(char *buffer, Header &header, OrderResponse &orderResponse)
{
    orderResponse.messageType = OrderResponse::MESSAGE_TYPE;
    if (header.payloadSize != sizeof(ModifyOrderQuantity))
    {
        orderResponse.status = OrderResponse::Status::REJECTED;
        std::cerr << ERR_INVALID_DATA << std::endl;
        return;
    }

    ModifyOrderQuantity modifyOrderQuantity;
    std::memcpy(&modifyOrderQuantity, buffer, header.payloadSize);
    orderResponse.orderId = modifyOrderQuantity.orderId;
    if (modifyOrderQuantity.newQuantity <= 0)
    {
        orderResponse.status = OrderResponse::Status::REJECTED;
        std::cerr << ERR_INVALID_DATA << std::endl;
        return;
    }

    auto it = orderId2Order.find(modifyOrderQuantity.orderId);
    if (it == orderId2Order.end())
    {
        orderResponse.status = OrderResponse::Status::REJECTED;
        std::cerr << ERR_ORDER_DOES_NOT_EXIST << std::endl;
    }
    else
    {
        std::shared_ptr<Order> order = it->second;
        std::shared_ptr<PositionData> pos = instrumentId2PositionData.find(order->financialInstrumentId)->second;

        bool added = pos->modifyPosition(order, modifyOrderQuantity.newQuantity, BUY_THRESHOLD, SELL_THRESHOLD);
        if (added)
        {
            orderResponse.status = OrderResponse::Status::ACCEPTED;
            std::cout << SUCC_ORDER_QUANTITY_MODIFIED << " ORDER_ID=" << order->orderId << " ORDER_QUANTITY=" << order->qty << std::endl;
        }
        else
        {
            orderResponse.status = OrderResponse::Status::REJECTED;
            std::cout << WARN_MODIFY_ORDER_REJECTED << std::endl;
        }
    }
}

/*
* Remove all order's of the user, rollback position data and delete the user's 
* socket descriptor from the user id to order id map.
*
* Parameters
* ----------
* socketDescriptor : uint64_t
*     The client's socket descriptor.
*/
void RiskServer::removeUser(uint64_t socketDescriptor)
{
    for (uint64_t orderId : *userId2Order[socketDescriptor])
    {
        auto it = orderId2Order.find(orderId);
        if (it != orderId2Order.end())
        {
            std::shared_ptr<Order> order = it->second;
            std::shared_ptr<PositionData> pos = instrumentId2PositionData.find(order->financialInstrumentId)->second;
            pos->rollbackPosition(order);
            orderId2Order.erase(it);
        }
    }
    auto it = userId2Order.find(socketDescriptor);
    userId2Order.erase(it);
}
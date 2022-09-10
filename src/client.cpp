#include "../include/risk_server/client.hpp"

RiskClient::RiskClient(uint64_t p)
{
    PORT = p;
    initSocket();
}

/*
* Runs the CLI asking for user input via standard input.
* Creates message by user provided parameters, sends the message to the server
* on PORT and prints the response (ACCEPTED / REJECTED / N/A).
*/
void RiskClient::runCLI()
{
    Header header;
    bool messageSent = false;
    while (!messageSent)
    {
        uint16_t messageType = 0;
        std::cout << "Insert message type: ";
        std::cin >> messageType;
        switch (messageType)
        {
        case 1:
        {
            char *message = createNewOrderMessage(std::make_shared<Header>(header));
            sendMessage(header, message, true);
            messageSent = true;
            break;
        }
        case 2:
        {
            char *message = createDeleteOrderMessage(std::make_shared<Header>(header));
            sendMessage(header, message, false);
            messageSent = true;
            break;
        }
        case 3:
        {
            char *message = createModifyOrderQuantityMessage(std::make_shared<Header>(header));
            sendMessage(header, message, true);
            messageSent = true;
            break;
        }
        case 4:
        {
            char *message = createTradeMessage(std::make_shared<Header>(header));
            sendMessage(header, message, false);
            messageSent = true;
            break;
        }
        default:
        {
            std::cout << "Invalid, try again" << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::digits, '\n');
            break;
        }
        }
    }
}

/*
* Updates header and creates a new order message.
*
* Parameters
* ----------
* header : std::shared_ptr<Header>
*     Pointer to the header to update and create message.
* 
* Returns
* -------
* message : char*
*     The message to send to the server.
*/
char *RiskClient::createNewOrderMessage(std::shared_ptr<Header> header)
{
    NewOrder order;
    const auto timestamp_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    std::cin >> order.listingId;
    std::cin >> order.orderId;
    std::cin >> order.orderQuantity;
    std::cin >> order.orderPrice;
    std::cin >> order.side;
    order.messageType = 1;

    header->version = 0;
    header->sequenceNumber = 0;
    header->timestamp = timestamp_since_epoch;
    header->payloadSize = sizeof(order);

    u_long headerSize = sizeof(Header);
    char *message = new char[headerSize + header->payloadSize];
    std::memcpy(message, header.get(), headerSize);
    std::memcpy(message + headerSize, &order, header->payloadSize);
    return message;
}

/*
* Updates header and creates a delete order message.
*
* Parameters
* ----------
* header : std::shared_ptr<Header>
*     Pointer to the header to update and create message.
* 
* Returns
* -------
* message : char*
*     The message to send to the server.
*/
char *RiskClient::createDeleteOrderMessage(std::shared_ptr<Header> header)
{
    DeleteOrder order;

    const auto timestamp_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    std::cin >> order.orderId;
    order.messageType = 2;

    header->version = 0;
    header->payloadSize = sizeof(order);
    header->sequenceNumber = 0;
    header->timestamp = timestamp_since_epoch;

    u_long headerSize = sizeof(Header);
    char *message = new char[headerSize + header->payloadSize];
    std::memcpy(message, header.get(), headerSize);
    std::memcpy(message + headerSize, &order, header->payloadSize);
    return message;
}

/*
* Updates header and creates a modify order quantity message.
*
* Parameters
* ----------
* header : std::shared_ptr<Header>
*     Pointer to the header to update and create message.
* 
* Returns
* -------
* message : char*
*     The message to send to the server.
*/
char *RiskClient::createModifyOrderQuantityMessage(std::shared_ptr<Header> header)
{
    ModifyOrderQuantity order;

    const auto timestamp_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    std::cin >> order.orderId;
    std::cin >> order.newQuantity;
    order.messageType = 3;

    header->version = 0;
    header->payloadSize = sizeof(order);
    header->sequenceNumber = 0;
    header->timestamp = timestamp_since_epoch;
    
    u_long headerSize = sizeof(Header);
    char *message = new char[headerSize + header->payloadSize];
    std::memcpy(message, header.get(), headerSize);
    std::memcpy(message + headerSize, &order, header->payloadSize);
    return message;
}

/*
* Updates header and creates a new trade message.
*
* Parameters
* ----------
* header : std::shared_ptr<Header>
*     Pointer to the header to update and create message.
* 
* Returns
* -------
* message : char*
*     The message to send to the server.
*/
char *RiskClient::createTradeMessage(std::shared_ptr<Header> header)
{
    Trade order;
    const auto timestamp_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    std::cin >> order.listingId;
    std::cin >> order.tradeId;
    std::cin >> order.tradeQuantity;
    std::cin >> order.tradePrice;
    order.messageType = 4;
    header->version = 0;
    header->payloadSize = sizeof(order);
    header->sequenceNumber = 0;
    header->timestamp = timestamp_since_epoch;

    u_long headerSize = sizeof(Header);
    char *message = new char[headerSize + header->payloadSize];
    std::memcpy(message, header.get(), headerSize);
    std::memcpy(message + headerSize, &order, header->payloadSize);
    return message;
}

/*
* Initializes a TCP socket on the provided PORT and connects to the server 
* address (mAddress).
*/
void RiskClient::initSocket()
{
    if ((mSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "Socket creation error" << std::endl;
        exit(EXIT_FAILURE);
    }

    mAddress.sin_family = AF_INET;
    mAddress.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses to binary.
    if (inet_pton(AF_INET, "127.0.0.1", &mAddress.sin_addr) <= 0)
    {
        std::cerr << "Invalid address" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (connect(mSocket, (struct sockaddr *)&mAddress, sizeof(mAddress)) < 0)
    {
        std::cerr << "Connection failed" << std::endl;
        exit(EXIT_FAILURE);
    }
}

/*
* Send a message to the server.
*
* Parameters
* ----------
* header : std::shared_ptr<Header>
*     Pointer to the header.
* message : char*
*     The message to send to the server.
* replyExpected : bool
*     true if reply is expected from the server. false otherwise.
*
* Returns
* -------
* accepted : bool
*     true if the message was accepted, false if message is rejected or 
*     if no reply expected.
*/
bool RiskClient::sendMessage(Header &header, char *message, bool replyExpected)
{
    send(mSocket, message, sizeof(header) + header.payloadSize, 0);

    if (replyExpected)
    {
        char buffer[1024];
        int valread = read(mSocket, buffer, sizeof(Header));

        Header responseHeader;
        std::memcpy(&responseHeader, buffer, sizeof(Header));

        if (responseHeader.payloadSize != sizeof(OrderResponse))
        {
            std::cerr << ERR_INVALID_DATA << std::endl;
            exit(EXIT_FAILURE);
        }

        OrderResponse reply;
        valread = read(mSocket, buffer, responseHeader.payloadSize);
        std::memcpy(&reply, buffer, responseHeader.payloadSize);
        if (reply.status == OrderResponse::Status::ACCEPTED)
        {
            std::cout << MESSAGE_ACCEPTED << std::endl;
            return true;
        }
        std::cout << MESSAGE_REJECTED << std::endl;
        return false;
    }

    return true;
}



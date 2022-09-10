#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <limits>
#include "strings.hpp"
#include "message.hpp"

class RiskClient
{
public:
    RiskClient(uint64_t p);
    char *createDeleteOrderMessage(std::shared_ptr<Header> Header);
    char *createModifyOrderQuantityMessage(std::shared_ptr<Header> header);
    char *createNewOrderMessage(std::shared_ptr<Header> header);
    char *createTradeMessage(std::shared_ptr<Header> header);
    bool sendMessage(Header &header, char *message, bool replyExpected);

    void runCLI();

private:
    void initSocket();

    uint64_t PORT;
    struct sockaddr_in mAddress;
    int mSocket;
};

#endif
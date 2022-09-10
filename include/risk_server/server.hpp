#ifndef SERVER_HPP
#define SERVER_HPP

#include <arpa/inet.h>
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <netinet/in.h>
#include <set>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "message.hpp"
#include "position_data.hpp"
#include "strings.hpp"

class RiskServer
{
public:
    RiskServer(uint64_t b, uint64_t s, int p) : BUY_THRESHOLD(b), SELL_THRESHOLD(s), PORT(p) {}
    void addUser(uint64_t newSocket);
    void addMasterAndChildSockets();
    void closeConnection(int newSocket);

    void createNewOrder(int socketDescriptor, char *buffer, Header &header, OrderResponse &orderResponse);
    void deleteExistingOrder(char *buffer, Header &header);
    void executeTrade(char *buffer, Header &header);

    void handleClientSocketIOOperations();
    bool handleMessage(int socketDescriptor, OrderResponse &orderResponse, char *buffer, Header &header);
    void handleNewConnection(int newSocket, struct sockaddr_in address);
    void initListenerSocket();
    
    void modifyExistingOrder(char *buffer, Header &header, OrderResponse &orderResponse);

    void removeUser(uint64_t socketDescriptor);

private:
    uint64_t BUY_THRESHOLD = 0, SELL_THRESHOLD = 0;
    int PORT = 0;
    std::unordered_map<int, std::vector<uint64_t> *> userId2Order;
    std::unordered_map<int, std::shared_ptr<Order>> orderId2Order;
    std::unordered_map<int, std::shared_ptr<PositionData>> instrumentId2PositionData;
    fd_set socketDescriptorSet;
    int masterSocket, mAddressLen, maxDescriptor;
    struct sockaddr_in mAddress;
    std::set<int> clientSocket;
};

#endif
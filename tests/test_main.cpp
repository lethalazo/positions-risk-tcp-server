// Cover the initial test case and edge cases
#include "../include/risk_server/client.hpp"
#include <iostream>
#include <assert.h>

#define PORT 51717


/*  
*   ==========================
*   HELPER FUNCTIONS
*   ==========================
*/


void helper_createNewOrder(Header& header, NewOrder& order, uint64_t listingId, uint64_t orderId, uint64_t quantity, uint64_t price, char side) {
    const auto timestamp_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    order.listingId = listingId;
    order.orderId = orderId;
    order.orderQuantity = quantity;
    order.orderPrice = price;
    order.side = side;
    order.messageType = 1;

    header.version = 0;
    header.sequenceNumber = 0;
    header.timestamp = timestamp_since_epoch;
    header.payloadSize = sizeof(order);
}

void helper_createTrade(Header& header, Trade& order, uint64_t listingId, uint64_t tradeId, int64_t quantity, uint64_t price) {
    const auto timestamp_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    order.listingId = listingId;
    order.tradeId = tradeId;
    order.tradeQuantity = quantity;
    order.tradePrice = price;
    order.messageType = 4;

    header.version = 0;
    header.sequenceNumber = 0;
    header.timestamp = timestamp_since_epoch;
    header.payloadSize = sizeof(order);
}

void helper_deleteOrder(Header& header, DeleteOrder& order, uint64_t orderId) {
    const auto timestamp_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    order.orderId = orderId;
    order.messageType = 2;

    header.version = 0;
    header.sequenceNumber = 0;
    header.timestamp = timestamp_since_epoch;
    header.payloadSize = sizeof(order);
}

void helper_modifyOrder(Header& header, ModifyOrderQuantity& order, uint64_t orderId, uint64_t quantity) {
    const auto timestamp_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    order.orderId = orderId;
    order.newQuantity = quantity;
    order.messageType = 3;

    header.version = 0;
    header.sequenceNumber = 0;
    header.timestamp = timestamp_since_epoch;
    header.payloadSize = sizeof(order);
}

/*  
*   ==========================
*   CUSTOM TESTCASES
*   ==========================
*/

void test_newOrderDuplicateId(std::shared_ptr<RiskClient> client) {
std::cout << "TEST NEW BUY ORDER <ACCEPTED>" << std::endl;
    Header header;
    NewOrder order;

    helper_createNewOrder(header, order, 1, 13, 10, 10'0000, 'B');

    u_long headerSize = sizeof(Header);
    char *message = new char[headerSize + header.payloadSize];
    std::memcpy(message, &header, headerSize);
    std::memcpy(message + headerSize, &order, header.payloadSize);

    assert(client->sendMessage(header, message, true));
    std::cout << "PASSED!" << std::endl;

    std::cout << "TEST DUPLICATE BUY ORDER <REJECTED>" << std::endl;
    Header header2;
    NewOrder order2;

    helper_createNewOrder(header2, order2, 1, 13, 10, 10'0000, 'B');

    message = new char[headerSize + header2.payloadSize];
    std::memcpy(message, &header2, headerSize);
    std::memcpy(message + headerSize, &order2, header2.payloadSize);

    assert(!client->sendMessage(header2, message, true));
    std::cout << "PASSED!" << std::endl;
}

void test_modifyExistingOrder(std::shared_ptr<RiskClient> client) {
    std::cout << "TEST NEW BUY ORDER <ACCEPTED>" << std::endl;
    Header header;
    NewOrder order;

    helper_createNewOrder(header, order, 1, 5, 10, 10'0000, 'B');

    u_long headerSize = sizeof(Header);
    char *message = new char[headerSize + header.payloadSize];
    std::memcpy(message, &header, headerSize);
    std::memcpy(message + headerSize, &order, header.payloadSize);

    assert(client->sendMessage(header, message, true));
    std::cout << "PASSED!" << std::endl;

    std::cout << "TEST MODIFY EXISTING ORDER <ACCEPTED>" << std::endl;
    Header header2;
    ModifyOrderQuantity order2;

    helper_modifyOrder(header2, order2, 5, 10);

    message = new char[headerSize + header2.payloadSize];
    std::memcpy(message, &header2, headerSize);
    std::memcpy(message + headerSize, &order2, header2.payloadSize);

    assert(!client->sendMessage(header2, message, true));
    std::cout << "PASSED!" << std::endl;
}

void test_modifyNonExistingOrder(std::shared_ptr<RiskClient> client) {
    std::cout << "TEST MODIFY NON-EXISTING ORDER <REJECTED>" << std::endl;
    u_long headerSize = sizeof(Header);
    Header header;
    ModifyOrderQuantity order;

    helper_modifyOrder(header, order, 99, 10);

    char *message = new char[headerSize + header.payloadSize];
    std::memcpy(message, &header, headerSize);
    std::memcpy(message + headerSize, &order, header.payloadSize);

    assert(!client->sendMessage(header, message, true));
    std::cout << "PASSED!" << std::endl;
}

/* 
* Simple main runner to test multiple cases.
*/
int main() {
    std::shared_ptr<RiskClient> client(new RiskClient(PORT));
    
    std::cout << "TEST NEW BUY ORDER <ACCEPTED>" << std::endl;
    Header header;
    NewOrder order;

    helper_createNewOrder(header, order, 1, 1, 10, 10'0000, 'B');

    u_long headerSize = sizeof(Header);
    char *message = new char[headerSize + header.payloadSize];
    std::memcpy(message, &header, headerSize);
    std::memcpy(message + headerSize, &order, header.payloadSize);

    assert(client->sendMessage(header, message, true));
    std::cout << "PASSED!" << std::endl;

    std::cout << "TEST NEW SELL ORDER DIFFERENT LISTING <ACCEPTED>" << std::endl;
    Header header2;
    NewOrder order2;

    helper_createNewOrder(header2, order2, 2, 2, 15, 1'5000, 'S');

    message = new char[headerSize + header2.payloadSize];
    std::memcpy(message, &header2, headerSize);
    std::memcpy(message + headerSize, &order2, header2.payloadSize);

    assert(client->sendMessage(header2, message, true));
    std::cout << "PASSED!" << std::endl;

    std::cout << "TEST NEW BUY ORDER DIFFERENT LISTING <ACCEPTED>" << std::endl;
    Header header3;
    NewOrder order3;

    helper_createNewOrder(header3, order3, 2, 3, 4, 1'5000, 'B');

    message = new char[headerSize + header3.payloadSize];
    std::memcpy(message, &header3, headerSize);
    std::memcpy(message + headerSize, &order3, header3.payloadSize);

    assert(client->sendMessage(header3, message, true));
    std::cout << "PASSED!" << std::endl;

    std::cout << "TEST NEW BUY ORDER OVER THRESHOLD <REJECTED>" << std::endl;
    Header header4;
    NewOrder order4;

    helper_createNewOrder(header4, order4, 2, 4, 20, 1'5000, 'B');

    message = new char[headerSize + header4.payloadSize];
    std::memcpy(message, &header4, headerSize);
    std::memcpy(message + headerSize, &order4, header4.payloadSize);

    assert(!client->sendMessage(header4, message, true));
    std::cout << "PASSED!" << std::endl;

    std::cout << "TEST SHORT TRADE <N/A>" << std::endl;
    Header header5;
    Trade order5;
    helper_createTrade(header5, order5, 2, 4, -4, 1'5000);

    message = new char[headerSize + header5.payloadSize];
    std::memcpy(message, &header5, headerSize);
    std::memcpy(message + headerSize, &order5, header5.payloadSize);

    client->sendMessage(header5, message, false);
    std::cout << "PASSED!" << std::endl;
    
    std::cout << "TEST ORDER DELETE <N/A>" << std::endl;
    Header header6;
    DeleteOrder order6;
    helper_deleteOrder(header6, order6, 3);

    message = new char[headerSize + header6.payloadSize];
    std::memcpy(message, &header6, headerSize);
    std::memcpy(message + headerSize, &order6, header6.payloadSize);

    client->sendMessage(header6, message, false);
    std::cout << "PASSED!" << std::endl;


    // Test custom cases
    test_newOrderDuplicateId(client);
    test_modifyNonExistingOrder(client);

    return 0;
}
#ifndef POSITION_DATA_HPP
#define POSITION_DATA_HPP

#include <algorithm>
#include <stdlib.h>

struct Order
{
    char side;
    uint64_t orderId, financialInstrumentId, qty, price;

    Order() {}
    Order(uint64_t id, uint64_t instrument, uint64_t qty, uint64_t price, char side)
        : orderId(id), financialInstrumentId(instrument), qty(qty), price(price), side(side) {}
};

class PositionData
{
public:
    bool addPosition(std::shared_ptr<Order> order, uint64_t BUY_THRESHOLD, uint64_t SELL_THRESHOLD);
    uint64_t modifyPosition(std::shared_ptr<Order> order, uint64_t newQty, uint64_t BUY_THRESHOLD, uint64_t SELL_THRESHOLD);
    void rollbackPosition(std::shared_ptr<Order> order);
    void trade(int64_t tradeQty);

private:
    uint64_t instrument_id = 0, buyQty = 0, sellQty = 0;
    int64_t netPos = 0;
    uint64_t calcHypotheticalBuy() const;
    uint64_t calcHypotheticalSell() const;
};

#endif
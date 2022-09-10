#include "../include/risk_server/position_data.hpp"

/*
* Adds order to the position if risk within threshold.
*
* Parameters
* ----------
* order : std::shared_ptr<Order>
*     Pointer to the order to add to the position.
* BUY_THRESHOLD : uint64_t
*     The buy threshold.
* SELL_THRESHOLD
*     The sell threshold.
*
* Returns
* -------
* accepted : bool
*     true if newly calculated hypothetical buy or sell risk was within 
*     threshold, false otherwise.
*/
bool PositionData::addPosition(std::shared_ptr<Order> order, uint64_t BUY_THRESHOLD, uint64_t SELL_THRESHOLD)
{
    if (order->side == 'B')
    {
        buyQty += order->qty;
        if (calcHypotheticalBuy() > BUY_THRESHOLD)
        {
            buyQty -= order->qty;
            return false;
        }
    }
    else
    {
        sellQty += order->qty;
        if (calcHypotheticalSell() > SELL_THRESHOLD)
        {
            sellQty -= order->qty;
            return false;
        }
    }
    return true;
}

uint64_t PositionData::calcHypotheticalBuy() const
{
    return std::max(buyQty, netPos + buyQty);
}

uint64_t PositionData::calcHypotheticalSell() const
{
    return std::max(sellQty, sellQty - netPos);
}

/*
* Modifies order quantity if risk within threshold.
*
* Parameters
* ----------
* order : std::shared_ptr<Order>
*     Pointer to the order to modify the position.
* BUY_THRESHOLD : uint64_t
*     The buy threshold.
* SELL_THRESHOLD
*     The sell threshold.
*
* Returns
* -------
* accepted : bool
*     true if newly calculated hypothetical buy or sell risk was within 
*     threshold, false otherwise.
*/
uint64_t PositionData::modifyPosition(std::shared_ptr<Order> order, uint64_t newQty, uint64_t BUY_THRESHOLD, uint64_t SELL_THRESHOLD)
{
    int delta = newQty - order->qty;
    if (order->side == 'B')
    {
        buyQty += delta;
        if (calcHypotheticalBuy() > BUY_THRESHOLD)
        {
            buyQty -= delta;
            return false;
        }
    }
    else
    {
        sellQty += delta;
        if (calcHypotheticalSell() > SELL_THRESHOLD)
        {
            sellQty -= delta;
            return false;
        }
    }
    order->qty = newQty;
    return true;
}

/*
* Rollsback order and removes order from the position data.
*
* Parameters
* ----------
* order : std::shared_ptr<Order>
*     Pointer to the order to remove to the position.
*/
void PositionData::rollbackPosition(std::shared_ptr<Order> order)
{
    if (order->side == 'B')
    {
        buyQty -= order->qty;
    }
    else
    {
        sellQty -= order->qty;
    }
}

/*
* Performs trade and updates netPos.
*
* Parameters
* ----------
* tradeQty : int64_t
*     The quantity of the position to trade. (+ve for long, -ve for short)
*/
void PositionData::trade(int64_t tradeQty)
{
    netPos += tradeQty;
}
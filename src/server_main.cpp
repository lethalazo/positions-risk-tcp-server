#include "../include/risk_server/server.hpp"

/*
* Main runner code for risk server.
*
* Arguments
* ---------
*   BUY_THRESHOLD
*       uint64_t
*   SELL_THRESHOLD
*       uint64_t
*   PORT
*       uint64_t
*/
int main(int argc, char *argv[])
{
    uint64_t BUY_THRESHOLD, SELL_THRESHOLD;
    int PORT;
    if (argc >= 4)
    {
        BUY_THRESHOLD = std::atoi(argv[1]);
        SELL_THRESHOLD = std::atoi(argv[2]);
        PORT = std::atoi(argv[3]);
    }
    else
    {
        std::cerr << "Arguments not provided. Valid arguments: ... <buy_threshold> <sell_threshold> <port>" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::unique_ptr<RiskServer> server(new RiskServer(BUY_THRESHOLD, SELL_THRESHOLD, PORT));
    server->initListenerSocket();

    return 0;
}

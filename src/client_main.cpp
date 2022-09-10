#include "../include/risk_server/client.hpp"

/*
* Runner code for CLI client.
*
* Arguments
* ---------
*   PORT
*       uint64_t
*/
int main(int argc, char const *argv[])
{
    int PORT;
    if (argc >= 2)
    {
        PORT = std::atoi(argv[1]);
    }
    else
    {
        std::cerr << "Arguments not provided. Valid arguments: ... <port>" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::unique_ptr<RiskClient> client(new RiskClient(PORT));
    while (true)
    {
        client->runCLI();
    }
    return 0;
}
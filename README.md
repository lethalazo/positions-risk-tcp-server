This package contains headers, documented sources and tests for the standard risk TCP server and client written in C++, using standard library and sockets.

This code was tested after compiling the binary using g++ std C++17 (on POSIX/Mac OS) and, tests in tests/test_main.cpp were run successfully.

WIP:

1. Improve the build process using CMake.
2. Make the code more modern using templating and C++17/C++20 features.
3. Use concurrent connections to the server.
4. Handle more edge cases related to invalid data and invalid header sequence.

How to run:

1. Compile the risk server using g++ (`g++ -o server src/server_main.cpp src/server.cpp src/position_data.cpp -std=c++17`)
2. Compile the risk client using g++ (`g++ -o client src/client_main.cpp src/client.cpp -std=c++17`)
3. Run the server with arguments (e.g. `./server 20 15 51717` or `./server <buy_threshold> <sell_threshold> <port>`)
4. Run the client with arguments (e.g. `./client 51717` or `./client <port>`)

Now, to run tests:

1. Compile the tests using g++ (`g++ -o test tests/test_main.cpp src/client.cpp -std=c++17`)
2. Make sure server is running on port 51717 (PORT definition can be changed in tests/test_main.cpp)
3. Run the test without arguments (e.g. `./test`)

Folder descriptions:

- ./include
- /risk_server: Contains header files for server, client, message types, position data and any error/success strings used in the program.

  - client.hpp: Header file for the risk client.
  - message.hpp: Header file for the message types.
  - position_data.hpp: Header file for the position data class.
  - server.hpp: Header file for the risk server.
  - strings.hpp: Header file for the definitions of strings used in the program.

- ./src: Contains the source files for the server, client, position data. Also contains the main runner files.

  - client_main.cpp: Main runner code for the risk client (depends on client.cpp).
  - client.cpp: Source for the risk client.
  - position_data.cpp: Source for the position data class.
  - server_main.cpp: Main runner class for the risk server (depends on server.cpp and position_data.cpp).
  - server.cpp: Source for the risk server (depends on position_data.cpp).

- ./tests: Contains the test runner file for the risk server, sending messages using the risk client.

* test_main.cpp: Main source for the tests which covers multiple cases and edge cases (depends on linking client.cpp binary and the risk server running on port 51717)
# positions-risk-tcp-server

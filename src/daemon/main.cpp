#include <iostream>
#include <unistd.h>
#include <boost/asio/io_context.hpp>
#include "Daemon.h"

int main() {
    signal(SIGPIPE, SIG_IGN);

    try {
        boost::asio::io_context io_context;

        Daemon daemon(io_context);
        daemon.start();

        io_context.run();

        std::cout << "exiting" << "\n";
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}

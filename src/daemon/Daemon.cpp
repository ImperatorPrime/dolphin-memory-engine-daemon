#include "Daemon.h"

#include <algorithm>
#include <iostream>

using boost::asio::ip::tcp;

Daemon::Daemon(boost::asio::io_context &io_context)  : acceptor_(io_context), signals_(io_context, SIGINT, SIGTERM) {
}

void Daemon::start() {
    if (const uid_t effectiveUid = geteuid(); false && effectiveUid != 0) {
        std::cout << "This program must be run as root" << std::endl;
        return;
    }

    const tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1"), DAEMON_PORT);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    std::cout << "Listening on TCP 127.0.0.1:" << DAEMON_PORT << std::endl;

    acceptSignals();
    acceptData();

}

void Daemon::acceptData() {
    acceptor_.async_accept([this](const boost::system::error_code &error, tcp::socket socket) {
        if (!error) {
            std::erase_if(sessions_,
                          [](const std::weak_ptr<Session>& s) { return s.expired(); });
            const auto session = std::make_shared<Session>(std::move(socket));
            sessions_.push_back(session);
            session->startSession();
            acceptData();
        } else {
            std::cout << "\nStopping requeuing " << std::endl;
        }
    });
}

void Daemon::acceptSignals() {
    signals_.async_wait(
        [this](const boost::system::error_code error, int signal_number) {
            if (!error) {
                std::cout << "\nReceived signal: " << signal_number << std::endl;
                acceptor_.close();
                for (auto& weakSession : sessions_) {
                    if (const auto session = weakSession.lock()) {
                        session->close();
                    }
                }
            }
        });
}
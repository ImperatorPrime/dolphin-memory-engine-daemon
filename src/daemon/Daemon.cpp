#include "Daemon.h"

#include <algorithm>
#include <iostream>

using boost::asio::local::stream_protocol;

Daemon::Daemon(boost::asio::io_context &io_context)  : acceptor_(io_context), signals_(io_context, SIGINT, SIGTERM) {
}

void Daemon::start() {
    if (const uid_t effectiveUid = geteuid(); effectiveUid != 0) {
        std::cout << "This program must be run as root" << std::endl;
        return;
    }

    std::string socketName = "/tmp/dolphin_daemon.sock";
    std::remove(socketName.c_str());
    acceptor_.open(stream_protocol());

    const mode_t oldMask = umask(0011);
    acceptor_.bind(stream_protocol::endpoint(socketName));
    umask(oldMask);

    acceptor_.listen();

    acceptSignals();
    acceptData();

}

void Daemon::acceptData() {
    acceptor_.async_accept([this](const boost::system::error_code &error, stream_protocol::socket socket) {
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
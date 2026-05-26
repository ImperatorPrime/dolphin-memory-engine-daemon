#ifndef DOLPHIN_MEMORY_ENGINE_DAEMON_DAEMON_H
#define DOLPHIN_MEMORY_ENGINE_DAEMON_DAEMON_H

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/signal_set.hpp>
#include <memory>
#include <vector>

#include "Session.h"

class Daemon {
public:
    explicit Daemon(boost::asio::io_context& io_context);
    void start();

private:
    void acceptData();
    void acceptSignals();

    boost::asio::local::stream_protocol::acceptor acceptor_;
    boost::asio::signal_set signals_;
    std::vector<std::weak_ptr<Session>> sessions_;

};

#endif //DOLPHIN_MEMORY_ENGINE_DAEMON_DAEMON_H

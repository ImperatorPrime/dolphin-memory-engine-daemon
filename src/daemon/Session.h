#ifndef DOLPHIN_MEMORY_ENGINE_DAEMON_SESSION_H
#define DOLPHIN_MEMORY_ENGINE_DAEMON_SESSION_H

#include <boost/asio/ip/tcp.hpp>
#include <optional>
#include <vector>

constexpr uint32_t API_VERSION = 1;
constexpr uint32_t MAX_INPUT_BYTES = 100;
constexpr uint32_t MAX_ABSOLUTE_ADDRESSES = 8;
constexpr uint32_t MAX_OUTPUT_BYTES = 100;

// Wire layout: [type(1)] [operationsCount(1)] [absolute_addresses_count(1)] [keep_alive(1)]
//              [addresses(MAX_ABSOLUTE_ADDRESSES × 4)] [data(MAX_INPUT_BYTES)]
constexpr size_t REQUEST_PACKET_MAX_SIZE = 4 + MAX_ABSOLUTE_ADDRESSES * sizeof(uint32_t) + MAX_INPUT_BYTES;

struct RequestPacket {
    enum class RequestType : uint8_t {
        MemoryOperations,
        ApiInfo
    };
    RequestType type;
    uint8_t operationsCount;
    uint8_t absolute_addresses_count;
    uint8_t keep_alive;
    std::vector<uint32_t> addresses;
    uint8_t data[MAX_INPUT_BYTES];
};

struct MemoryOperation {
    bool hasRead: 1;
    bool hasWrite: 1;
    bool isWord: 1;
    bool hasOffset: 1;
    uint8_t addressIndex: 4;

    uint8_t byteCount;
    int16_t offset;
    std::vector<uint8_t> writeBytes;
};

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket sock);

    void startSession();
    void close();

private:
    void readRequest();
    void processMemoryOperations(const RequestPacket& requestPacket);

    static std::optional<RequestPacket> parseRequestPacket(const char* data, std::size_t length);
    static MemoryOperation parseMemoryOperation(const uint8_t* data);

    void writeAPIInfoResponse();

    void writeMemoryOperationResponse(const char* output, size_t length);

    boost::asio::ip::tcp::socket socket_;
};


#endif //DOLPHIN_MEMORY_ENGINE_DAEMON_SESSION_H

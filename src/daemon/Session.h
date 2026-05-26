#ifndef DOLPHIN_MEMORY_ENGINE_DAEMON_SESSION_H
#define DOLPHIN_MEMORY_ENGINE_DAEMON_SESSION_H

#include <boost/asio/local/stream_protocol.hpp>
#include <vector>

constexpr uint32_t API_VERSION = 1;
constexpr uint32_t MAX_INPUT_BYTES = 100;
constexpr uint32_t MAX_ABSOLUTE_ADDRESSES = 8;
constexpr uint32_t MAX_OUTPUT_BYTES = 100;

struct RequestPacket {
    enum class RequestType : uint8_t {
        ApiInfo = 1,
        DolphinStatus = 2,
        MemoryOperations = 3
    };
    RequestType type;
    uint8_t operationsCount;
    uint8_t data[MAX_INPUT_BYTES];
};

struct MemoryOperation {
    bool hasRead: 1;
    bool hasWrite: 1;
    bool hasOffset: 1;
    uint8_t : 5;
    uint32_t offset;
    uint32_t address;
    //This is nominally size_t, but 32 should be safe to upcast
    uint32_t byteCount;
    std::vector<uint8_t> writeBytes;
};

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::local::stream_protocol::socket sock);

    void startSession();
    void close();

private:
    void readRequest();
    void processMemoryOperations(const RequestPacket& requestPacket);

    static uint32_t getDolphinStatusAsInteger();
    static MemoryOperation parseMemoryOperation(const uint8_t* data);

    void writeAPIInfoResponse();
    void writeDolphinStatusResponse();
    void writeMemoryOperationResponse(uint32_t dolphinStatus, const char* output, size_t length);

    boost::asio::local::stream_protocol::socket socket_;
};


#endif //DOLPHIN_MEMORY_ENGINE_DAEMON_SESSION_H

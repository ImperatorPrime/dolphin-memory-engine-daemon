#include "Session.h"

#include <boost/asio/write.hpp>
#include <boost/endian/conversion.hpp>
#include <iostream>
#include <vector>

#include "DolphinProcess/DolphinAccessor.h"

using boost::asio::local::stream_protocol;

Session::Session(stream_protocol::socket sock) : socket_(std::move(sock)) {}

void Session::startSession() { readRequest(); }

void Session::close() { socket_.close(); }

void Session::readRequest() {
    // Create self as a shared point to ensure that the session doesn't go out of scope before the buffer has been
    // processed
    auto self{shared_from_this()};
    auto readBuffer = std::make_shared<std::array<char, sizeof(RequestPacket)>>();
    socket_.async_read_some(boost::asio::buffer(*readBuffer), [this, readBuffer, self](
                                                                  const boost::system::error_code& error,
                                                                  std::size_t length) {
        if (!error) {
            DolphinComm::DolphinAccessor::hook();
            RequestPacket requestPacket{};
            std::memcpy(&requestPacket, readBuffer.get(), sizeof(RequestPacket));
            switch (requestPacket.type) {
                case RequestPacket::RequestType::ApiInfo:
                    writeAPIInfoResponse();
                    break;
                case RequestPacket::RequestType::DolphinStatus:
                    writeDolphinStatusResponse();
                    break;
                case RequestPacket::RequestType::MemoryOperations:
                    processMemoryOperations(requestPacket);
                    break;
                default:
                    break;
            }
        } else {
            std::cout << "Error encountered" << error << std::endl;
        }
    });
}

void Session::processMemoryOperations(const RequestPacket& requestPacket) {
    std::vector<char> memoryOperationResponse;
    DolphinComm::DolphinAccessor::DolphinStatus dolphinStatus =
        DolphinComm::DolphinAccessor::getStatus();
    bool allOperationsSuccessful = true;
    if (dolphinStatus == DolphinComm::DolphinAccessor::DolphinStatus::hooked) {
        uint8_t byteOffset = 0;
        for (int i = 0; i < requestPacket.operationsCount; i++) {
            const MemoryOperation op = parseMemoryOperation(&requestPacket.data[byteOffset]);

            uint32_t effectiveAddress = op.address;
            if (op.hasOffset) {
                char memoryBuffer[4];
                allOperationsSuccessful &= DolphinComm::DolphinAccessor::readFromRAM(
                    Common::dolphinAddrToOffset(op.address,
                                                DolphinComm::DolphinAccessor::isARAMAccessible()),
                    memoryBuffer, 4, true);

                auto* offsetAddress = reinterpret_cast<uint32_t*>(memoryBuffer);
                effectiveAddress = offsetAddress[0] + op.offset;
            }

            std::cout << "Using Address " << std::hex << effectiveAddress << std::endl;

            if (op.hasRead) {
                std::cout << "Got Dolphin Read Request" << std::endl;
                const size_t oldSize = memoryOperationResponse.size();
                memoryOperationResponse.resize(oldSize + op.byteCount);
                allOperationsSuccessful &= DolphinComm::DolphinAccessor::readFromRAM(
                    Common::dolphinAddrToOffset(effectiveAddress,
                                                DolphinComm::DolphinAccessor::isARAMAccessible()),
                    memoryOperationResponse.data() + oldSize, op.byteCount, false);

            } else if (op.hasWrite) {
                std::cout << "Got Dolphin Write Request" << std::endl;
                allOperationsSuccessful &= DolphinComm::DolphinAccessor::writeToRAM(
                    Common::dolphinAddrToOffset(effectiveAddress,
                                                DolphinComm::DolphinAccessor::isARAMAccessible()),
                    reinterpret_cast<const char*>(op.writeBytes.data()), op.byteCount, false);

                byteOffset += op.byteCount;
            }

            byteOffset += 1 + 3 * sizeof(uint32_t);
        }
    }

    //Get the current status to send in the response
    uint32_t dolphinStatusAsInteger = getDolphinStatusAsInteger();
    //If anything was not successful and the connection status is currently hooked, set the status to 3 (Unhooked)
    if (!allOperationsSuccessful && dolphinStatusAsInteger == 0) {
        dolphinStatusAsInteger = 3;
    }

    writeMemoryOperationResponse(dolphinStatusAsInteger, memoryOperationResponse.data(), memoryOperationResponse.size());
}

uint32_t Session::getDolphinStatusAsInteger() {
    DolphinComm::DolphinAccessor::DolphinStatus dolphinStatus = DolphinComm::DolphinAccessor::getStatus();
    return static_cast<uint32_t>(dolphinStatus);
}

MemoryOperation Session::parseMemoryOperation(const uint8_t* data) {
    MemoryOperation op;
    op.hasRead = data[0] & 0x80;
    op.hasWrite = data[0] & 0x40;
    op.hasOffset = data[0] & 0x20;

    uint32_t bigEndianOffset;
    uint32_t bigEndianAddress;
    uint32_t bigEndianByteCount;
    std::memcpy(&bigEndianOffset, &data[1], sizeof(uint32_t));
    std::memcpy(&bigEndianAddress, &data[1 + 1 * sizeof(uint32_t)], sizeof(uint32_t));
    std::memcpy(&bigEndianByteCount, &data[1 + 2 * sizeof(uint32_t)], sizeof(uint32_t));

    op.offset = boost::endian::big_to_native(bigEndianOffset);
    op.address = boost::endian::big_to_native(bigEndianAddress);
    op.byteCount = boost::endian::big_to_native(bigEndianByteCount);

    if (op.hasWrite) {
        const uint8_t* writeStart = &data[1 + 3 * sizeof(uint32_t)];
        op.writeBytes.assign(writeStart, writeStart + op.byteCount);
    }
    return op;
}

void Session::writeAPIInfoResponse() {
    constexpr uint32_t apiVersion = boost::endian::native_to_big(API_VERSION);
    constexpr uint32_t maxInput = boost::endian::native_to_big(MAX_INPUT_BYTES);
    constexpr uint32_t maxOutput = boost::endian::native_to_big(MAX_OUTPUT_BYTES);
    constexpr uint32_t maxAddresses = boost::endian::native_to_big(MAX_ABSOLUTE_ADDRESSES);

    const uint32_t dolphinStatus = getDolphinStatusAsInteger();
    const uint32_t bigEndianDolphinStatus = boost::endian::native_to_big(dolphinStatus);

    auto writeBuffer = std::make_shared<std::array<char, 5 * sizeof(uint32_t)>>();
    std::memcpy(writeBuffer.get()->data() + 0 * sizeof(uint32_t), &apiVersion, sizeof(uint32_t));
    std::memcpy(writeBuffer.get()->data() + 1 * sizeof(uint32_t), &maxInput, sizeof(uint32_t));
    std::memcpy(writeBuffer.get()->data() + 2 * sizeof(uint32_t), &maxOutput, sizeof(uint32_t));
    std::memcpy(writeBuffer.get()->data() + 3 * sizeof(uint32_t), &maxAddresses, sizeof(uint32_t));
    std::memcpy(writeBuffer.get()->data() + 4 * sizeof(uint32_t), &bigEndianDolphinStatus, sizeof(uint32_t));

    // Create self as a shared point to ensure that the session doesn't go out of scope before the buffer has been
    // processed
    auto self{shared_from_this()};
    boost::asio::async_write(socket_, boost::asio::buffer(*writeBuffer, sizeof(*writeBuffer)),
                             [this, writeBuffer, self](boost::system::error_code ec, std::size_t length) {
                                 std::cout << "Writing Data " << ec.message() << std::endl;
                                 if (!ec) {
                                     readRequest();
                                 }
                             });
}

void Session::writeDolphinStatusResponse() {
    const uint32_t dolphinStatus = getDolphinStatusAsInteger();
    const uint32_t bigEndianDolphinStatus = boost::endian::native_to_big(dolphinStatus);
    // Create self as a shared point to ensure that the session doesn't go out of scope before the buffer has been
    // processed
    auto self{shared_from_this()};
    auto writeBuffer = std::make_shared<std::array<char, 100>>();
    std::memcpy(writeBuffer.get()->data(), &bigEndianDolphinStatus, sizeof(uint32_t));
    boost::asio::async_write(socket_, boost::asio::buffer(*writeBuffer, sizeof(*writeBuffer)),
                             [this, writeBuffer, self](boost::system::error_code ec, std::size_t length) {
                                 std::cout << "Writing Data " << ec.message() << std::endl;
                                 if (!ec) {
                                     readRequest();
                                 }
                             });
}

void Session::writeMemoryOperationResponse(uint32_t dolphinStatus, const char* output, size_t length) {
    // Create self as a shared point to ensure that the session doesn't go out of scope before the buffer has been
    // processed
    auto self{shared_from_this()};

    uint32_t bigEndianDolphinStatus = boost::endian::native_to_big(dolphinStatus);
    auto writeBuffer = std::make_shared<std::array<char, 100>>();
    std::memcpy(writeBuffer.get()->data(), &bigEndianDolphinStatus, sizeof(uint32_t));
    std::memcpy(writeBuffer.get()->data() + sizeof(uint32_t), output, sizeof(char) * length);
    boost::asio::async_write(
        socket_, boost::asio::buffer(*writeBuffer, sizeof(*writeBuffer)),
        [this, writeBuffer, self, dolphinStatus](boost::system::error_code ec, std::size_t length) {
            std::cout << "Writing Data " << ec.message() << std::endl;
            // Only listen for more if there was no error and dolphin is hooked
            if (!ec && dolphinStatus == 0) {
                readRequest();
            }
        });
}

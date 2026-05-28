#include "Session.h"

#include <boost/asio/write.hpp>
#include <boost/endian/conversion.hpp>
#include <iostream>
#include <vector>

#include "DolphinProcess/DolphinAccessor.h"

using boost::asio::ip::tcp;

Session::Session(tcp::socket sock) : socket_(std::move(sock)) {}

void Session::startSession() { readRequest(); }

void Session::close() { socket_.close(); }

void Session::readRequest() {
    // Create self as a shared point to ensure that the session doesn't go out of scope before the buffer has been
    // processed
    auto self{shared_from_this()};
    auto readBuffer = std::make_shared<std::array<char, REQUEST_PACKET_MAX_SIZE>>();
    socket_.async_read_some(boost::asio::buffer(*readBuffer), [this, readBuffer, self](
                                                                  const boost::system::error_code& error,
                                                                  std::size_t length) {
        if (!error) {
            DolphinComm::DolphinAccessor::hook();
            const auto requestPacket = parseRequestPacket(readBuffer->data(), length);

            if (!requestPacket) return;

            switch (requestPacket->type) {
                case RequestPacket::RequestType::ApiInfo:
                    writeAPIInfoResponse();
                    break;
                case RequestPacket::RequestType::MemoryOperations:
                    processMemoryOperations(*requestPacket);
                    break;
                default:
                    break;
            }
        } else {
            std::cout << "Error encountered " << error << std::endl;
        }
    });
}

void Session::processMemoryOperations(const RequestPacket& requestPacket) {
    std::vector<char> memoryOperationResponse;
    DolphinComm::DolphinAccessor::DolphinStatus currentDolphinStatus =
        DolphinComm::DolphinAccessor::getStatus();

    if (currentDolphinStatus == DolphinComm::DolphinAccessor::DolphinStatus::hooked) {

        const uint8_t successBytesCount = 1 + (requestPacket.operationsCount - 1) / 8;
        std::vector<uint8_t> successBytes(successBytesCount, 0);
        memoryOperationResponse.resize(successBytesCount, 0);
        uint8_t byteOffset = 0;
        for (int i = 0; i < requestPacket.operationsCount; i++) {
            const MemoryOperation op = parseMemoryOperation(&requestPacket.data[byteOffset]);

            bool operationSuccessful = false;
            const uint32_t baseAddress = requestPacket.addresses.at(op.addressIndex);
            uint32_t effectiveAddress = baseAddress;
            if (op.hasOffset) {
                char memoryBuffer[4];
                operationSuccessful = DolphinComm::DolphinAccessor::readFromRAM(
                    Common::dolphinAddrToOffset(baseAddress,
                                                DolphinComm::DolphinAccessor::isARAMAccessible()),
                    memoryBuffer, 4, true);

                const auto* offsetAddress = reinterpret_cast<uint32_t*>(memoryBuffer);
                effectiveAddress = offsetAddress[0] + op.offset;
            }

            std::cout << "Using Address " << std::hex << effectiveAddress << std::endl;


            if (op.hasRead) {
                std::cout << "Got Dolphin Read Request" << std::endl;
                const size_t oldSize = memoryOperationResponse.size();
                memoryOperationResponse.resize(oldSize + op.byteCount);
                operationSuccessful = DolphinComm::DolphinAccessor::readFromRAM(
                    Common::dolphinAddrToOffset(effectiveAddress,
                                                DolphinComm::DolphinAccessor::isARAMAccessible()),
                    memoryOperationResponse.data() + oldSize, op.byteCount, false);

            } else if (op.hasWrite) {
                std::cout << "Got Dolphin Write Request" << std::endl;
                operationSuccessful = DolphinComm::DolphinAccessor::writeToRAM(
                    Common::dolphinAddrToOffset(effectiveAddress,
                                                DolphinComm::DolphinAccessor::isARAMAccessible()),
                    reinterpret_cast<const char*>(op.writeBytes.data()), op.byteCount, false);
            }

            if (operationSuccessful) {
                successBytes[i / 8] |= (1 << (i % 8));
            }

            // flags byte
            byteOffset += 1;
            // byteCount byte
            if (!op.isWord)   byteOffset += 1;
            // offset field
            if (op.hasOffset) byteOffset += sizeof(int16_t);
            // write payload
            if (op.hasWrite)  byteOffset += op.byteCount;
        }

        std::memcpy(memoryOperationResponse.data(), successBytes.data(), successBytesCount);
    }

    currentDolphinStatus = DolphinComm::DolphinAccessor::getStatus();
    //If Dolphin isn't hooked into the daemon, close the connection
    if (currentDolphinStatus != DolphinComm::DolphinAccessor::DolphinStatus::hooked) {
        return;
    }

    writeMemoryOperationResponse(memoryOperationResponse.data(), memoryOperationResponse.size());
}


std::optional<RequestPacket> Session::parseRequestPacket(const char* data, std::size_t length) {
    if (length < 4) {
        return std::nullopt;
    }

    RequestPacket packet{};
    std::size_t offset = 0;

    packet.type = static_cast<RequestPacket::RequestType>(data[offset++]);
    packet.operationsCount = data[offset++];
    packet.absolute_addresses_count = data[offset++];
    packet.keep_alive = data[offset++];

    const std::size_t addressBytes = packet.absolute_addresses_count * sizeof(uint32_t);
    if (4 + addressBytes > length)
        return std::nullopt;

    packet.addresses.resize(packet.absolute_addresses_count);
    for (uint8_t i = 0; i < packet.absolute_addresses_count; i++) {
        uint32_t address;
        std::memcpy(&address, data + offset, sizeof(uint32_t));
        packet.addresses[i] = boost::endian::big_to_native(address);
        offset += sizeof(uint32_t);
    }

    const std::size_t dataBytes = length - offset;
    std::memcpy(packet.data, data + offset, std::min(dataBytes, static_cast<std::size_t>(MAX_INPUT_BYTES)));

    return packet;
}

MemoryOperation Session::parseMemoryOperation(const uint8_t* data) {
    MemoryOperation op{};
    uint8_t readOffset = 0;

    op.hasRead = data[readOffset] & 0x80;
    op.hasWrite = data[readOffset] & 0x40;
    op.isWord = data[readOffset] & 0x20;
    op.hasOffset = data[readOffset] & 0x10;
    op.addressIndex = data[readOffset] & 0x0F;
    readOffset++;

    if (op.isWord) {
        op.byteCount = 4;
    } else {
        op.byteCount = data[readOffset++];
    }

    if (op.hasOffset) {
        int16_t bigEndianOffset;
        std::memcpy(&bigEndianOffset, &data[readOffset], sizeof(int16_t));
        op.offset = boost::endian::big_to_native(bigEndianOffset);

        readOffset += sizeof(int16_t);
    }

    if (op.hasWrite) {
        const uint8_t* writeStart = &data[readOffset];
        op.writeBytes.assign(writeStart, writeStart + op.byteCount);
    }
    return op;
}

void Session::writeAPIInfoResponse() {
    constexpr uint32_t apiVersion = boost::endian::native_to_big(API_VERSION);
    constexpr uint32_t maxInput = boost::endian::native_to_big(MAX_INPUT_BYTES);
    constexpr uint32_t maxOutput = boost::endian::native_to_big(MAX_OUTPUT_BYTES);
    constexpr uint32_t maxAddresses = boost::endian::native_to_big(MAX_ABSOLUTE_ADDRESSES);

    auto writeBuffer = std::make_shared<std::array<char, 4 * sizeof(uint32_t)>>();
    std::memcpy(writeBuffer.get()->data() + 0 * sizeof(uint32_t), &apiVersion, sizeof(uint32_t));
    std::memcpy(writeBuffer.get()->data() + 1 * sizeof(uint32_t), &maxInput, sizeof(uint32_t));
    std::memcpy(writeBuffer.get()->data() + 2 * sizeof(uint32_t), &maxOutput, sizeof(uint32_t));
    std::memcpy(writeBuffer.get()->data() + 3 * sizeof(uint32_t), &maxAddresses, sizeof(uint32_t));


    DolphinComm::DolphinAccessor::DolphinStatus dolphinStatus =
    DolphinComm::DolphinAccessor::getStatus();
    if (dolphinStatus != DolphinComm::DolphinAccessor::DolphinStatus::hooked) {
        //If Dolphin isn't hooked into the daemon, close the connection
        return;
    }

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

void Session::writeMemoryOperationResponse(const char* output, size_t length) {
    // Create self as a shared point to ensure that the session doesn't go out of scope before the buffer has been
    // processed
    auto self{shared_from_this()};

    auto writeBuffer = std::make_shared<std::array<char, MAX_OUTPUT_BYTES>>();
    std::memcpy(writeBuffer.get()->data(), output, sizeof(char) * length);
    boost::asio::async_write(
        socket_, boost::asio::buffer(*writeBuffer, sizeof(*writeBuffer)),
        [this, writeBuffer, self](boost::system::error_code ec, std::size_t bytesWritten) {
            std::cout << "Writing Data " << ec.message() << std::endl;
            // Only listen for more if there was no error and dolphin is hooked
            if (!ec) {
                readRequest();
            }
        });
}

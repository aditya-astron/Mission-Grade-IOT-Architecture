#pragma once

#include "habitat/types.h"

#include <cstddef>
#include <cstdint>

namespace habitat {

constexpr size_t kFrameMaxPayload = 200;
constexpr size_t kFrameMaxBytes = 8 + kFrameMaxPayload + 2;

struct Frame {
    uint8_t version;
    NodeId node;
    MsgType type;
    uint8_t seq;
    uint16_t payload_len;
    const uint8_t* payload;
};

// On-wire: A5 5A | ver | node | type | seq | len_hi len_lo | payload | crc16
// CRC-16/CCITT-FALSE covers bytes after the two SOF markers.
size_t encode_frame(uint8_t* out, size_t out_cap, const Frame& frame);

class FrameDecoder {
  public:
    void reset();
    bool feed(uint8_t byte, Frame* out, uint8_t* payload_buf, size_t payload_cap);

  private:
    enum class State : uint8_t { Sof0, Sof1, Header, Payload, Crc0, Crc1 };
    State state_ = State::Sof0;
    uint8_t header_[6]{};
    size_t header_n_ = 0;
    uint8_t payload_[kFrameMaxPayload]{};
    size_t payload_n_ = 0;
    uint16_t payload_len_ = 0;
    uint8_t crc_hi_ = 0;
};

} // namespace habitat

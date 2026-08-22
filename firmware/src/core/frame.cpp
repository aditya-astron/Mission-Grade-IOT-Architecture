#include "habitat/frame.h"

#include "habitat/config.h"
#include "habitat/crc.h"

#include <cstring>

namespace habitat {

size_t encode_frame(uint8_t* out, size_t out_cap, const Frame& frame) {
    if (frame.payload_len > kFrameMaxPayload) {
        return 0;
    }
    const size_t total = 2 + 6 + frame.payload_len + 2;
    if (out == nullptr || out_cap < total) {
        return 0;
    }
    out[0] = config::kSof0;
    out[1] = config::kSof1;
    out[2] = frame.version;
    out[3] = static_cast<uint8_t>(frame.node);
    out[4] = static_cast<uint8_t>(frame.type);
    out[5] = frame.seq;
    out[6] = static_cast<uint8_t>((frame.payload_len >> 8) & 0xFF);
    out[7] = static_cast<uint8_t>(frame.payload_len & 0xFF);
    if (frame.payload_len > 0 && frame.payload != nullptr) {
        std::memcpy(out + 8, frame.payload, frame.payload_len);
    }
    const uint16_t crc = crc16_ccitt(out + 2, 6 + frame.payload_len);
    out[8 + frame.payload_len] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    out[9 + frame.payload_len] = static_cast<uint8_t>(crc & 0xFF);
    return total;
}

void FrameDecoder::reset() {
    state_ = State::Sof0;
    header_n_ = 0;
    payload_n_ = 0;
    payload_len_ = 0;
}

bool FrameDecoder::feed(uint8_t byte, Frame* out, uint8_t* payload_buf, size_t payload_cap) {
    switch (state_) {
    case State::Sof0:
        if (byte == config::kSof0) {
            state_ = State::Sof1;
        }
        return false;
    case State::Sof1:
        if (byte == config::kSof1) {
            state_ = State::Header;
            header_n_ = 0;
        } else if (byte != config::kSof0) {
            state_ = State::Sof0;
        }
        return false;
    case State::Header:
        header_[header_n_++] = byte;
        if (header_n_ < 6) {
            return false;
        }
        payload_len_ = static_cast<uint16_t>((header_[4] << 8) | header_[5]);
        if (payload_len_ > kFrameMaxPayload) {
            reset();
            return false;
        }
        payload_n_ = 0;
        state_ = (payload_len_ == 0) ? State::Crc0 : State::Payload;
        return false;
    case State::Payload:
        payload_[payload_n_++] = byte;
        if (payload_n_ >= payload_len_) {
            state_ = State::Crc0;
        }
        return false;
    case State::Crc0:
        crc_hi_ = byte;
        state_ = State::Crc1;
        return false;
    case State::Crc1: {
        const uint16_t recv = static_cast<uint16_t>((crc_hi_ << 8) | byte);
        uint8_t crc_src[6 + kFrameMaxPayload];
        std::memcpy(crc_src, header_, 6);
        if (payload_len_ > 0) {
            std::memcpy(crc_src + 6, payload_, payload_len_);
        }
        const uint16_t calc = crc16_ccitt(crc_src, 6 + payload_len_);
        const uint16_t plen = payload_len_;
        uint8_t hdr[6];
        std::memcpy(hdr, header_, 6);
        uint8_t pay[kFrameMaxPayload];
        if (plen > 0) {
            std::memcpy(pay, payload_, plen);
        }
        reset();
        if (calc != recv || out == nullptr) {
            return false;
        }
        if (plen > payload_cap) {
            return false;
        }
        if (plen > 0 && payload_buf != nullptr) {
            std::memcpy(payload_buf, pay, plen);
        }
        out->version = hdr[0];
        out->node = static_cast<NodeId>(hdr[1]);
        out->type = static_cast<MsgType>(hdr[2]);
        out->seq = hdr[3];
        out->payload_len = plen;
        out->payload = payload_buf;
        return true;
    }
    }
    reset();
    return false;
}

} // namespace habitat

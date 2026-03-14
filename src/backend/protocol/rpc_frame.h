/*
 * rpc_frame.h — RPC Frame encoder/decoder
 *
 * Builds outgoing frames and parses incoming byte streams into frames
 * using a state machine. Thread-safe for single-producer/single-consumer.
 */

#ifndef RPC_FRAME_H
#define RPC_FRAME_H

#include <QByteArray>
#include <QObject>
#include <cstdint>

namespace rpc {

/**
 * Decoded RPC frame.
 */
struct Frame {
    uint8_t    cmd = 0;
    uint8_t    seq = 0;
    QByteArray payload;

    bool isValid() const { return cmd != 0; }
};

/**
 * RPC frame codec.
 *
 * Encoding:  buildFrame() creates a complete wire-format frame.
 * Decoding:  feed() processes incoming bytes; emits frameReady() for each
 *            valid frame decoded.
 */
class FrameCodec : public QObject {
    Q_OBJECT

public:
    explicit FrameCodec(QObject *parent = nullptr);

    /**
     * Build a wire-format frame ready for transmission.
     * @param cmd     Command ID
     * @param seq     Sequence number
     * @param payload Payload data (may be empty)
     * @return        Complete frame bytes including sync, header, payload, CRC
     */
    static QByteArray buildFrame(uint8_t cmd, uint8_t seq,
                                  const QByteArray &payload = {});

    /**
     * Build a frame with raw payload pointer.
     */
    static QByteArray buildFrame(uint8_t cmd, uint8_t seq,
                                  const uint8_t *payload, uint16_t len);

    /**
     * Feed incoming bytes into the parser.
     * Emits frameReady() for each complete, CRC-valid frame found.
     */
    void feed(const QByteArray &data);

    /**
     * Reset the parser state (e.g., after disconnect).
     */
    void reset();

    /**
     * Get and auto-increment the sequence counter.
     */
    uint8_t nextSeq();

signals:
    /**
     * Emitted when a complete, valid frame has been decoded.
     */
    void frameReady(rpc::Frame frame);

    /**
     * Emitted when a frame fails CRC check.
     */
    void frameCrcError(uint8_t cmd, uint8_t seq);

private:
    enum class State {
        WaitSync,
        ReadHeader,
        ReadPayload,
        ReadCrc
    };

    State      m_state = State::WaitSync;
    QByteArray m_headerBuf;    // accumulates CMD + SEQ + LEN (4 bytes)
    QByteArray m_payloadBuf;
    QByteArray m_crcBuf;
    uint8_t    m_cmd = 0;
    uint8_t    m_seq = 0;
    uint16_t   m_payloadLen = 0;
    uint8_t    m_seqCounter = 0;

    void processByte(uint8_t byte);
    void finalizeFrame();
};

} // namespace rpc

#endif // RPC_FRAME_H

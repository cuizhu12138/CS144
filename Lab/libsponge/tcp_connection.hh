#ifndef SPONGE_LIBSPONGE_TCP_FACTORED_HH
#define SPONGE_LIBSPONGE_TCP_FACTORED_HH

#include "tcp_config.hh"
#include "tcp_receiver.hh"
#include "tcp_sender.hh"
#include "tcp_state.hh"

//! \brief A complete endpoint of a TCP connection
class TCPConnection {
  private:
    TCPConfig _cfg;
    TCPReceiver _receiver{_cfg.recv_capacity};
    TCPSender _sender{_cfg.send_capacity, _cfg.rt_timeout, _cfg.fixed_isn};

    //! outbound queue of segments that the TCPConnection wants sent
    std::queue<TCPSegment> _segments_out{};

    //! Should the TCPConnection stay active (and keep ACKing)
    //! for 10 * _cfg.rt_timeout milliseconds after both streams have ended,
    //! in case the remote TCPConnection doesn't know we've received its whole stream?
    bool _linger_after_streams_finish{true};

    // 最后一次收到报文的时间
    size_t LastSegmentReceived{0};

    // 标志着连接是否存活
    bool Alive{true};

    // linger计时器
    size_t LingerTimer{0};

    // 输入流是否关闭
    bool InComeStream{false};

    // 输出流是否关闭
    bool OutComeStream{false};

    // 是否开始逗留
    bool LingerBegin{false};

    // 是否发送过SYN
    bool SYNSET{false};

  public:
    //! \name "Input" interface for the writer
    //!@{

    //! \brief Initiate a connection by sending a SYN segment
    void connect();

    //! \brief Write data to the outbound byte stream, and send it over TCP if possible
    //! \returns the number of bytes from `data` that were actually written.
    size_t write(const std::string &data);

    //! \returns the number of `bytes` that can be written right now.
    size_t remaining_outbound_capacity() const;

    //! \brief Shut down the outbound byte stream (still allows reading incoming data)
    void end_input_stream();
    //!@}

    //! \name "Output" interface for the reader
    //!@{

    //! \brief The inbound byte stream received from the peer
    ByteStream &inbound_stream() { return _receiver.stream_out(); }
    //!@}

    //! \name Accessors used for testing

    //!@{
    //! \brief number of bytes sent and not yet acknowledged, counting SYN/FIN each as one byte
    size_t bytes_in_flight() const;
    //! \brief number of bytes not yet reassembled
    size_t unassembled_bytes() const;
    //! \brief Number of milliseconds since the last segment was received
    size_t time_since_last_segment_received() const;
    //!< \brief summarize the state of the sender, receiver, and the connection
    TCPState state() const { return {_sender, _receiver, active(), _linger_after_streams_finish}; };
    //!@}

    //! \name Methods for the owner or operating system to call
    //!@{

    //! Called when a new segment has been received from the network
    void segment_received(const TCPSegment &seg);

    //! Called periodically when time elapses
    void tick(const size_t ms_since_last_tick);

    //! \brief TCPSegments that the TCPConnection has enqueued for transmission.
    //! \note The owner or operating system will dequeue these and
    //! put each one into the payload of a lower-layer datagram (usually Internet datagrams (IP),
    //! but could also be user datagrams (UDP) or any other kind).
    std::queue<TCPSegment> &segments_out() { return _segments_out; }

    //! \brief Is the connection still alive in any way?
    //! \returns `true` if either stream is still running or if the TCPConnection is lingering
    //! after both streams have finished (e.g. to ACK retransmissions from the peer)
    bool active() const;
    //!@}

    //! Construct a new connection from a configuration
    explicit TCPConnection(const TCPConfig &cfg) : _cfg{cfg} {}

    //! \name construction and destruction
    //! moving is allowed; copying is disallowed; default construction not possible

    //!@{
    ~TCPConnection();  //!< destructor sends a RST if the connection is still open
    TCPConnection() = delete;
    TCPConnection(TCPConnection &&other) = default;
    TCPConnection &operator=(TCPConnection &&other) = default;
    TCPConnection(const TCPConnection &other) = delete;
    TCPConnection &operator=(const TCPConnection &other) = delete;
    //!@}

    //!@{
    //! \brief 切断连接
    void SendRSTSegment() {
        // sender制造一个空报文
        _sender.send_empty_segment();

        // 取出空报文，把RST改true
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        seg.header().rst = true;
        FillWithACK(seg);
        _segments_out.push(seg);
    }
    void CutConnection() {
        _receiver.stream_out().set_error();
        _sender.stream_in().set_error();
        Alive = false;
    }
    //!@}

    //!@{

    void FillWithACK(TCPSegment &seg) {
        if (!_receiver.ackno().has_value())
            return;
        seg.header().ack = true;
        seg.header().ackno = _receiver.ackno().value();
        seg.header().win = _receiver.window_size();
    }

    void SenderFillWindow() {
        _sender.fill_window();
        // 直接从sender发送队列里拿要的报文
        TCPSegment seg;
        while (!_sender.segments_out().empty()) {
            seg = _sender.segments_out().front();
            _sender.segments_out().pop();
            FillWithACK(seg);
            _segments_out.push(seg);
        }
    }
    void SenderReflectAcknoWin() {
        _sender.send_empty_segment();
        // 取出空报文，把RST改true
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        FillWithACK(seg);
        _segments_out.push(seg);
    }
    //!@}

    bool check_inbound_ended();
};

#endif  // SPONGE_LIBSPONGE_TCP_FACTORED_HH

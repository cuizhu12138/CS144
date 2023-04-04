#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>

using namespace std;

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPTimer {
  private:
    uint64_t RTO;
    // 定义还剩多少时间
    int RuningTime = 0;
    // 标记Timer是否在运行 ture在运行
    bool _IsItRuning = false;

  public:
    TCPTimer(const uint16_t OrignRTO) : RTO(OrignRTO) {}
    //!@{ 计时器的开始与暂停

    // Timer进行初始化
    void TimerInit() {
        RuningTime = RTO;
        _IsItRuning = true;
    }

    // Timer暂停并且重置
    void TimerStop() {
        RuningTime = 0;
        _IsItRuning = false;
    }
    //!@}

    // 定义 时间的流逝
    void RunOutTime(int TheTimeRuningOut) { RuningTime -= TheTimeRuningOut; }

    // 定义 timer是否运行
    bool IsItRuning() { return _IsItRuning; }

    // 设置RTO
    void SetRTO(uint64_t NewRTO) { RTO = NewRTO; }

    // 使RTO翻倍
    void DoubleRTo() { RTO *= 2; }

    //!@{ 定义访问器

    int TheTimeLeft() { return RuningTime; }
};
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    //! retransmission timer for the connection
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};

    //! 记录连续重传的次数
    unsigned int ConsecutiveRetransmissions = 0;

    //! 记录没有收到ack的序列占的字节数 用(see TCPSegment::length_in_sequence_space())计数
    size_t BytesInFlight = 0;

    //! 定义Timer
    TCPTimer Timer;

    //! 定义接收方的窗口大小
    size_t WindowSize = 1;

    // 备份队列
    std::queue<TCPSegment> _backup{};

    // 接受方接受到的绝对下标
    uint64_t _abs_re_seqno{0};

    // 是否发送过SYN
    bool SYNSET = false;

    // 是否发送过FIN
    bool FINSET = false;

    // 标记接收方窗口是否为0
    bool _window_size_is_0 = false;

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{
    void SendCommonSegment(TCPSegment &segment);

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }

    //! \brief 窗口大小访问器
    size_t GetWindowSize() { return WindowSize; }

    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}

    //! \name 发送数据报相关函数
    //!@{
    void Retransmission() {
        if (_backup.empty())
            return;
        _segments_out.push(_backup.front());
    }

    void IncrementRetransmission() { ConsecutiveRetransmissions++; }
    //!@}

    bool Synset() { return SYNSET; }
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
// #include <iostream>
#include <algorithm>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _rto{retx_timeout} {}

uint64_t TCPSender::bytes_in_flight() const { return BytesInFlight; }

void TCPSender::fill_window() {
    // 如果没发送过syn
    if (!SYNSET) {
        SYNSET = true;
        TCPSegment seg;
        seg.header().syn = true;
        SendCommonSegment(seg);
        return;
    }
    // 如果syn没被ack过，直接返回
    if (!_backup.empty() && _backup.front().header().syn)
        return;
    // 流如果为空但是没有结束输入，返回
    if (!_stream.buffer_size() && !_stream.eof())
        return;
    // 如果已经发送过FIN，返回
    if (FINSET)
        return;
    // 如果接受者窗口大小不是0
    if (_receiver_window_size) {
        // 还有空间
        while (_receiver_free_space) {
            TCPSegment seg;
            size_t payload_size = min({_stream.buffer_size(),
                                       static_cast<size_t>(_receiver_free_space),
                                       static_cast<size_t>(TCPConfig::MAX_PAYLOAD_SIZE)});
            seg.payload() = Buffer{_stream.read(payload_size)};
            // 流结束，并且接收方的窗口还有多余的空间（fin占一个序号）
            if (_stream.eof() && static_cast<size_t>(_receiver_free_space) > payload_size) {
                seg.header().fin = true;
                FINSET = true;
            }
            SendCommonSegment(seg);
            if (_stream.buffer_empty())
                break;
        }
    } else if (_receiver_free_space == 0) {
        // 0窗口检测报文只发送一次（tick会重发）———— 0 窗口视为 1大小
        TCPSegment seg;
        if (_stream.eof()) {
            seg.header().fin = true;
            FINSET = true;
            SendCommonSegment(seg);
        } else if (!_stream.buffer_empty()) {
            seg.payload() = Buffer{_stream.read(1)};
            SendCommonSegment(seg);
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // 计算当前的标记
    uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);

    // 如果ack非法，不管
    if (!_ack_valid(abs_ackno))
        return;

    // 重置接收方空间和剩余空间
    _receiver_window_size = window_size;
    _receiver_free_space = window_size;

    // pop已经完整收到的报文
    while (!_backup.empty()) {
        TCPSegment seg = _backup.front();
        if (unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space() <= abs_ackno) {
            BytesInFlight -= seg.length_in_sequence_space();
            _backup.pop();
            _time_elapsed = 0;
            _rto = _initial_retransmission_timeout;
            ConsecutiveRetransmissions = 0;
        } else {
            break;
        }
    }

    // 没有收到ack的包，需要占用空间
    if (!_backup.empty()) {
        _receiver_free_space =
            static_cast<uint16_t>(abs_ackno + static_cast<uint64_t>(window_size) -
                                  unwrap(_backup.front().header().seqno, _isn, _next_seqno) - BytesInFlight);
    }

    // 没有还在传输的字节，就停止计时器
    if (!BytesInFlight)
        _timer_running = false;

    // 重新fill
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (!_timer_running)
        return;

    // 维护时间
    _time_elapsed += ms_since_last_tick;

    // 重传
    if (_time_elapsed >= _rto) {
        _segments_out.push(_backup.front());
        if (_receiver_window_size || _backup.front().header().syn) {
            ++ConsecutiveRetransmissions;
            _rto <<= 1;
        }
        _time_elapsed = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return ConsecutiveRetransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}

// 判断ack是否合法
bool TCPSender::_ack_valid(uint64_t abs_ackno) {
    if (_backup.empty())
        return abs_ackno <= _next_seqno;
    return abs_ackno <= _next_seqno && abs_ackno >= unwrap(_backup.front().header().seqno, _isn, _next_seqno);
}

//  发送正常报文段
void TCPSender::SendCommonSegment(TCPSegment &seg) {
    seg.header().seqno = wrap(_next_seqno, _isn);

    // 维护对应值
    _next_seqno += seg.length_in_sequence_space();
    BytesInFlight += seg.length_in_sequence_space();

    if (SYNSET)
        _receiver_free_space -= seg.length_in_sequence_space();
    
    // 放入输出和备份队列 
    _segments_out.push(seg);
    _backup.push(seg);

    if (!_timer_running) {
        _timer_running = true;
        _time_elapsed = 0;
    }
}
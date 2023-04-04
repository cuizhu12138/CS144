#include "tcp_sender.hh"

#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <random>

#define endl "\n"
// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes them
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
    , Timer(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return BytesInFlight; }

void TCPSender::fill_window() {
    size_t ReaminWindows = WindowSize + _abs_re_seqno - _next_seqno;

    TCPSegment spsegment;

    if (!SYNSET) {
        spsegment.header().syn = 1;
        SendCommonSegment(spsegment);
        SYNSET = true;
    } else if (_stream.eof() && (FINSET)) {
        return;
    }
    // 因为输入流可以随时停止，可能上次发完就停，所以可会有单独的fin出现
    else if (_stream.eof() && (!FINSET) && ReaminWindows > 0) {
        spsegment.header().fin = 1;
        SendCommonSegment(spsegment);
        FINSET = true;
        return;
    } else {
        while (ReaminWindows) {
            TCPSegment segment;

            // 计算一下可以传输多少字节
            size_t limit = min(TCPConfig ::MAX_PAYLOAD_SIZE, ReaminWindows);

            segment.payload() = Buffer(std::move(_stream.read(limit)));

            size_t _length = segment.length_in_sequence_space();

            if (_length == 0)
                return;

            // fin占用一个序号，多发fin会超过窗口
            if (_stream.eof() && _length < ReaminWindows) {
                segment.header().fin = 1;
                FINSET = true;
            }

            SendCommonSegment(segment);

            ReaminWindows = WindowSize + _abs_re_seqno - _next_seqno;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    cerr << "ack is = " << ackno.raw_value() << endl;

    //  计算当前ackno对于的absackno
    uint64_t NowReceive = unwrap(ackno, _isn, _next_seqno);

    // 不可能的ack要忽略
    if (NowReceive > _next_seqno)
        return;

    // 维护接收方窗口是否为0
    _window_size_is_0 = (window_size == 0 ? true : false);

    // 维护接收方窗口大小,窗口为0视为1
    WindowSize = (window_size == 0 ? 1 : window_size);

    // 现在收到了ack在哪
    _abs_re_seqno = NowReceive;

    // true表示有新数据被接收方收到
    bool FlagAboutNewData = false;

    // 对于已经完全收到的段，全部pop
    while (!_backup.empty()) {
        TCPSegment &tcpsegment = _backup.front();
        uint64_t Thisabsseqno =
            unwrap(tcpsegment.header().seqno, _isn, _next_seqno) + tcpsegment.length_in_sequence_space() - 1;

        if (NowReceive > Thisabsseqno) {
            BytesInFlight -= tcpsegment.length_in_sequence_space();
            cerr << "-" << tcpsegment.length_in_sequence_space() << ' ' << BytesInFlight << endl;
            FlagAboutNewData = true;
            _backup.pop();
        } else {
            break;
        }
    }

    if (FlagAboutNewData) {
        Timer.SetRTO(_initial_retransmission_timeout);
        Timer.TimerInit();
        ConsecutiveRetransmissions = 0;
    }
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (!Timer.IsItRuning())
        return;
    Timer.RunOutTime(ms_since_last_tick);

    // 如果时间耗尽
    if (Timer.TheTimeLeft() <= 0 && Timer.IsItRuning()) {
        // 超时重传
        Retransmission();

        if (!_window_size_is_0) {
            // 出现连续重传
            IncrementRetransmission();
            // 加倍RTO
            Timer.DoubleRTo();
        }
        // 计时器RESET
        Timer.TimerInit();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return ConsecutiveRetransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment tcpsegment;

    // 从输出流中读取需要传输的字节
    string payloadstring = "";

    // 构造buff
    Buffer payload(move(payloadstring));

    // 传入数据
    tcpsegment.payload() = payload;

    tcpsegment.header().seqno = wrap(_next_seqno, _isn);

    // 放入发送队列
    _segments_out.push(tcpsegment);
}
void TCPSender::SendCommonSegment(TCPSegment &segment) {
    // 计算当前报文段序号
    segment.header().seqno = wrap(_next_seqno, _isn);

    // 维护序列号和未到达序号数
    _next_seqno += segment.length_in_sequence_space();
    BytesInFlight += segment.length_in_sequence_space();
    cerr << "+" << segment.length_in_sequence_space() << " " << BytesInFlight << endl;

    // 塞入发送队列和备份队列
    _segments_out.push(segment);
    _backup.push(segment);

    // 启动计时器
    if (!Timer.IsItRuning())
        Timer.TimerInit();
}

/*
6 ac

所有发送 和 重传函数 都要启动计时器(如果没有启动)

如果所有数据都被发送，需要停止计时器

*/

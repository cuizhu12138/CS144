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
    size_t ReaminWindows = WindowSize;

    TCPSegment spsegment;

    if (!SYNSET) {
        spsegment.header().syn = 1;
        SendCommonSegment(spsegment);
        SYNSET = true;
    } else if (_stream.eof()) {
        spsegment.header().fin = 1;
        SendCommonSegment(spsegment);
    } else {
        while (ReaminWindows > 0) {
            if (_stream.buffer_empty())
                break;

            TCPSegment segment;

            // 计算一下可以传输多少字节
            size_t limit = min(TCPConfig ::MAX_PAYLOAD_SIZE, ReaminWindows);
            limit = min(limit,_stream.buffer_size());

            segment.payload() = Buffer(std::move(_stream.read(limit)));

            segment.header().seqno = wrap(_next_seqno, _isn);

            // 维护剩余窗口大小
            ReaminWindows -= segment.length_in_sequence_space();

            SendCommonSegment(segment);
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // [debug]
    cerr << "the " << _ackused << " ack received" << endl;
    cerr << "the ack number is " << ackno.raw_value() << endl;
    // 维护接收方窗口大小
    WindowSize = window_size;

    //  计算当前ackno对于的absackno
    uint64_t NowReceive = unwrap(ackno, _isn, _next_seqno);

    // [debug]
    cerr << "ack_receive -> nowreceive = " << NowReceive << endl;

    // true表示有新数据被接收方收到
    bool FlagAboutNewData = false;

    // 对于已经完全收到的段，全部pop
    while (!_backup.empty()) {
        cerr << "flag\n\n";

        TCPSegment &tcpsegment = _backup.front();
        uint64_t Thisabsseqno =
            unwrap(tcpsegment.header().seqno, _isn, _next_seqno) + tcpsegment.length_in_sequence_space() - 1;

        // [debug]
        cerr << "ack_receive -> back up absseqno = " << Thisabsseqno << endl;

        if (NowReceive > Thisabsseqno) {
            _backup.pop();
            BytesInFlight -= tcpsegment.length_in_sequence_space();
            FlagAboutNewData = true;
        } else {
            Retransmission();
            break;
        }
    }
    // [debug]
    cerr << "ack_receive -> flag = " << FlagAboutNewData << endl;

    if (FlagAboutNewData) {
        Timer.SetRTO(_initial_retransmission_timeout);
        Timer.TimerInit();

        ConsecutiveRetransmissions = 0;
        fill_window();
    }

    // [debug]
    cerr << "ack received is over \n*************************\n\n\n\n" << endl;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (!Timer.IsItRuning())
        return;
    Timer.RunOutTime(ms_since_last_tick);
    // 如果时间耗尽
    if (Timer.TheTimeLeft() <= 0) {
        // 超时重传
        Retransmission();
        // 出现连续重传
        IncrementRetransmission();
        // 加倍RTO
        Timer.DoubleRTo();
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

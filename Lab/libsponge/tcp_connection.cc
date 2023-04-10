#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//  ？？？？？？？？？？？？？
size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return LastSegmentReceived; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    LastSegmentReceived = 0;

    // 如果rst设置，则输入输出流全部设置成error，并且永久切断连接
    if (seg.header().rst && SYNSET) {
        CutConnection();
        return;
    }

    // 把报文给receiver进行处理
    _receiver.segment_received(seg);

    // 判断是否需要逗留
    if (check_inbound_ended() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }

    // 如果ack设置了，则告诉sender ackno和窗口大小
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        TCPSegment lsseg;
        while (!_sender.segments_out().empty()) {
            lsseg = _sender.segments_out().front();
            _sender.segments_out().pop();
            FillWithACK(lsseg);
            _segments_out.push(lsseg);
        }
    }

    // 如果收到远端的fin,意味着对面已经发完了
    if (seg.header().fin) {
        InComeStream = true;
        if (OutComeStream == false) {
            _linger_after_streams_finish = false;
        }
    }

    if (!SYNSET && seg.header().syn) {
        connect();
    }
    // 如果传入段占用任何序列号，则至少发送一个段回复，反应ackno和窗口大小的更新
    else if (seg.length_in_sequence_space()) {
        SenderReflectAcknoWin();
    }
}

bool TCPConnection::active() const { return Alive; }

size_t TCPConnection::write(const string &data) {
    // 往sender的发送流里面写
    size_t final_accpet = _sender.stream_in().write(data);
    SenderFillWindow();
    return final_accpet;
}

// 要维护TimeNow
//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    // 告诉sender时间流逝
    _sender.tick(ms_since_last_tick);

    LastSegmentReceived += ms_since_last_tick;

    // 重传读取
    while (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        FillWithACK(seg);
        _segments_out.push(seg);
    }

    // 如果重传次数太多，就直接切断连接
    if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
        SendRSTSegment();
        CutConnection();
    }

    if (InComeStream && OutComeStream && _sender.bytes_in_flight() == 0) {
        if (!_linger_after_streams_finish) {
            Alive = false;
        } else {
            LingerBegin = true;
        }
    }

    // 如果已经开始逗留，就直接更新逗留计时器
    if (LingerBegin) {
        LingerTimer += ms_since_last_tick;
        if (LingerTimer >= _cfg.rt_timeout * 10) {
            Alive = false;
        }
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    SenderFillWindow();

    OutComeStream = true;
}

void TCPConnection::connect() {
    // 第一次调用直接发送syn
    SYNSET = true;
    SenderFillWindow();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            SendRSTSegment();
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

bool TCPConnection::check_inbound_ended() {
    return _receiver.unassembled_bytes() == 0 && _receiver.stream_out().input_ended();
}
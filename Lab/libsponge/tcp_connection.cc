#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return LastSegmentReceived; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    LastSegmentReceived = 0;

    // 如果rst设置，则输入输出流全部设置成error，并且关闭连接(active = false)
    if (seg.header().rst) {
        CutConnection();
        return;
    }

    // 把报文给receiver进行处理
    _receiver.segment_received(seg);

    // 判断是否需要逗留(对方先结束)
    if (check_inbound_ended() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }

    // 如果ack设置了，则告诉sender ackno和窗口大小
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        // ack_receive完，还有一次fill_windows，所以需要重新发送消息
        real_send();
    }

    // 如果传入段占用任何序列号，则至少发送一个段回复，反应ackno和窗口大小的更新
    // 这里发送的段可以是fill_windows，如果fill失败，就只发送ack和win
    if (seg.length_in_sequence_space() > 0) {
        if (!SenderFillWindow()) {
            SendeAcknoWin();
        }
    }
}

bool TCPConnection::active() const { return Alive; }

size_t TCPConnection::write(const string &data) {
    if (data.length() == 0)
        return 0;
    // 往sender的发送流里面写
    size_t final_accpet = _sender.stream_in().write(data);
    SenderFillWindow();
    return final_accpet;
}

// 要维护TimeNow
//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    LastSegmentReceived += ms_since_last_tick;
    // 告诉sender时间流逝
    _sender.tick(ms_since_last_tick);
    // 重传读取
    if (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        FillWithACK(seg);
        /*
        !!!!!!!!!!!!!!!!!rst需要加在重传的包里!!!!!!!!!!!!!!!!!!!!!!!!!
        */
        if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
            CutConnection();
            seg.header().rst = true;
        }
        _segments_out.push(seg);
    }
    // 如果输入输出流都结束了，就开始考虑关闭连接的事情
    /*
    !!!!!!!!!!!!!!!!!!!!!!用上一个报文到达的时间去判断是否需要断开连接!!!!!!!!!!!!!!!!!!!!!!!!!!!
    */
    if (check_inbound_ended() && check_outbound_ended()) {
        if (!_linger_after_streams_finish) {
            Alive = false;
        } else if (LastSegmentReceived >= 10 * _cfg.rt_timeout) {
            Alive = false;
        }
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    SenderFillWindow();
}

void TCPConnection::connect() { SenderFillWindow(); }

bool TCPConnection::check_inbound_ended() {
    return _receiver.unassembled_bytes() == 0 && _receiver.stream_out().input_ended();
}
bool TCPConnection::check_outbound_ended() {
    return _sender.stream_in().eof() && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 &&
           _sender.bytes_in_flight() == 0;
}

bool TCPConnection::real_send() {
    bool isSend = false;
    while (!_sender.segments_out().empty()) {
        isSend = true;
        TCPSegment segment = _sender.segments_out().front();
        _sender.segments_out().pop();
        FillWithACK(segment);
        _segments_out.push(segment);
    }
    return isSend;
}

void TCPConnection::SendRSTSegment() {
    // sender制造一个空报文
    _sender.send_empty_segment();
    // 取出空报文，把RST改true
    TCPSegment seg = _sender.segments_out().front();
    _sender.segments_out().pop();
    seg.header().rst = true;
    FillWithACK(seg);
    _segments_out.push(seg);
}

void TCPConnection::CutConnection() {
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    Alive = false;
}

void TCPConnection::FillWithACK(TCPSegment &seg) {
    if (_receiver.ackno().has_value()) {
        seg.header().ack = true;
        seg.header().ackno = _receiver.ackno().value();
    }
    seg.header().win = _receiver.window_size();
}

bool TCPConnection::SenderFillWindow() {
    _sender.fill_window();
    return real_send();
}

void TCPConnection ::SendeAcknoWin() {
    _sender.send_empty_segment();
    TCPSegment seg = _sender.segments_out().front();
    _sender.segments_out().pop();
    FillWithACK(seg);
    _segments_out.push(seg);
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            CutConnection();
            SendRSTSegment();
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

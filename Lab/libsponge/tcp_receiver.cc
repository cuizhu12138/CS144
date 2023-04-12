#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // 未收到syn的时候，receiver处于listen状态
    if (!SYNSET && !seg.header().syn)
        return;
    // 收到SYN，进行处理
    if (!SYNSET) {
        SYN = seg.header().seqno;
        SYNSET = true;
        if (seg.header().fin)
            FINSET = true;
        _reassembler.push_substring(seg.payload().copy(), 0, seg.header().fin);
        return;
    }
    // FIN进行标记
    if (seg.header().fin)
        FINSET = true;

    _reassembler.push_substring(
        seg.payload().copy(), unwrap(seg.header().seqno, SYN, _reassembler.ack_index()) - 1, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!SYNSET)
        return nullopt;
    else {
        // 先加上SYN的序号，FIN的需要要考虑是否到达已经是否重组完毕
        return wrap(_reassembler.ack_index() + 1 + (FINSET && _reassembler.unassembled_bytes() == 0), SYN);
    }
}
//  总容量减去没有读取的容量
size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }

#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    size_t SYNADD = 0;
    // 如果syn置1 先标记一下syn已经收到，然后写入ISN，并且当前报文如果携带信息，需要特殊判断
    if (seg.header().syn) {
        SYNSET = true;
        SYN = seg.header().seqno;
        SYNADD = 1;
    }
    // 如果fin置1 标记一下
    if (seg.header().fin) {
        FINSET = true;
    }
    // 没收到过SYN则一直等待
    if (!SYNSET)
        return;

    // 把报文携带的字符串传入流重组器
    _reassembler.push_substring(seg.payload().copy(),
                                unwrap(seg.header().seqno + SYNADD, SYN, _reassembler.GetLastRea()) - 1,
                                seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!SYNSET)
        return {};
    else {
        // 判断一下是否整个字节流都已经重组完毕，如果是，就要算上fin，否则不需要
        if (_reassembler.stream_out().input_ended())
            return wrap(_reassembler.GetNowToWrite() + 2, SYN);
        else
            return wrap(_reassembler.GetNowToWrite() + 1, SYN);
    }
}
/*
Returns the distance between the “first unassembled” index (the index corresponding to the
ackno) and the “first unacceptable” index.
*/
// 记得查看输出流的缓存区，判断这之间输出了多少字节，重新计算窗口
size_t TCPReceiver::window_size() const {
    return _reassembler.GetRight() - _reassembler.GetNowToWrite() + 1 +
           (_reassembler.GetLastRunSize() - _reassembler.stream_out().buffer_size());
}

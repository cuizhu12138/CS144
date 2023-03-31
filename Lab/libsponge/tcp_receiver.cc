#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    cerr << "Successful Received !! \n";
    cerr << "\n*************HEAD****************\n" << seg.header().to_string() << "\n*****************************\n";
    cerr << "\n**************PAYLOAD***************\n" << seg.payload().copy() << "\n*****************************\n";

    size_t SYNADD = 0;
    if (seg.header().syn) {
        SYNSET = true;
        SYN = seg.header().seqno;
        SYNADD = 1;
        // debug
        cerr << "SYN RECEIVED = " << SYN.raw_value() << endl;
        cerr << "expect to be = " << seg.header().seqno + SYNADD << endl;
        cerr << "abs seqno = " << unwrap(seg.header().seqno + SYNADD, SYN, _reassembler.GetLastRea()) - 1 << endl;
    }

    if (seg.header().fin) {
        FINSET = true;
    }

    if (!SYNSET)
        return;
    // [debug]
    cerr << "payload is = " << seg.payload().copy() << endl;
    cerr << "payload length is = " << seg.payload().copy().length() << endl;
    _reassembler.push_substring(seg.payload().copy(),
                                unwrap(seg.header().seqno + SYNADD, SYN, _reassembler.GetLastRea()) - 1,
                                seg.header().fin);

    // if (seg.header().fin) {
    //     SYN = WrappingInt32{0};
    //     SYNSET = false;
    //}
    cerr << "Segment Received done !!\n\n";
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!SYNSET)
        return {};
    else {
        // [debug]
        cerr << "check now to write in ackono = " << _reassembler.GetNowToWrite() << endl;

        if (_reassembler.stream_out().input_ended())
            return wrap(_reassembler.GetNowToWrite() + 2, SYN);
        else
            return wrap(_reassembler.GetNowToWrite() + 1, SYN);
    }
}

size_t TCPReceiver::window_size() const {
    cerr << "have read ? " << _reassembler.stream_out().buffer_size() << endl;
    return _reassembler.GetRight() - _reassembler.GetNowToWrite() + 1 +
           (_reassembler.GetLastRunSize() - _reassembler.stream_out().buffer_size());
}

/*
底层协议的校验和

*/
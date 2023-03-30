#include "wrapping_integers.hh"
// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint64_t tmp = 1ul << 32;
    return WrappingInt32((isn + (n % tmp)).raw_value() % tmp);
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // DUMMY_CODE(n, isn, checkpoint);
    uint64_t tmp = 1ul << 32;

    uint64_t Add =
        (n.raw_value() >= isn.raw_value()) ? n.raw_value() - isn.raw_value() : tmp - (isn.raw_value() - n.raw_value());

    uint64_t ans1 = (checkpoint / tmp * tmp) + Add, ans2 = ((checkpoint / tmp + 1ul) * tmp) + Add,
             ans3 = ((checkpoint / tmp - 1ul) * tmp) + Add;

    uint64_t cmp1 = ans1 > checkpoint ? ans1 - checkpoint : checkpoint - ans1,
             cmp2 = ans2 > checkpoint ? ans2 - checkpoint : checkpoint - ans2,
             cmp3 = ans3 > checkpoint ? ans3 - checkpoint : checkpoint - ans3;

    if (cmp3 <= cmp1 && cmp3 <= cmp2) {
        return ans3;
    } else if (cmp2 <= cmp1 && cmp2 <= cmp3) {
        return ans2;
    } else {
        return ans1;
    }
}

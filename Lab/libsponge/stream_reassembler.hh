#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"
typedef long long ll;
#include <cstdint>
#include <string>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.
    // 当前需要发送的报文的序号
    size_t NowToWrite = 0;

    //!< The reassembled in-order byte stream
    ByteStream _output;

    //!< The maximum number of bytes
    size_t _capacity;

    //  滑动窗口
    std::vector<char> Cap;

    // 记录右边第一位没有重组过的位置
    std::vector<ll> DSU;

    // 记录正在等待重组的字节数
    size_t WaitingReass = 0;

    // 整个流的最大下标
    ll MaxnIndexInStream = -1;

    // 表示窗口边界是否赋值，True表示已经赋值
    bool BoundaryFlag = false;

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`. 流重组程序将停留在“容量”的内存限制内
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;

    //! \brief 把字节流里的位置映射到实际位置
    ll f(ll x) { return x % _capacity; }

    //! \brief 并查集寻找爹
    ll find(ll x);
    size_t ack_index() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
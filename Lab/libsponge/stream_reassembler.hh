#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

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
    std ::vector<int> DSU;

    // 记录正在等待重组的字节数
    size_t WaitingReass = 0;

    // 未发送的第一个字节
    size_t HaveNotSend = 0;

    // 上一次跑的时候输出流中size是多少
    size_t LastRunSize = 0;

    // 整个流的最大下标
    size_t MaxnIndexInStream = INT_MAX;

    // 表示窗口边界是否赋值，True表示已经赋值
    bool BoundaryFlag = false;

    // 开启调试 True表示开启调试
    // bool Flag = false;

    //[debug] 表示调用次数
    // int _x = 0;
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
    int f(int x) { return x % _capacity; }

    //! \brief 并查集寻找爹
    int find(int x) {
        if (DSU[x] == x)
            return x;
        else {
            return DSU[x] = find(DSU[x]);
        }
    }

    //! \brief 判断是0 就变成最大值
    int Check(int x) {
        if (x == 0)
            return _capacity;
        return x;
    }
    // 得到上一个重组完成的下标
    size_t GetLastRea() const {
        if (NowToWrite == 0)
            return 0;
        else
            return NowToWrite - 1;
    }
    // 得到窗口左右边界
    size_t GetRight() const { return HaveNotSend + _capacity - 1; }
    size_t GetNowToWrite() const { return NowToWrite; }
    size_t GetLastRunSize() const { return LastRunSize; }
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

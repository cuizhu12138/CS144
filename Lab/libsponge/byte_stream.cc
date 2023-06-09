#include "byte_stream.hh"

#include <bits/stdc++.h>
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _end(false), Buff(), Capacity(capacity) {}

size_t ByteStream::write(const string &data) {
    // 计算串长和剩余容量，计算能传几个
    size_t maxn = data.length();
    size_t limit = remaining_capacity();
    int tmp = min(maxn, limit);

    // 这里tmp如果用size_t 会出问题，因为tmp可能等于0，tmp-1就直接max_int
    for (int i = 0; i <= tmp - 1; i++)
        Buff.push_front(data[i]);

    TotalWrite += tmp;

    return tmp;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string ans;

    // limit 是 当前缓存区中字节的个数
    size_t limit = buffer_size();

    // tmp 是 最终读取的个数
    int tmp = min(limit, len);

    // cnt 是 起始字节
    int cnt = limit - tmp;

    // 可能出现的问题同上个函数
    for (int i = limit - 1; i >= cnt; i--)
        ans += Buff[i];

    return ans;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t tmp = 0;

    // 保证缓冲区不空的情况下，直接pop即可
    while (!buffer_empty() && tmp < len) {
        Buff.pop_back();
        tmp++;
        TotalRead++;
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string ans = peek_output(len);

    // 读取多少就pop多少
    pop_output(ans.length());

    return ans;
}

void ByteStream::end_input() { _end = true; }

bool ByteStream::input_ended() const { return _end; }

size_t ByteStream::buffer_size() const { return Buff.size(); }

bool ByteStream::buffer_empty() const { return Buff.empty(); }

bool ByteStream::eof() const { return Buff.empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return TotalWrite; }

size_t ByteStream::bytes_read() const { return TotalRead; }

size_t ByteStream::remaining_capacity() const { return Capacity - Buff.size(); }

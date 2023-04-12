#include "stream_reassembler.hh"
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;
typedef long long ll;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), Cap(capacity + 1), DSU(capacity + 1) {
    for (size_t i = 0; i <= capacity; i++)
        DSU[i] = i;
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // 寻找现在缓存区中的数据量
    size_t NowSize = _output.buffer_size();

    size_t windows_left_edge = NowToWrite, windows_right_edge = windows_left_edge + (_capacity - NowSize - 1);

    if (index > windows_right_edge)
        return;

    if (data.length() == 0) {
        if (eof)
            MaxnIndexInStream = index + data.length() - 1;
        if ((MaxnIndexInStream != -1 && static_cast<ll>(NowToWrite) > MaxnIndexInStream) || (eof && index == 0))
            _output.end_input();
        return;
    }

    size_t Head = max(index, NowToWrite);
    size_t tmp1 = index + data.length() - 1;
    size_t Tail = min(tmp1, windows_right_edge);

    // 计算整个字节流最大下标
    if (eof && Tail == index + data.length() - 1)
        MaxnIndexInStream = tmp1;

    // 考虑到字符串头可能发生切割，计算切割增量
    ll SituationAdd = Head - index;

    ll xunhuan_Tail = Tail;

    // 根据维护好的DSU数组，快速跳到还没有重组的位置
    for (ll i = Head; i <= xunhuan_Tail; i++) {
        // 区间尾(窗口的最后一位)链接区间头会出错，所以直接对区间尾特判，区间尾的父亲永远是自己
        if (i == static_cast<ll>(windows_right_edge)) {
            if (!BoundaryFlag) {
                Cap[f(i)] = data[i - Head + SituationAdd];
                WaitingReass++;
                BoundaryFlag = true;
            }
            continue;
        }

        //  提前算好对应节点减少函数调用次数，并且先维护一下最远的爹，防止后续出错
        ll Pre = f(i);
        DSU[Pre] = find(Pre);
        // 看当前这一位最右边没有填过的一位是不是自己
        if (DSU[Pre] == Pre) {
            // 循环的窗口保存当前报文的对应字节
            Cap[Pre] = data[i - Head + SituationAdd];
            // 更新父亲
            DSU[Pre] = find(f(i + 1));
            // 顺便更新一下等待重组的数据
            WaitingReass++;

        } else {
            if (DSU[Pre] > Pre)
                i += (DSU[Pre] - Pre - 1);
            else
                i += (_capacity - Pre + DSU[Pre] - 1);
        }
    }

    // 如果爹不是自己 或者当前在窗口边界，且已经赋值成功，说明肯定已经填过这个地方了
    ll Pre = f(NowToWrite);

    while (Pre != find(Pre) || (NowToWrite == windows_right_edge && BoundaryFlag)) {
        // 先一个个字节给
        string ans = "";
        ans += Cap[Pre];
        int ReturnNum = _output.write(ans);

        // 判断是否成功写入
        if (ReturnNum == 0)
            break;

        // 执行到这里表示写入成功, 维护一下各项数据, 重置DSU数组，Cap数组
        DSU[Pre] = Pre;
        NowToWrite++;

        WaitingReass--;

        // 因为窗口移动，所以原本位于窗口最后的位置得以解放，所以重置窗口边界赋值标记，并且给窗口边界正确的DSU值
        if (BoundaryFlag) {
            BoundaryFlag = false;
            DSU[f(windows_right_edge)] = find(f(windows_right_edge + 1));
        }

        // 计算下一次要写入输出流的Pre
        Pre = f(NowToWrite);
    }

    // 如果当前要写的字节下标已经超过了整个字节流的最大下标，直接终止输出流
    if (MaxnIndexInStream != -1 && static_cast<ll>(NowToWrite) > MaxnIndexInStream)
        _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return WaitingReass; }

bool StreamReassembler::empty() const { return WaitingReass == 0; }

ll StreamReassembler::find(ll x) {
    if (DSU[x] == x)
        return x;
    else {
        return DSU[x] = find(DSU[x]);
    }
}
size_t StreamReassembler ::ack_index() const { return NowToWrite; }

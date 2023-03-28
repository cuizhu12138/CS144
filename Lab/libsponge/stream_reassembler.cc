#include "stream_reassembler.hh"
#define DEBUG(x) cerr << "x is " << x << endl;
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), Cap(capacity + 1), DSU(capacity + 1) {
    // [debug] 进行初始化标志
    if (Flag)
        cerr << "初始化" << endl;
    for (size_t i = 0; i <= capacity; i++)
        DSU[i] = i;
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // 设置开始调试的条件
    /**/ if (data.length() == 1 && data[0] == 'b' && _x == 0)
        Flag = 1;

    // 寻找现在缓存区中的数据量
    size_t NowSize = _output.buffer_size();
    // 维护现在发送到哪里，以此推出窗口大小
    HaveNotSend += (LastRunSize - NowSize);

    // 先校验一下区间, 多余部分直接丢掉
    // tmp1是给定报文段的末位， tmp2是当前窗口的末位

    // [debug] show diaoyongcishu **************************
    /**/ if (Flag) {
        cerr << "这是第" << _x << "调用" << endl;
        // [debug]
        _x++;
        cerr << "是否结束输入? " << eof << endl;
        cerr << " 数据长度是 " << data.length() << endl;
    }
    //[debug]
    if (Flag) {
        cerr << "tmpoutput"
             << ": HaveNotSend = " << HaveNotSend << endl;
        cerr << "tmpoutput"
             << ": _capacity =" << _capacity << endl;
        cerr << "tmpoutput"
             << ": HaveNotSend + _capacity - 1 = " << HaveNotSend + _capacity - 1 << endl;
        cerr << "tmpoutput"
             << ": index =" << index << endl;
    }
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    if (index > HaveNotSend + _capacity - 1)
        return;
    if (data.length() == 0) {
        if (eof)
            _output.end_input();
        return;
    }

    // [debug] show NowTowrite **************************
    /**/ if (Flag)
        cerr << "nowtowrite is " << NowToWrite << endl;

    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    size_t Head = max(index, NowToWrite);
    size_t tmp1 = index + data.length() - 1, tmp2 = HaveNotSend + _capacity - 1;
    size_t Tail = min(tmp1, tmp2);

    // 计算流最大下标
    if (eof)
        // _output.end_input();
        MaxnIndexInStream = tmp1;

    // 考虑到字符串头可能发生切割，计算切割增量
    int SituationAdd = Head - index;

    //[debug] show the head and tail **************************
    /**/ if (Flag) {
        DEBUG(Head) DEBUG(Tail);
        cerr << "tmp2 = " << tmp2 << endl;
        cerr << "Have Not Send = " << HaveNotSend << endl;
    }

    //  [debug] show the input string
    if (Flag) {
        cerr << "data length is " << data.length() << endl;
        cerr << "truely data is " << data << endl;
        cerr << "truely index is " << index << endl;
    }
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    // 根据维护好的DSU数组，快速跳到还没有重组的位置
    for (size_t i = Head; i <= Tail; i++) {
        // 区间尾(窗口的最后一位)链接区间头会出错，所以直接对区间尾特判，区间尾的父亲永远是自己
        if (i == tmp2) {
            if (!BoundaryFlag) {
                Cap[f(i)] = data[i - Head + SituationAdd];
                WaitingReass++;
                BoundaryFlag = true;
            }
            break;
        }

        //  提前算好对应节点减少函数调用次数，并且先维护一下最远的爹，防止后续出错
        int Pre = f(i);
        DSU[Pre] = find(Pre);
        // show the fa[Pre] and Pre **************************
        /**/ if (Flag) {
            cerr << "fa[Pre] is " << DSU[Pre] << endl;
            cerr << "Pre is " << Pre << endl;
        }
        // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        // 看当前这一位最右边没有填过的一位是不是自己
        if (DSU[Pre] == Pre) {
            // 循环的窗口保存当前报文的对应字节
            Cap[Pre] = data[i - Head + SituationAdd];
            // 更新父亲
            DSU[Pre] = find(f(i + 1));
            // 顺便更新一下等待重组的数据
            WaitingReass++;

            //[debug] show the string insert in---------
            /**/ if (Flag) {
                cerr << "Successful Insert!!\n";
                cerr << "the string insert in is " << data[i - Head + SituationAdd] << endl;
                cerr << "the fa[i] is " << DSU[Pre] << endl;
                cerr << "the pre is " << Pre << endl;
            }
            // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        } else {
            // 根据距离直接跳
            i += abs(Check(Pre) - Check(DSU[Pre])) - 1;
            //[debug] show jump to where ~~~~~~~~~~~~~~~~~~
            if (Flag)
                cerr << "Dont need to Insert, Jump To the " << i << endl;
            // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        }
    }

    // 如果爹不是自己，说明肯定已经填过这个地方了
    int Pre = f(NowToWrite);
    // check Pre ans fa[pre] **************************
    // if (Flag) {
    //     DEBUG(Pre)
    //     DEBUG(find(Pre))
    //     DEBUG(DSU[Pre]);
    // }
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    while (Pre != find(Pre) || (NowToWrite == tmp2 && BoundaryFlag)) {
        // 暂时先一个个字节给
        string ans = "";
        ans += Cap[Pre];
        int ReturnNum = _output.write(ans);

        // 判断是否成功写入
        if (ReturnNum == 0)
            break;

        // check if sucessful input **************************
        /**/ if (Flag) {
            cerr << "******************ans is " << ans << endl;
            cerr << "ReturnNum " << ReturnNum << endl;
        }
        // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        // 执行到这里表示写入成功, 维护一下各项数据, 重置DSU数组，Cap数组
        DSU[Pre] = Pre;
        NowToWrite++;
        if (NowToWrite > MaxnIndexInStream)
            _output.end_input();
        WaitingReass--;

        // 因为窗口移动，所以原本位于窗口最后的位置得以解放，所以重置窗口边界赋值标记，并且给窗口边界正确的DSU值
        if (BoundaryFlag) {
            BoundaryFlag = false;
            DSU[f(tmp2)] = find(f(tmp2 + 1));
        }

        // [debug] show if is succeffull update
        /*if (data.length() < 10) {
            cerr << "YES" << endl;
            cerr << "nowtowrite is " << NowToWrite << endl;
        }*/

        Pre = f(NowToWrite);
    }
    /*if (data.length() < 10)
        cerr << endl;
    */
    // 记录一下上一次缓存区中存了多少排序好但是没有读取的数据
    LastRunSize = _output.buffer_size();
}

size_t StreamReassembler::unassembled_bytes() const { return WaitingReass; }

bool StreamReassembler::empty() const { return WaitingReass == 0; }

/*
窗口移位 cap+1 重置？

*字符替换 特判

*/
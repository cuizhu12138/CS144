<!--
 * @Author:       SJX
 * @E-Mail:       540643428@qq.com
 * @TODO:         Become Better
 * @: ------------------------------------------------------
 * @Date: 2023-03-31 18:53:15
-->
# Lab0 - Networking Warmup

## 2.1 手动获取网页文件(通过Telnet)

正常情况下，浏览器输入 http://cs144.keithw.org/hello 就可以通过浏览器获取对应html文件

但是我们使用Telnet来还原这个过程

首先使用`telnet cs144.keithw.org http`来连接远程主机
即打开一个可靠的字节流，并运行http服务

正常情况下，应该得到这样结果
```
user@computer:~$ telnet cs144.keithw.org http
Trying 104.196.238.229...
Connected to cs144.keithw.org.
Escape character is '^]'.
```

然后按顺序输入
```
GET /hello HTTP/1.1
Host: cs144.keithw.org
Connection: close
```
三句话分别对应
```
告诉服务器URL的路径部分，域名的'/'之后的东西
告诉服务器URL的主机部分，即域名
告诉服务器需要在回复后关闭链接
```

然后多敲几行回车表示你已经写完了你的HTTP请求


### 作业
给一个路径让你自己去访问
```
http://cs144.keithw.org/lab0/sunetid
```
其中sunetid改成你自己的
可以得到一个独特的X-Your-Code-Is 

我得到的结果是
```
HTTP/1.1 200 OK
Date: Mon, 20 Mar 2023 13:37:39 GMT
Server: Apache
X-You-Said-Your-SunetID-Was: cuizhu12138
X-Your-Code-Is: 293654
Content-length: 115
Vary: Accept-Encoding
Content-Type: text/plain
Hello! You told us that your SUNet ID was "cuizhu12138". Please see the HTTP headers (above) for your secret code.
```
## 2.2 给自己发邮件
没有SUNetid ~~懒~~ 跳过
## 2.3 尝试连接自己
`netcat -v -l -p 9090`
一个窗口在9090端口监听

`telnet localhost 9090`
一个窗口连接自己

## 3.1编译一下

## 3.2关于一些代码规范
1. 不用malloc()和free(), new 和 delete
2. 不用指针，templates, threads(线程), locks(锁), and virtual functions(虚函数)
3. 不用C风格的字符串和对应的函数
4. 不用C风格的强制转化，用 C++ static cast 如果必须使用的话
5. 更多的使用const & 来传递函数参数
6. 使得每一个变量const除非需要被改变
7. 使每个方法都为常量，除非它需要改变对象
8. 避免全局变量，并且每个变量作用域尽可能小
9. 交作业前，请运行 make format 规范编码风格

## 3.4动手实现一个webget
```c++
// 新建一个socket
TCPSocket tcpsocket;
// 新建地址
Address address(host, "http");
// 链接目的地址
tcpsocket.connect(address);
// 发送数据包
tcpsocket.write("GET " + path + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
// 接受消息
while (!tcpsocket.eof())
{
    cout << tcpsocket.read();
} 
tcpsocket.close();
```

## 4 实现一个可靠数据流
分两步，先在.hh中加private变量
然后再到.cc中补充函数实现

DUMMY_CODE函数指伪代码(没用的代码)，用来通过编译，实际不需要使用(直接删掉就行)

具体用双端队列模拟即可，注意队列中存的是字节，string每个字符要拆开来写，注意边界条件的判断，注意size_t类型可能的坑，同时记得维护每一个给定的值


## 笔记
### 1. =delete 操作
如果你不希望你的类被拷贝 (类不可以被拷贝意味着只能用指针或引用传参，节约资源)

请显式的通过在public域中（private本身就是不能显式的直接调用的）使用 =delete 或其他手段禁用之；
```c++
// MyClass is neither copyable nor movable.
MyClass(const MyClass&) = delete;
MyClass& operator=(const MyClass&) = delete;
```
对于=delete，效果就是防止某个函数被调用。

更进一步来说，可能是为了：

（1）防止隐式转换

（2）希望类不能被拷贝（之前的做法是把类的构造函数定义为private）


```c++
#include <cstdio>
class TestClass
{
public:
    void func(int data) { printf("data: %d\n", data); }
    void func(double data)=delete;
};
int main(void)
{
    TestClass obj;
    obj.func(100);
    obj.func(100.0);
    return 0;
}
```

比如这样，第二次调用obj.func的时候就会报错，因为

`[Error] use of deleted function 'void TestClass::func(double)'`
### 2.explicit关键字
如果你不希望类的构造函数隐式自动转化的话，可以添加explicit关键字

### 3.=default
C++11新增了=default标识，编译器看到后，会生成默认的执行效率更高的函数定义体，同时会减轻编码时的工作量。

### 4.函数参数表后面加const
意味这这是一个只读函数，不可以在里面修改参数列表

# Lab1 - Stitching Bubstrings Into a Byte Stream

Lab内容：实现一个**流重组器**

前期读题非常困难（）

后续读题正确后，得出正确题意是：给一个子串和第一个字母的下标，然后进行强制在线的流重组，并且尽可能的把重组完的流推入输出队列

**具体实现**：用循环数组(基于取模运算)来实现窗口滑动，用并查集维护每一个点右边第一个没有写入字节的下标，总体复杂度线性$O(n)$

注意：

1.输出队列和流重组器实际上共用一个$Capacity$，只有被$read$之后，才会不占用空间 

2.给定的字符串中可能有任何数据

3.子串可能以任何顺序到达

4.可能有空的子串

5.注意函数中$eof$的定义，以及对于输出流的操控

6.给定的子串可能有重复内容(可能有前几个子串中已经发过的内容)

7\. 一个字节流只有一个流重组器对其重组，即只有一次初始化

8\. 一个test文件内可能有多组测试，报错内容只有错的那一组

9\.注意窗口位置的计算

10\.对传入字符串进行头切割操作时，在添加数据时需要额外计算下标

11\.记得维护各种有用的变量

12\.窗口尾的DSU值不能直接指向下一位，需要特殊判断，否则在找爹的时候可能会死循环


# Lab2 - the TCP receiver


## 1 总览
 
TCPreceiver 需要告诉发送方

1.the index of the “first unassembled” byte -- 即第一个还没有被流重组的序号

2\.the distance between the “first unassembled” index and the “first unacceptable” index. -- 第一个未重组的 到 窗口上限

## 3.1 在64位下标 和 32位序列号之间转换（对序列号进行封装）
1.TCP头中，序列号只有32位，但是字节流几乎是可以任意长的

2\.为了安全和避免与旧的链接冲突，序列号都是随机开始的

3\.逻辑开始和逻辑结束标志都会占一个序列号，注意TCP确保字节流的头和尾都能可靠的接受


**wrap函数如下，记得对（1 << 32）取模即可**
```c++
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint64_t tmp = 1ul << 32;
    return WrappingInt32((isn + (n % tmp)).raw_value() % tmp);
}
```


**unwrao函数如下，根据检查点的位置直接枚举前中后三段即可，在处理每一段具体位置的时候数据为负数直接让他自然溢出就行**
```c++
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
```


## 3.2 实现TCP接收器
1\.接受来自peer的片段

2\.使用流重组器对收到的片段进行重组

3\.计算acknumber和Windows大小

具体实现上讲:

对于`segment_received`函数，你需要做以下这些事情

1\.如果需要，请设置初始序列号。第一个到达的设置了SYN标志的段的序列号为初始序列号。为了在32位封装的seqnos/acknos和它们的绝对等价物之间进行转换，您需要跟踪它。(请注意，SYN标志只是头文件中的一个标志。同一段也可以携带数据，甚至可以设置FIN标志。)

2\.将任何数据或流结束标记推到StreamReassembler。如果
FIN标志设置在TCPSegment的报头中，这意味着负载的最后一个字节是整个流的最后一个字节。记住，StreamReassembler期望流索引从0开始;你将不得不unwarp序列号去做到这些

对于`ackno函数`，你需要做以下事情

1\.返回一个optional\<WrappingInt32>，其中包含接收端不知道的第一个字节的序列号。这是窗口的左边缘,即接收方感兴趣的第一个字节。如果ISN尚未设置，则返回一个空的optional

对于`window_size函数`，你需要做以下事情

1\.返回“第一个未组装”索引(与ackno对应的索引)与“第一个未接受(unacceptable)”索引之间的距离



**代码实现上：**

1.不需要自己解析报文（卡了很久）

2\.注意$syn$和$fin$的序号什么时候该加上去什么时候不该加

3\.计算窗口的时候，要重新计算缓存区中读取了多少数据

4\.没有收到$syn$之前，需要一直等待，且$syn$置1的报文也可能携带数据


.cc文件如下
```c++
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
```
.hh文件只是多加了两个变量
```c++
// syn FLAG
WrappingInt32 SYN = WrappingInt32(0);
bool SYNSET = false;
// fin FLAG
bool FINSET = false;
```

# Lab3 - the TCP sender

## 3 - TCPsender

TCPsender需要做什么？

1\.跟踪接收端的窗口大小(根据返回的ack和Window size)

2\.尽可能的填充窗口，sender只会在输入队列空，或是在窗口满时停止发送

3\.跟踪，并超时重传未完成传输的包

## 3.1 - 如何实现超时重传？

1\.TCPsender 会周期性的调用$tick$函数，表示时间的流逝

2\.构造 TCPSender 时，会为其提供一个参数，告诉它重传超时 (RTO) 的“初始值”。 RTO 是重新发送未完成的 TCP 段之前等待的毫秒数。 RTO 的值会随着时间的推移而变化，但“初始值”保持不变。起始代码将 RTO 的“初始值”保存在名为初始重传超时的成员变量中。

3\.您将实现重传计时器：一个可以在特定时间启动的警报，一旦 RTO 过去，警报就会响起（或“过期”）。我们强调，这种时间流逝的概念来自被调用的 tick 方法——而不是通过获取一天中的实际时间。

4\.每次发送包含数据（序列空间中的非零长度）的段（无论是第一次还是重传），如果定时器未运行，则启动它运行，以便它在 RTO 毫秒后到期（对于当前值保留时间）。通过“过期”，我们的意思是时间将在未来用完一定的毫秒数。

5\.当所有未完成的数据都被确认后，停止重传计时器。

6\.如果调用 tick 并且重传计时器已过期：

 (a) 重传尚未被 TCP 接收方完全确认的最早（最低序列号）段。您需要将未完成的段存储在一些内部数据结构中，以便可以执行此操作。 
 
 (b) 如果窗口大小不为零： 
 > 1．跟踪连续重传的次数，并增加它，因为你刚刚重传了一些东西。您的 TCPConnection 将使用此信息来确定连接是否无望（连续重传次数过多）并需要中止  
 > 2.将 RTO 的价值翻倍。这被称为“指数退避”——它会减慢糟糕网络上的重传速度，以避免进一步破坏工作。 
 
 (c) 重置重传计时器并启动它，使其在 RTO 毫秒后到期（考虑到您可能刚刚将 RTO 的值加倍！）。

7\.当接收方向发送方发送确认成功接收新数据的 ackno 时（ackno 反映的绝对序列号大于任何先前的 ackno）： 

(a) 将 RTO 设置回其“初始值”。 

(b) 如果发送方有任何未完成的数据，重新启动重传定时器，使其在 RTO 毫秒（对于 RTO 的当前值）后到期。

 (c) 将“连续重传”的计数重置为零。

## 3.2 - 实现TCPSender(文档摆一下)

1\. void fill window()

>The TCPSender is asked to fill the window : it reads from its input ByteStream and
sends as many bytes as possible in the form of TCPSegments, as long as there are new
bytes to be read and space available in the window.
You’ll want to make sure that every TCPSegment you send fits fully inside the receiver’s
window. Make each individual TCPSegment as big as possible, but no bigger than the
value given by TCPConfig::MAX PAYLOAD SIZE (1452 bytes).
You can use the TCPSegment::length in sequence space() method to count the total
number of sequence numbers occupied by a segment. Remember that the SYN and
FIN flags also occupy a sequence number each, which means that they occupy space in
the window.

>**What should I do if the window size is zero?** If the receiver has announced a
window size of zero, the fill window method should act like the window size is
one. The sender might end up sending a single byte that gets rejected (and not
acknowledged) by the receiver, but this can also provoke the receiver into sending
a new acknowledgment segment where it reveals that more space has opened up
in its window. Without this, the sender would never learn that it was allowed to
start sending again.

2\.void ack received( const WrappingInt32 ackno, const uint16 t window size)

>A segment is received from the receiver, conveying the new left (= ackno) and right (=
ackno + window size) edges of the window. The TCPSender should look through its
collection of outstanding segments and remove any that have now been fully acknowl-
edged (the ackno is greater than all of the sequence numbers in the segment). The
TCPSender should fill the window again if new space has opened up.

3\.void tick( const size t ms since last tick ):

>Time has passed — a certain number of milliseconds since the last time this method
was called. The sender may need to retransmit an outstanding segment.

4\. void send empty segment(): 

>The TCPSender should generate and send a TCPSegment
that has zero length in sequence space, and with the sequence number set correctly.
This is useful if the owner (the TCPConnection that you’re going to implement next
week) wants to send an empty ACK segment.
Note: a segment like this one, which occupies no sequence numbers, doesn’t need to be
kept track of as “outstanding” and won’t ever be retransmitted.

# Lab4 - TCPConnection

仔细阅读文档即可
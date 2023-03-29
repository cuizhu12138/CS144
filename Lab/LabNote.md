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




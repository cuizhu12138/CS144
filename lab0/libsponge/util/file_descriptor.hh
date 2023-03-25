#ifndef SPONGE_LIBSPONGE_FILE_DESCRIPTOR_HH
#define SPONGE_LIBSPONGE_FILE_DESCRIPTOR_HH

#include "buffer.hh"

#include <array>
#include <cstddef>
#include <limits>
#include <memory>

//! A reference-counted handle to a file descriptor  文件描述符的引用计数句柄
class FileDescriptor
{
  //! \brief A handle on a kernel file descriptor. 内核文件描述符上的句柄
  //! \details FileDescriptor objects contain a std::shared_ptr to a FDWrapper.
  class FDWrapper
  {
  public:
    int _fd;                   //!< The file descriptor number returned by the kernel              内核返回的文件描述符编号
    bool _eof = false;         //!< Flag indicating whether FDWrapper::_fd is at EOF               表明_fd是否在EOF
    bool _closed = false;      //!< Flag indicating whether FDWrapper::_fd has been closed         表明_fd是否被关闭
    unsigned _read_count = 0;  //!< The number of times FDWrapper::_fd has been read               _fd被读取的次数
    unsigned _write_count = 0; //!< The numberof times FDWrapper::_fd has been written             _fd被写的次数

    //! Construct from a file descriptor number returned by the kernel                             从内核返回的文件描述符数构造,explict 创建对象的时候不能进行隐式转换
    explicit FDWrapper(const int fd);
    //! Closes the file descriptor upon destruction                                                析构函数
    ~FDWrapper();
    //! Calls [close(2)](\ref man2::close) on FDWrapper::_fd
    void close();

    //! \name
    //! An FDWrapper cannot be copied or moved

    //!@{
    FDWrapper(const FDWrapper &other) = delete;
    FDWrapper &operator=(const FDWrapper &other) = delete;
    FDWrapper(FDWrapper &&other) = delete;
    FDWrapper &operator=(FDWrapper &&other) = delete;
    //!@}
  };

  //! A reference-counted handle to a shared FDWrapper                                            共享的引用计数句柄
  std::shared_ptr<FDWrapper> _internal_fd;

  // private constructor used to duplicate the FileDescriptor (increase the reference count)      用于复制文件描述符的私有构造函数(增加引用计数)
  explicit FileDescriptor(std::shared_ptr<FDWrapper> other_shared_ptr);

protected:
  void register_read() { ++_internal_fd->_read_count; }   //!< increment read count 增加读计数
  void register_write() { ++_internal_fd->_write_count; } //!< increment write count 增加写计数

public:
  //! Construct from a file descriptor number returned by the kernel 从内核返回的文件描述符数构造
  explicit FileDescriptor(const int fd);

  //! Free the std::shared_ptr; the FDWrapper destructor calls close() when the refcount goes to zero.
  ~FileDescriptor() = default;

  //! Read up to `limit` bytes 读取到“限制”字节
  std::string read(const size_t limit = std::numeric_limits<size_t>::max());

  //! Read up to `limit` bytes into `str` (caller can allocate storage) 向上读取' limit '字节到' str '(调用者可以分配存储)
  void read(std::string &str, const size_t limit = std::numeric_limits<size_t>::max());

  //! Write a string, possibly blocking until all is written  写一个字符串，可能阻塞直到全部写入
  size_t write(const char *str, const bool write_all = true) { return write(BufferViewList(str), write_all); }

  //! Write a string, possibly blocking until all is written  写一个字符串，可能阻塞直到全部写入
  size_t write(const std::string &str, const bool write_all = true) { return write(BufferViewList(str), write_all); }

  //! Write a buffer (or list of buffers), possibly blocking until all is written  写入缓冲区(或缓冲区列表)，可能会阻塞直到全部写入
  size_t write(BufferViewList buffer, const bool write_all = true);

  //! Close the underlying file descriptor 关闭底层文件描述符
  void close() { _internal_fd->close(); }

  //! Copy a FileDescriptor explicitly, increasing the FDWrapper refcount  显式复制文件描述符，增加fd包装器引用计数
  FileDescriptor duplicate() const;

  //! Set blocking(true) or non-blocking(false)  设置阻塞(true)或非阻塞(false)
  void set_blocking(const bool blocking_state);

  //! \name FDWrapper accessors
  //!@{

  //! underlying descriptor number  底层描述符编号
  int fd_num() const { return _internal_fd->_fd; }

  //! EOF flag state e.o f标志状态
  bool eof() const { return _internal_fd->_eof; }

  //! closed flag state 返回fd是否被关闭
  bool closed() const { return _internal_fd->_closed; }

  //! number of reads  返回fd被读取的次数
  unsigned int read_count() const { return _internal_fd->_read_count; }

  //! number of writes 返回fd被写的次数
  unsigned int write_count() const { return _internal_fd->_write_count; }
  //!@}

  //! \name Copy/move constructor/assignment operators
  //! FileDescriptor can be moved, but cannot be copied (but see duplicate()) 文件描述符可以移动，但不能复制(但请参阅duplicate())
  //!@{
  FileDescriptor(const FileDescriptor &other) = delete;            //!< \brief copy construction is forbidden
  FileDescriptor &operator=(const FileDescriptor &other) = delete; //!< \brief copy assignment is forbidden
  FileDescriptor(FileDescriptor &&other) = default;                //!< \brief move construction is allowed
  FileDescriptor &operator=(FileDescriptor &&other) = default;     //!< \brief move assignment is allowed
                                                                   //!@}
};

//! \class FileDescriptor
//! In addition, FileDescriptor tracks EOF state and calls to FileDescriptor::read and
//! FileDescriptor::write, which EventLoop uses to detect busy loop conditions.
//!
//! For an example of FileDescriptor use, see the EventLoop class documentation.

#endif // SPONGE_LIBSPONGE_FILE_DESCRIPTOR_HH

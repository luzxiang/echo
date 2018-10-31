/********************************************************
 * filename: fifo.h
 * date: 2017-7-27
 * desc: 环形缓冲区
 *       该类是参考kfifo进行了简单c++封装, 要求size必须为2的幂,
 *       Get及Put函数没有共同修改的变量, 因此支持单个生产者、单个
 *       消费者无锁使用
 ********************************************************/
#ifndef FIFO_H
#define FIFO_H
#include <iostream>

struct FIFO_
{
public:
    FIFO_(unsigned int sz = 1024);
    FIFO_(char *buf, unsigned int sz);
    virtual ~FIFO_();
    //数据写入缓冲区
    unsigned int Put(const char *buf, unsigned int len);
    //从缓冲给读取数据
    unsigned int Get(char *buf, unsigned int len);
    //返回缓冲区中数据长度
    unsigned int Len()
    {
        return in - out;
    }
    bool Empty()
    {
    	return (in == out);
    }
    
    void Reset()
    {
        in = out = 0;
    }
    //获取数据开始位置
    char* Outset()
    {
        return buffer + (out & (size - 1));
    }
    void Release(void)
    {
    }
    
private:
    void Init(unsigned int sz)
    {
        in = 0;
        out = 0;
        size = sz;
    }

public:
    //数据缓冲区
    char *buffer;
    //数据缓冲区长度
    unsigned int size;
    //写位置
    unsigned int in;
    //读位置
    unsigned int out;
private:
    //标识是否拥有buffer
    bool need_free_buffer_;
};
inline std::ostream &
operator << (std::ostream &os, const FIFO_ &fifo)
{
    os << "FIFO" << std::endl;
    os << "[" << std::endl;
    os << "size = " << fifo.size << std::endl;
    os << "in = " << fifo.in << std::endl;
    os << "out = " << fifo.out << std::endl;
    os << "len = " << fifo.in-fifo.out << std::endl;
    os << "]";
    return os;
}
#endif

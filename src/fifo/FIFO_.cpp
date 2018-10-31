/********************************************************
 * filename: fifo.cc
 * date: 2017-7-27
 * desc: 环形缓冲区实现
 ********************************************************/
#include <string.h>
#include <algorithm>
#include "FIFO_.h"
using std::min;

FIFO_::FIFO_(unsigned int sz)
{
    Init(sz);
    buffer = new char[sz];
    need_free_buffer_ = true;

}

FIFO_::FIFO_(char *buf, unsigned int sz)
{
    Init(sz);
    buffer = buf;
    need_free_buffer_ = false;
}

FIFO_::~FIFO_()
{
    //拥有的buffer才需要释放
    if(need_free_buffer_ && buffer)
        delete[] buffer;
}


/********************************************************
 * Function: Put
 * Description: 将数据写入缓冲区
 * Input: buf 要写入的数据
 *        len 写入的数据长度
 * Output: 
 * Return: 实际写入的数据长度
 * Others:
 ********************************************************/
unsigned int FIFO_::Put(const char *buf, unsigned int len)
{
    unsigned int end_len = 0;
    
    //计算可以写的数据量
    len = min(len, size - in + out);

    //将数据先放入缓冲区尾部
    end_len = min(len, size - (in & (size - 1)));
    memcpy(buffer + (in & (size - 1)), buf, end_len);
 
    //将数据放入缓冲区头部
    memcpy(buffer, buf + end_len, len - end_len);

    in += len;
    return len;
}


/********************************************************
 * Function: Get
 * Description: 从缓冲区中读取数据
 * Input: len 读取的数据长度
 * Output: buf 读取的数据
 * Return: 实际读取的数据长度
 * Others:
 ********************************************************/
unsigned int FIFO_::Get(char *buf, unsigned int len)
{
    unsigned int end_len = 0;
    //可读数据
    len = min(len, in - out);
    //先从尾部读取数据
    end_len = min(len, size - (out & (size - 1)));
    memcpy(buf, buffer + (out & (size - 1)), end_len);
 
    //再从头部读取剩余数据
    memcpy(buf + end_len, buffer, len - end_len);
 
    out += len;
    return len;
}

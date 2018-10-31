/*
 * FIFO_.h
 *
 *  Created on: 2018年9月12日
 *      Author: luzxiang
 */

#ifndef FIFO_FIFO_H_
#define FIFO_FIFO_H_
#include <mutex>
#ifdef OS_LINUX
#include <sys/time.h>
#endif
#include <thread>
#include <chrono>
#include <mutex>                // std::mutex, std::unique_lock
#include <condition_variable>    // std::condition_variable
#include <string.h>
#include "FIFO_.h"
using namespace std;

typedef std::unique_lock <std::mutex> mtxlock;
typedef struct sockt_fifo{
	FIFO_ *Buf;
	std::mutex mtx; // 全局互斥锁.
	std::condition_variable cv; // 全局条件变量.
    //数据写入缓冲区
    unsigned int Put(const char *buf, unsigned int len)
    {/* notice: if you have just one thread to call Push function,
    	 * necessary to enable the thread lock here, else you must enable*/
    	mtxlock lck(mtx);
    	len = Buf->Put(buf, len);
    	this->cv.notify_all(); // 唤醒所有线程.
    	return len;
    }
    void Release(void)
    {
    }
    //从缓冲给读取数据
    unsigned int Get(char *buf, unsigned int len)
    {
    	return Buf->Get(buf, len);
    }
    unsigned int HaveFree(void) const
    {
    	return Buf->size - Buf->Len();
    }
    bool HaveFree(unsigned int size) const
    {
    	return (Buf->size - Buf->Len() >= size);
    }
    //返回缓冲区中数据长度
    unsigned int Len() const{
    	return Buf->Len();
    }
    bool Empty() const
    {
    	return Buf->Empty();
    }
	sockt_fifo(unsigned int size = 1024)
	{
		Buf = new FIFO_(size);
	}
	sockt_fifo(char *buf, unsigned int size)
	{
		Buf = new FIFO_(buf, size);
	}
	virtual ~sockt_fifo()
	{
//		pthread_cond_destroy(&this->cv);
//		pthread_mutex_destroy(&this->mtx);
	}
}sockt_fifo_st;



#endif /* FIFO_FIFO__H_ */

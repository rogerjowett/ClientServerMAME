#ifndef __NSM_COMMON__
#define __NSM_COMMON__

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/circular_buffer.hpp>
#include <vector>

using namespace boost;
using boost::asio::ip::tcp;
using namespace std;

class Client;
class Server;

class MemoryBlock
{
public:
	unsigned char *data;
	int size;

	MemoryBlock()
        :
    data(NULL),
    size(0)
    {
    }

	MemoryBlock(int _size)
		:
	size(_size)
	{
		data = (unsigned char*)malloc(_size);
		memset(data,0,_size);
	}

	MemoryBlock(unsigned char *_data,int _size)
		:
	data(_data),
	size(_size)
	{
	}

	int getBitCount()
	{
		int bitCount=0;
		for(int a=0;a<size;a++)
		{
			for(int bitNum=0;bitNum<7;bitNum++)
			{
				if( (data[a]&(1<<bitNum)) != 0)
				{
					bitCount++;
				}
			}
		}
		return bitCount;
	}
};

#endif

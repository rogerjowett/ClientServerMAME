#include "RakPeerInterface.h"
#include "RakNetStatistics.h"
#include "RakNetTypes.h"
#include "BitStream.h"
#include "PacketLogger.h"
#include "RakNetTypes.h"

#include "NSM_Server.h"

#include <assert.h>
#include <cstdio>
#include <cstring>
#include <stdlib.h>

#include "osdcore.h"

Server *netServer=NULL;

volatile bool memoryBlocksLocked = false;

Server *createGlobalServer(string _port)
{
cout << "Creating server on port " << _port << endl;
netServer = new Server(_port);
return netServer;
}

void deleteGlobalServer()
{
if(netServer) delete netServer;
netServer = NULL;
}

// Copied from Multiplayer.cpp
// If the first byte is ID_TIMESTAMP, then we want the 5th byte
// Otherwise we want the 1st byte
extern unsigned char GetPacketIdentifier(RakNet::Packet *p);
extern unsigned char *GetPacketData(RakNet::Packet *p);
extern int GetPacketSize(RakNet::Packet *p);

Session::Session(const RakNet::RakNetGUID &_guid)
:
guid(_guid)
{
}

void Session::pushInputBuffer(const string &s)
{
	while(memoryBlocksLocked)
	{
	;
	}
	memoryBlocksLocked=true;
        inputBufferQueue.push_back(s);
    memoryBlocksLocked=false;
}

string Session::popInputBuffer()
{
	while(memoryBlocksLocked)
	{
	;
	}
	memoryBlocksLocked=true;
	if(inputBufferQueue.empty())
	{
		//cout << "INPUT BUFFER QUEUE IS EMPTY\n";
	    memoryBlocksLocked=false;
	    return string("");
	}
	//cout << "POPPING A STRING\n";
	string s = inputBufferQueue[0];
	inputBufferQueue.erase(inputBufferQueue.begin());
	memoryBlocksLocked=false;
	return s;
}

unsigned char compressedBuffer[MAX_ZLIB_BUF_SIZE];
unsigned char syncBuffer[MAX_ZLIB_BUF_SIZE];
unsigned char uncompressedBuffer[MAX_ZLIB_BUF_SIZE];

Server::Server(string _port) 
:
port(_port)
{
	rakInterface = RakNet::RakPeerInterface::GetInstance();
	rakInterface->SetIncomingPassword("MAME",(int)strlen("MAME"));
	rakInterface->SetTimeoutTime(30000,RakNet::UNASSIGNED_SYSTEM_ADDRESS);

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	firstSync=true;

}

Server::~Server()
{
	// Be nice and let the server know we quit.
	rakInterface->Shutdown(300);

	// We're done with the network
	RakNet::RakPeerInterface::DestroyInstance(rakInterface);
}

bool Server::initializeConnection()
{
	RakNet::SocketDescriptor socketDescriptor(atoi(port.c_str()),0);
	bool retval = rakInterface->Startup(16,&socketDescriptor,1)==RakNet::RAKNET_STARTED;
	rakInterface->SetMaximumIncomingConnections(16);

	if(!retval)
	{
		printf("Server failed to start. Terminating\n");
		return false;
	}
	rakInterface->SetOccasionalPing(true);
	rakInterface->SetUnreliableTimeout(1000);

	DataStructures::List<RakNet::RakNetSmartPtr < RakNet::RakNetSocket> > sockets;
	rakInterface->GetSockets(sockets);
	printf("Ports used by RakNet:\n");
	for (unsigned int i=0; i < sockets.Size(); i++)
	{
		printf("%i. %i\n", i+1, sockets[i]->boundAddress.port);
	}
    return true;
}

MemoryBlock Server::createMemoryBlock(int size)
{
	blocks.push_back(MemoryBlock(size));
	xorBlocks.push_back(MemoryBlock(size));
	staleBlocks.push_back(MemoryBlock(size));
	initialBlocks.push_back(MemoryBlock(size));
	return blocks.back();
}

MemoryBlock Server::createMemoryBlock(unsigned char *ptr,int size)
{
	blocks.push_back(MemoryBlock(ptr,size));
	xorBlocks.push_back(MemoryBlock(size));
	staleBlocks.push_back(MemoryBlock(size));
	initialBlocks.push_back(MemoryBlock(size));
	return blocks.back();
}

void Server::initialSync(const RakNet::RakNetGUID &guid)
{
	unsigned char *data = uncompressedBuffer;

	while(memoryBlocksLocked)
	{
	;
	}
	memoryBlocksLocked=true;
	cout << "IN CRITICAL SECTION\n";
	cout << "SERVER: Sending initial snapshot\n";
	int numBlocks = int(blocks.size());
	cout << "NUMBLOCKS: " << numBlocks << endl;
	memcpy(data,&numBlocks,sizeof(int));
	data += sizeof(int);

	// NOTE: The server must send stale data to the client for the first time
	// So that future syncs will be accurate
        unsigned char checksum = 0;
	for(int blockIndex=0;blockIndex<int(staleBlocks.size());blockIndex++)
	{
		//cout << "BLOCK SIZE FOR INDEX " << blockIndex << ": " << staleBlocks[blockIndex].size << endl;
		memcpy(data,&(staleBlocks[blockIndex].size),sizeof(int));
		data += sizeof(int);
		memcpy(data,staleBlocks[blockIndex].data,staleBlocks[blockIndex].size);
		data += staleBlocks[blockIndex].size;

        for(int a=0;a<staleBlocks[blockIndex].size;a++)
        {
            checksum = checksum ^ staleBlocks[blockIndex].data[a];
        }
    }
        cout << "INITIAL CHECKSUM: " << int(checksum) << endl;

        int numConstBlocks = int(constBlocks.size());
	memcpy(data,&numConstBlocks,sizeof(int));
	data += sizeof(int);


			// NOTE: The server must send stale data to the client for the first time
			// So that future syncs will be accurate
			for(int blockIndex=0;blockIndex<int(constBlocks.size());blockIndex++)
			{
				//cout << "BLOCK SIZE FOR INDEX " << blockIndex << ": " << constBlocks[blockIndex].size << endl;
		                int constBlockSize = int(constBlocks[blockIndex].size);
				memcpy(data,&constBlockSize,sizeof(int));
				data += sizeof(int);
				memcpy(data,constBlocks[blockIndex].data,constBlocks[blockIndex].size);
				data += constBlocks[blockIndex].size;
			}

	int uncompressedSize = (data-uncompressedBuffer);
	if(uncompressedSize>MAX_ZLIB_BUF_SIZE/2)
	{
		printf("SYNC BUFFER IS DANGEROUSLY SMALL\n");
	}

	uLongf compressedSizeLong = MAX_ZLIB_BUF_SIZE;

	compressedBuffer[0] = ID_INITIAL_SYNC_COMPLETE;
	compress2(
		compressedBuffer+1+sizeof(int),
		&compressedSizeLong,
		uncompressedBuffer,
		uncompressedSize,
		9
		);

	int compressedSize = (int)compressedSizeLong;
	memcpy(compressedBuffer+1,&compressedSize,sizeof(int));

	printf("COMPRESSED SIZE: %d\n",compressedSize/8/1024);

	/*
	rakInterface->Send(
		(char*)compressedBuffer,
		compressedSize+1+sizeof(int),
		HIGH_PRIORITY,
		RELIABLE_ORDERED,//UNRELIABLE_SEQUENCED,
		ORDERING_CHANNEL_SYNC,
		guid,
		false
	       );
	*/

	//
	unsigned char *sendPtr = compressedBuffer+1;
	int sizeRemaining = compressedSize+sizeof(int);
	printf("INITIAL STATE SIZE: %dKB\n",sizeRemaining/1024);
	int packetSize = sizeRemaining/100;
	while(sizeRemaining>packetSize && sizeRemaining>100)
	{
		RakNet::BitStream bitStreamPart(65536);
		unsigned char header = ID_INITIAL_SYNC_PARTIAL;
		bitStreamPart.WriteBits((const unsigned char*)&header,8*sizeof(unsigned char));
		bitStreamPart.WriteBits((const unsigned char*)sendPtr,8*packetSize);
		sizeRemaining -= packetSize;
		sendPtr += packetSize;
		rakInterface->Send(
			&bitStreamPart,
			HIGH_PRIORITY,
			RELIABLE_ORDERED,//UNRELIABLE_SEQUENCED,
			ORDERING_CHANNEL_SYNC,
			guid,
			false
		       );
	}
	{
		RakNet::BitStream bitStreamPart(65536);
		unsigned char header = ID_INITIAL_SYNC_COMPLETE;
		bitStreamPart.WriteBits((const unsigned char*)&header,8*sizeof(unsigned char));
		bitStreamPart.WriteBits((const unsigned char*)sendPtr,8*sizeRemaining);
		rakInterface->Send(
			&bitStreamPart,
			LOW_PRIORITY,
			RELIABLE_ORDERED,//UNRELIABLE_SEQUENCED,
			ORDERING_CHANNEL_SYNC,
			guid,
			false
		       );
	}
	//

        cout << "FINISHED SENDING BLOCKS TO CLIENT\n";
	cout << "SERVER: Done with initial snapshot\n";
	cout << "OUT OF CRITICAL AREA\n";
	cout.flush();
	memoryBlocksLocked=false;
}

void Server::update()
{
    osd_sleep(0);
	RakNet::Packet *p;
		for (p=rakInterface->Receive(); p; rakInterface->DeallocatePacket(p), p=rakInterface->Receive())
		{
			// We got a packet, get the identifier with our handy function
			unsigned char packetIdentifier = GetPacketIdentifier(p);

			// Check if this is a network message packet
			switch (packetIdentifier)
			{
			case ID_CONNECTION_LOST:
				// Couldn't deliver a reliable packet - i.e. the other system was abnormally
				// terminated
			case ID_DISCONNECTION_NOTIFICATION:
				// Connection lost normally
				printf("ID_DISCONNECTION_NOTIFICATION from %s\n", p->systemAddress.ToString(true));
				for(int a=0;a<(int)sessions.size();a++)
				{
					if(sessions[a].getGUID()==p->guid)
					{
						sessions[a].setGUID(RakNet::UNASSIGNED_RAKNET_GUID);
					}
				}
				break;


			case ID_NEW_INCOMING_CONNECTION:
				// Somebody connected.  We have their IP now
				printf("ID_NEW_INCOMING_CONNECTION from %s with GUID %s\n", p->systemAddress.ToString(true), p->guid.ToString());
				//Find a session index for the player
				for(int a=0;a<=(int)sessions.size();a++)
				{
					if(a==(int)sessions.size())
					{
						sessions.push_back(Session(p->guid));
                        break;
					}
					else if(sessions[a].getGUID()==RakNet::UNASSIGNED_RAKNET_GUID)
					{
						sessions[a].setGUID(p->guid);
						break;
					}
				}
				//Perform initial sync with player
				initialSync(p->guid);
				break;

			case ID_INCOMPATIBLE_PROTOCOL_VERSION:
				printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
				break;


			case ID_CLIENT_INPUTS:
				sessions[getSessionIndexFromGUID(p->guid)].pushInputBuffer(string((char*)GetPacketData(p),(int)GetPacketSize(p)));
				break;
			default:
				printf("UNEXPECTED PACKET ID: %d\n",int(packetIdentifier));
				break;
			}

		}
}

void Server::sync() 
{
	while(memoryBlocksLocked)
	{
	;
	}
	memoryBlocksLocked=true;
	int bytesSynched=0;
	//cout << "IN CRITICAL SECTION\n";
	//cout << "SERVER: Syncing with clients\n";
	bool anyDirty=false;
    unsigned char blockChecksum=0;
    unsigned char xorChecksum=0;
    unsigned char staleChecksum=0;
    unsigned char *uncompressedPtr = uncompressedBuffer;
	for(int blockIndex=0;blockIndex<int(blocks.size());blockIndex++)
	{
		MemoryBlock &block = blocks[blockIndex];
		MemoryBlock &staleBlock = staleBlocks[blockIndex];
		MemoryBlock &initialBlock = initialBlocks[blockIndex];
		MemoryBlock &xorBlock = xorBlocks[blockIndex];

		if(block.size != staleBlock.size || block.size != xorBlock.size)
		{
			cout << "BLOCK SIZE MISMATCH\n";
		}

		bool dirty=false;
		for(int a=0;a<block.size;a++)
		{
			xorBlock.data[a] = block.data[a] ^ staleBlock.data[a];
			if(xorBlock.data[a]) dirty=true;
		}
        if(dirty)
        {
		    for(int a=0;a<block.size;a++)
		    {
                blockChecksum = blockChecksum ^ block.data[a];
                xorChecksum = xorChecksum ^ xorBlock.data[a];
                staleChecksum = staleChecksum ^ staleBlock.data[a];
            }
        }
		//dirty=true;
		memcpy(staleBlock.data,block.data,block.size);
		if(firstSync)
		{
			memcpy(initialBlock.data,block.data,block.size);
		}
		if(dirty && !anyDirty)
		{
			//First dirty block
			anyDirty=true;
		}

		if(dirty)
		{
			bytesSynched += xorBlock.size;
            memcpy(
                uncompressedPtr,
                &blockIndex,
                sizeof(int)
                );
            uncompressedPtr += sizeof(int);
            memcpy(
                uncompressedPtr,
                xorBlock.data,
                xorBlock.size
                );
            uncompressedPtr += xorBlock.size;
		}
	}
	if(anyDirty && firstSync==false)
	{
        printf("BLOCK CHECKSUM: %d\n",int(blockChecksum));
        printf("XOR CHECKSUM: %d\n",int(xorChecksum));
        printf("STALE CHECKSUM: %d\n",int(staleChecksum));
	int finishIndex = -1;
	memcpy(
			uncompressedPtr,
			&finishIndex,
			sizeof(int)
	      );
	uncompressedPtr += sizeof(int);
	
	int uncompressedSize = uncompressedPtr-uncompressedBuffer;
	uLongf compressedSizeLong = MAX_ZLIB_BUF_SIZE;

	compress2(
		compressedBuffer,
		&compressedSizeLong,
		uncompressedBuffer,
		uncompressedSize,
		9
		);
	int compressedSize = (int)compressedSizeLong;
	printf("SYNC SIZE: %d\n",compressedSize);

	int SYNC_PACKET_SIZE=1024*1024*64;
	if(syncTransferSeconds)
		SYNC_PACKET_SIZE = compressedSize/60/syncTransferSeconds;
		
	int sendMessageSize = 1+sizeof(int)+sizeof(int)+min(SYNC_PACKET_SIZE,compressedSize);
	unsigned char *sendMessage = syncBuffer;
	sendMessage[0] = ID_RESYNC_PARTIAL;
	if(compressedSize<=SYNC_PACKET_SIZE)
		sendMessage[0] = ID_RESYNC_COMPLETE;
	memcpy(sendMessage+1,&uncompressedSize,sizeof(int));
	memcpy(sendMessage+1+sizeof(int),&compressedSize,sizeof(int));
	memcpy(sendMessage+1+sizeof(int)+sizeof(int),compressedBuffer,min(SYNC_PACKET_SIZE,compressedSize) );

	syncPacketQueue.push_back(make_pair(sendMessage,sendMessageSize));
	sendMessage += sendMessageSize;
	compressedSize -= SYNC_PACKET_SIZE;
	int atIndex = SYNC_PACKET_SIZE;

	while(compressedSize>0)
	{
		sendMessageSize = 1+min(SYNC_PACKET_SIZE,compressedSize);
		sendMessage[0] = ID_RESYNC_PARTIAL;
		if(compressedSize<=SYNC_PACKET_SIZE)
			sendMessage[0] = ID_RESYNC_COMPLETE;
		memcpy(sendMessage+1,compressedBuffer+atIndex,min(SYNC_PACKET_SIZE,compressedSize) );
		compressedSize -= SYNC_PACKET_SIZE;
		atIndex += SYNC_PACKET_SIZE;

		syncPacketQueue.push_back(make_pair(sendMessage,sendMessageSize));
		sendMessage += sendMessageSize;
	}

	}
	//if(runTimes%1==0) cout << "BYTES SYNCED: " << bytesSynched << endl;
	//cout << "OUT OF CRITICAL AREA\n";
	//cout.flush();
	firstSync=false;
	memoryBlocksLocked=false;
}

void Server::popSyncQueue()
{
	if(syncPacketQueue.size())
	{
	pair<unsigned char *,int> syncPacket = syncPacketQueue.front();
	//printf("Sending sync message of size %d (%d packets left)\n",syncPacket.second,syncPacketQueue.size());
	syncPacketQueue.pop_front();

	rakInterface->Send(
		(const char*)syncPacket.first,
		syncPacket.second,
       		HIGH_PRIORITY,
		RELIABLE_ORDERED,
		ORDERING_CHANNEL_SYNC,
		RakNet::UNASSIGNED_SYSTEM_ADDRESS,
		true
	       );
	}
}

void Server::addConstBlock(unsigned char *tmpdata,int size)
{
	while(memoryBlocksLocked)
	{
	;
	}
	memoryBlocksLocked=true;
    //cout << "Adding const block...\n";
    //cout << "IN CRITICAL SECTION\n";
    if(constBlocks.size()>=100)
    {
        constBlocks.erase(constBlocks.begin());
    }
    constBlocks.push_back(MemoryBlock(size));
    memcpy(constBlocks.back().data,tmpdata,size);

	uLongf compressedSize = MAX_ZLIB_BUF_SIZE;

    int ret = compress2(compressedBuffer,&compressedSize,(Bytef*)tmpdata,size,9);
	if (ret != Z_OK)
		cout << "CREATING ZLIB STREAM FAILED\n";

    int compressedSizeInt = (int)compressedSize;

    //cout << "COMPRESSING DATA FROM " << size << " TO " << compressedSize << endl;

    int sendMessageSize = 1+sizeof(int)+sizeof(int)+compressedSize;
	unsigned char *sendMessage = (unsigned char*)malloc(sendMessageSize);
	sendMessage[0] = ID_CONST_DATA;
	memcpy(sendMessage+1,&size,sizeof(int));
	memcpy(sendMessage+1+sizeof(int),&compressedSizeInt,sizeof(int));
	memcpy(sendMessage+1+sizeof(int)+sizeof(int),compressedBuffer,compressedSize);

	rakInterface->Send(
			(const char*)sendMessage,
			sendMessageSize,
			HIGH_PRIORITY,
			UNRELIABLE,
			ORDERING_CHANNEL_CONST_DATA,
			RakNet::UNASSIGNED_SYSTEM_ADDRESS,
			true
		       );
	memoryBlocksLocked=false;
    //cout << "done\n";
    //cout << "EXITING CRITICAL SECTION\n";
}

string Server::getLatencyString(int connectionIndex)
{
	char buf[4096];
	sprintf(buf,"Client %d Ping: %d ms", connectionIndex, rakInterface->GetAveragePing(rakInterface->GetSystemAddressFromGuid(sessions[connectionIndex].getGUID())));
	return string(buf);
}

string Server::getStatisticsString()
{
	RakNet::RakNetStatistics *rss;
	string retval;
	for(int a=0;a<getNumSessions();a++)
	{
		char message[4096];
		rss=rakInterface->GetStatistics(rakInterface->GetSystemAddressFromIndex(0));
		StatisticsToString(rss, message, 0);
		retval += string(message) + string("\n");
	}
	return retval;
}


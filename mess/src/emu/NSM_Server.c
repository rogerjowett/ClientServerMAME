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

z_stream strm;
unsigned char compressedOutBuf[MAX_COMPRESSED_OUTBUF_SIZE];
//unsigned char *compressedOutBufPtr;

z_stream strm2;
unsigned char decompressionBuf[MAX_COMPRESSED_OUTBUF_SIZE];

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
	return blocks.back();
}

MemoryBlock Server::createMemoryBlock(unsigned char *ptr,int size)
{
	blocks.push_back(MemoryBlock(ptr,size));
	xorBlocks.push_back(MemoryBlock(size));
	staleBlocks.push_back(MemoryBlock(size));
	return blocks.back();
}

void Server::initialSync(const RakNet::RakNetGUID &guid)
{
	while(memoryBlocksLocked)
	{
	;
	}
	memoryBlocksLocked=true;
	cout << "IN CRITICAL SECTION\n";
	cout << "SERVER: Sending initial snapshot\n";
	RakNet::BitStream bitStream(65536);
    unsigned char header = ID_INITIAL_SYNC;
	bitStream.WriteBits((const unsigned char*)&header,8*sizeof(unsigned char));
	int numBlocks = int(blocks.size());
	cout << "NUMBLOCKS: " << numBlocks << endl;
	bitStream.WriteBits((const unsigned char*)&numBlocks,8*sizeof(int));

	// NOTE: The server must send stale data to the client for the first time
	// So that future syncs will be accurate
        unsigned char checksum = 0;
	for(int blockIndex=0;blockIndex<int(staleBlocks.size());blockIndex++)
	{
		//cout << "BLOCK SIZE FOR INDEX " << blockIndex << ": " << staleBlocks[blockIndex].size << endl;
		bitStream.WriteBits((const unsigned char*)&(staleBlocks[blockIndex].size),8*sizeof(int));
		bitStream.WriteBits((const unsigned char*)staleBlocks[blockIndex].data,8*staleBlocks[blockIndex].size);

        for(int a=0;a<staleBlocks[blockIndex].size;a++)
        {
            checksum = checksum ^ staleBlocks[blockIndex].data[a];
        }
    }
        cout << "INITIAL CHECKSUM: " << int(checksum) << endl;

        int numConstBlocks = int(constBlocks.size());
	bitStream.WriteBits((const unsigned char*)&numConstBlocks,8*sizeof(int));


			// NOTE: The server must send stale data to the client for the first time
			// So that future syncs will be accurate
			for(int blockIndex=0;blockIndex<int(constBlocks.size());blockIndex++)
			{
				//cout << "BLOCK SIZE FOR INDEX " << blockIndex << ": " << constBlocks[blockIndex].size << endl;
		                int constBlockSize = int(constBlocks[blockIndex].size);
				bitStream.WriteBits((const unsigned char*)&constBlockSize,8*sizeof(int));
				bitStream.WriteBits((const unsigned char*)constBlocks[blockIndex].data,8*constBlocks[blockIndex].size);
			}

	rakInterface->Send(
			&bitStream,
			HIGH_PRIORITY,
			RELIABLE_ORDERED,//UNRELIABLE_SEQUENCED,
			ORDERING_CHANNEL_SYNC,
			guid,
			false
		       );
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
	for(int blockIndex=0;blockIndex<int(blocks.size());blockIndex++)
	{
		MemoryBlock &block = blocks[blockIndex];
		MemoryBlock &staleBlock = staleBlocks[blockIndex];
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
		if(dirty && !anyDirty)
		{
			//First dirty block
			int ret = deflateInit(&strm, 9/*Z_DEFAULT_COMPRESSION*/);
			strm.next_out = compressedOutBuf;
			strm.avail_out = MAX_COMPRESSED_OUTBUF_SIZE;
			if (ret != Z_OK)
				cout << "CREATING ZLIB STREAM FAILED\n";
			anyDirty=true;
		}

		if(dirty)
		{
			bytesSynched += xorBlock.size;
			//cout << "SERVER: SENDING DIRTY BLOCK AT INDEX: " << blockIndex << endl;
			//Send xorBlock
			//for(int a=0;a<(int)sessions.size();a++)
			{
				//cout << "SENDING DIRTY BLOCK TO CLIENT OF SIZE " << xorBlock.size << "\n";
				try
				{
					//boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(&(blockIndex), sizeof(int)));
					//boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(&(xorBlock.size), sizeof(int)));
					//boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(xorBlock.data, xorBlock.size));
					strm.next_in = (Bytef*)&blockIndex;
		    			strm.avail_in = sizeof(int);
					int retval = deflate(&strm, Z_SYNC_FLUSH);
					strm.next_in = xorBlock.data;
		    			strm.avail_in = xorBlock.size;
					retval = deflate(&strm, Z_SYNC_FLUSH);
					//cout << "DEFLATE RETVAL: " << retval << endl;
					//if(MAX_COMPRESSED_OUTBUF_SIZE - (compressedOutBufPtr-compressedOutBuf) <= 0)
					//{
						//cout << "WE BLEW OUR COMPRESSED OUTPUT BUFFER!\n";
					//}
				}
                		catch(const std::exception &ex)
				{
					//cout << "SERVER: Client closed connection, removing client\n";
					//sessions.erase(sessions.begin()+a);
				}
			}
		}
	}
	if(anyDirty)
	{
        printf("BLOCK CHECKSUM: %d\n",int(blockChecksum));
        printf("XOR CHECKSUM: %d\n",int(xorChecksum));
        printf("STALE CHECKSUM: %d\n",int(staleChecksum));
	int finishIndex = -1;
	strm.next_in = (Bytef*)&finishIndex;
	strm.avail_in = sizeof(int);
	int retval = deflate(&strm, Z_FINISH);
	//cout << "DEFLATE RETVAL: " << retval << endl;
	
	int compressedSize = MAX_COMPRESSED_OUTBUF_SIZE - strm.avail_out;
	int sendMessageSize = 1+sizeof(int)+compressedSize;
	unsigned char *sendMessage = (unsigned char*)malloc(sendMessageSize);
	sendMessage[0] = ID_RESYNC;
	memcpy(sendMessage+1,&compressedSize,sizeof(int));
	memcpy(sendMessage+1+sizeof(int),compressedOutBuf,compressedSize);

	rakInterface->Send(
			(const char*)sendMessage,
			sendMessageSize,
            LOW_PRIORITY,
			RELIABLE_ORDERED,
			ORDERING_CHANNEL_SYNC,
			RakNet::UNASSIGNED_SYSTEM_ADDRESS,
			true
		       );
    free(sendMessage);
	deflateEnd(&strm);
	}
	//if(runTimes%1==0) cout << "BYTES SYNCED: " << bytesSynched << endl;
	//cout << "OUT OF CRITICAL AREA\n";
	//cout.flush();
	memoryBlocksLocked=false;
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

	int sendMessageSize = 1+sizeof(int)+size;
	unsigned char *sendMessage = (unsigned char*)malloc(sendMessageSize);
	sendMessage[0] = ID_CONST_DATA;
	memcpy(sendMessage+1,&size,sizeof(int));
	memcpy(sendMessage+1+sizeof(int),tmpdata,size);

	rakInterface->Send(
			(const char*)sendMessage,
			sendMessageSize,
			LOW_PRIORITY,
			UNRELIABLE,
			ORDERING_CHANNEL_CONST_DATA,
			RakNet::UNASSIGNED_SYSTEM_ADDRESS,
			true
		       );
    free(sendMessage);
	memoryBlocksLocked=false;
    //cout << "done\n";
    //cout << "EXITING CRITICAL SECTION\n";
}



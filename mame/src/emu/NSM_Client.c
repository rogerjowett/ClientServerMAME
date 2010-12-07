//RAKNET MUST COME FIRST, OTHER LIBS TRY TO REPLACE new/delete/malloc/free WITH THEIR OWN SHIT
#include "RakPeerInterface.h"
#include "RakNetStatistics.h"
#include "RakNetTypes.h"
#include "BitStream.h"
#include "PacketLogger.h"
#include "RakNetTypes.h"
#include "RakSleep.h"

#include "NSM_Client.h"

#include "NSM_Server.h"

#include <assert.h>
#include <cstdio>
#include <cstring>
#include <stdlib.h>

Client *netClient=NULL;

Client *createGlobalClient(string _username)
{
    netClient = new Client(_username);
    return netClient;
}

void deleteGlobalClient()
{
    if(netClient) delete netClient;
    netClient = NULL;
}

#include "osdcore.h"
#include "emu.h"

extern unsigned char compressedBuffer[MAX_ZLIB_BUF_SIZE];
extern unsigned char uncompressedBuffer[MAX_ZLIB_BUF_SIZE];
extern unsigned char syncBuffer[MAX_ZLIB_BUF_SIZE];

bool needToSkipAhead=false;

#include "unicode.h"
#include "ui.h"
#include "osdcore.h"

Client::Client(string _username)
    :
    Common(_username)
{
    rakInterface = RakNet::RakPeerInterface::GetInstance();
    rakInterface->AllowConnectionResponseIPMigration(false);

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    initComplete=false;
    firstResync=true;

    syncPtr = compressedBuffer;

    selfPeerID = 0;
}

Client::~Client()
{
	// Be nice and let the server know we quit.
	rakInterface->Shutdown(300);

	// We're done with the network
	RakNet::RakPeerInterface::DestroyInstance(rakInterface);
}

MemoryBlock Client::createMemoryBlock(int size)
{
    blocks.push_back(MemoryBlock(size));
    xorBlocks.push_back(MemoryBlock(size));
    staleBlocks.push_back(MemoryBlock(size));
    syncCheckBlocks.push_back(MemoryBlock(size));
    return blocks.back();
}

MemoryBlock Client::createMemoryBlock(unsigned char *ptr,int size)
{
    blocks.push_back(MemoryBlock(ptr,size));
    xorBlocks.push_back(MemoryBlock(size));
    staleBlocks.push_back(MemoryBlock(size));
    syncCheckBlocks.push_back(MemoryBlock(size));
    return blocks.back();
}

// Copied from Multiplayer.cpp
// If the first byte is ID_TIMESTAMP, then we want the 5th byte
// Otherwise we want the 1st byte
unsigned char GetPacketIdentifier(RakNet::Packet *p)
{
	if (p==0)
		return 255;

	if ((unsigned char)p->data[0] == ID_TIMESTAMP)
	{
		assert(p->length > sizeof(unsigned char) + sizeof(RakNet::TimeMS));
		return (unsigned char) p->data[sizeof(unsigned char) + sizeof(RakNet::TimeMS)];
	}
	else
		return (unsigned char) p->data[0];
}

unsigned char *GetPacketData(RakNet::Packet *p)
{
	if (p==0)
		return 0;

	if ((unsigned char)p->data[0] == ID_TIMESTAMP)
	{
		assert(p->length > sizeof(unsigned char) + sizeof(RakNet::TimeMS));
		return (unsigned char*) &(p->data[sizeof(unsigned char) + sizeof(RakNet::TimeMS)+1]);
	}
	else
	{
		return (unsigned char*) &(p->data[sizeof(unsigned char)]);
	}
}
int GetPacketSize(RakNet::Packet *p)
{
	if (p==0)
		return 0;

	if ((unsigned char)p->data[0] == ID_TIMESTAMP)
	{
		assert(p->length > sizeof(unsigned char) + sizeof(RakNet::TimeMS));
		return int(p->length) - int(sizeof(unsigned char) + sizeof(RakNet::TimeMS));
	}
	else
	{
		return int(p->length) - int(sizeof(unsigned char));
	}
}

extern void video_frame_update(running_machine *machine, int debug);
int initialSyncPercentComplete=0;

bool Client::initializeConnection(unsigned short selfPort,const char *hostname,unsigned short port,running_machine *machine)
{
    RakNet::SocketDescriptor socketDescriptor(0,0);
    socketDescriptor.port = selfPort;
    printf("Client running on port %d\n",selfPort);
    RakNet::StartupResult retval = rakInterface->Startup(8,&socketDescriptor,1);
    rakInterface->SetMaximumIncomingConnections(512);
	rakInterface->SetIncomingPassword("MAME",(int)strlen("MAME"));
	rakInterface->SetTimeoutTime(30000,RakNet::UNASSIGNED_SYSTEM_ADDRESS);
    rakInterface->SetOccasionalPing(true);
	rakInterface->SetUnreliableTimeout(1000);

	if(retval != RakNet::RAKNET_STARTED)
	{
		printf("Client failed to start. Terminating\n");
		return false;
	}

    peerInputs[1] = vector<string>();

    RakNet::SystemAddress sa = ConnectBlocking(hostname,port);
    if(sa==RakNet::UNASSIGNED_SYSTEM_ADDRESS)
    {
        printf("Could not connect to server!\n");
        return false;
    }

    {
        char buf[4096];
        buf[0] = ID_CLIENT_INFO;
        strcpy(buf+1,username.c_str());
        rakInterface->Send(buf,1+username.length()+1,HIGH_PRIORITY,RELIABLE_ORDERED,0,sa,false);
    }

    peerIDs[sa] = 1;

    while(initComplete==false)
    {
        RakNet::Packet *p = rakInterface->Receive();
	if(!p)
	{
		printf("WAITING FOR SERVER TO SEND GAME WORLD...\n");
		video_frame_update(machine, false);
		RakSleep(10);
		continue; //We need the first few packets, so stall until we get them
	}
	unsigned char packetID = GetPacketIdentifier(p);

	printf("GOT PACKET: %d\n",int(packetID));

	switch(packetID)
	{
			case ID_DISCONNECTION_NOTIFICATION:
				// Connection lost normally
				printf("ID_DISCONNECTION_NOTIFICATION\n");
                if(selfPeerID==0)
                {
                    printf("Disconnected because there was no slots available or because a connection could not be made between you and another peer.\n");
                }
				return false;
			case ID_ALREADY_CONNECTED:
				printf("ID_ALREADY_CONNECTED\n");
				return false;
			case ID_INCOMPATIBLE_PROTOCOL_VERSION:
				printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
				return false;
			case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server telling the clients of another client disconnecting gracefully.  You can manually broadcast this in a peer to peer enviroment if you want.
				printf("ID_REMOTE_DISCONNECTION_NOTIFICATION\n");
				return false;
			case ID_REMOTE_CONNECTION_LOST: // Server telling the clients of another client disconnecting forcefully.  You can manually broadcast this in a peer to peer enviroment if you want.
				printf("ID_REMOTE_CONNECTION_LOST\n");
				return false;
			case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
				printf("ID_REMOTE_NEW_INCOMING_CONNECTION\n");
				break;
			case ID_CONNECTION_BANNED: // Banned from this server
				printf("We are banned from this server.\n");
				return false;
			case ID_CONNECTION_ATTEMPT_FAILED:
				printf("Connection attempt failed\n");
				return false;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				// Sorry, the server is full.  I don't do anything here but
				// A real app should tell the user
				printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
				return false;

			case ID_INVALID_PASSWORD:
				printf("ID_INVALID_PASSWORD\n");
				return false;

			case ID_CONNECTION_LOST:
				// Couldn't deliver a reliable packet - i.e. the other system was abnormally
				// terminated
				printf("ID_CONNECTION_LOST\n");
				return false;

			case ID_CONNECTION_REQUEST_ACCEPTED:
				// This tells the client they have connected
				printf("ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s\n", p->systemAddress.ToString(true), p->guid.ToString());
				printf("My external address is %s\n", rakInterface->GetExternalID(p->systemAddress).ToString(true));
				break;

            case ID_HOST_ACCEPTED:
                {
                    int peerID;
                    memcpy(&peerID,p->data+1,sizeof(int));
                    RakNet::SystemAddress sa;
                    memcpy(&(sa.binaryAddress),p->data+1+sizeof(int),sizeof(uint32_t));
                    memcpy(&(sa.port),p->data+1+sizeof(int)+sizeof(uint32_t),sizeof(unsigned short));

                    char buf[4096];
                    strcpy(buf,(char*)(p->data+1+sizeof(int)+sizeof(uint32_t)+sizeof(unsigned short)));
                    if(rakInterface->GetExternalID(p->systemAddress)==sa)
                    {
                        //This is me, set my own ID and name
                        selfPeerID = peerID;
                        peerIDs[sa] = peerID;
                        peerNames[peerID] = buf;
                    }
                    else
                    {
                        //This is someone else, set their ID and name
                        peerIDs[sa] = peerID;
                        peerNames[peerID] = buf;
                    }
                }
                break;

            case ID_SEND_PEER_ID:
                {
                    int peerID;
                    memcpy(&peerID,p->data+1,sizeof(int));
                    cout << "Matching: " << p->systemAddress.ToString() << " To " << peerID << endl;
                    peerIDs[p->systemAddress] = peerID;
                }
                break;


			case ID_INITIAL_SYNC_PARTIAL:
			{
 				printf("GOT PARTIAL SYNC FROM SERVER\n");
				initialSyncPercentComplete++;
				int curPos = (int)initialSyncBuffer.size();
				initialSyncBuffer.resize(initialSyncBuffer.size()+GetPacketSize(p));
				memcpy(&initialSyncBuffer[curPos],GetPacketData(p),GetPacketSize(p));
			}
            break;
			case ID_INITIAL_SYNC_COMPLETE:
			{
			    printf("GOT INITIAL SYNC FROM SERVER!\n");
				int curPos = (int)initialSyncBuffer.size();
				initialSyncBuffer.resize(initialSyncBuffer.size()+GetPacketSize(p));
				memcpy(&initialSyncBuffer[curPos],GetPacketData(p),GetPacketSize(p));
			    loadInitialData(&initialSyncBuffer[0],(int)initialSyncBuffer.size());
			    initComplete=true;
			}
            break;
			case ID_CONST_DATA:
			    addConstData(GetPacketData(p),GetPacketSize(p));
			    break;
            case ID_CLIENT_INPUTS:
                if(peerIDs.find(p->systemAddress)==peerIDs.end() || peerInputs.find(peerIDs[p->systemAddress])==peerInputs.end())
                {
                    cout << __FILE__ << ":" << __LINE__ << " OOPS!!!!\n";
                }
                peerInputs[peerIDs[p->systemAddress]].push_back(string((char*)GetPacketData(p),(int)GetPacketSize(p)));
                break;
            case ID_SERVER_INPUTS:
            {
                peerInputs[1].push_back(string(((char*)GetPacketData(p)),(int)GetPacketSize(p)));
            }
            break;
            case ID_SETTINGS:
	    {
                memcpy(&secondsBetweenSync,p->data+1,sizeof(int));
		char buf[4096];
		strcpy(buf,(const char*)(p->data+1+sizeof(int)));
		string s(buf,strlen(buf));
		peerNames[1] = s;
	    }
			    break;
			default:
			    //printf("GOT AN INVALID PACKET TYPE: %d\n",int(packetID));
			    //throw std::runtime_error("OOPS");
                break;
	}

	rakInterface->DeallocatePacket(p);
    }
    return true;
}

extern attotime zeroInputMinTime;

void Client::loadInitialData(unsigned char *data,int size)
{
	int uncompressedSize;
	memcpy(&uncompressedSize,data,sizeof(int));
	data += sizeof(int);

	int compressedSize;
	memcpy(&compressedSize,data,sizeof(int));
	data += sizeof(int);

    vector<unsigned char> initialSyncBuffer(uncompressedSize + 128);

	uLongf destLen = uncompressedSize+128;
	uncompress(&initialSyncBuffer[0],&destLen,data,compressedSize);

	RakNet::BitStream initialSyncStream(&initialSyncBuffer[0],uncompressedSize + 128,false);

	printf("COMPRESSED SIZE: %d\n",compressedSize/1024/8);

    RakNet::TimeUS timeBeforeSync;
    initialSyncStream.Read(timeBeforeSync);

    initialSyncStream.Read(zeroInputMinTime);
    zeroInputMinTime.seconds += 2;

    if(startupTime<timeBeforeSync)
        printf("STARTUP TIME IS TOO EARLY!!!");
    startupTime -= timeBeforeSync;

    cout << "READING NUM BLOCKS\n";
    int numBlocks;
    initialSyncStream.Read(numBlocks);

    cout << "LOADING " << numBlocks << " BLOCKS\n";
            unsigned char checksum = 0;
    if(blocks.size()==0)
    {
        //Assume the memory is not set up on the client yet and the client will assign the memory to something later
        for(int blockIndex=0;blockIndex<numBlocks;blockIndex++)
        {
            int blockSize;
            initialSyncStream.Read(blockSize);

            cout << "GOT INITIAL BLOCK OF SIZE: " << blockSize << endl;

            blocks.push_back(MemoryBlock(blockSize));
            xorBlocks.push_back(MemoryBlock(blockSize));
            staleBlocks.push_back(MemoryBlock(blockSize));

            initialSyncStream.ReadBits(blocks.back().data,blockSize*8);
            memcpy(staleBlocks.back().data,blocks.back().data,blockSize);

            for(int a=0;a<blockSize;a++)
            {
                checksum = checksum ^ blocks.back().data[a];
            }

            //cout << "INITIAL BLOCK COUNT: " << blocks.back().getBitCount() << endl;
        }
    }
    else
    {
        //Server and client must match on blocks
        if(blocks.size() != numBlocks)
        {
            cout << "ERROR: CLIENT AND SERVER BLOCK COUNTS DO NOT MATCH!\n";
        }

        for(int blockIndex=0;blockIndex<numBlocks;blockIndex++)
        {
            //cout << "ON INDEX: " << blockIndex << " OF " << numBlocks << endl;
            int blockSize;
            initialSyncStream.Read(blockSize);

            if(blockSize != blocks[blockIndex].size)
            {
                cout << "ERROR: CLIENT AND SERVER BLOCK SIZES AT INDEX " << blockIndex << " DO NOT MATCH!\n";
                cout << blockSize << " != " << blocks[blockIndex].size << endl;
                exit(1);
            }

            initialSyncStream.ReadBits(staleBlocks[blockIndex].data,blockSize*8);
            memcpy(blocks[blockIndex].data,staleBlocks[blockIndex].data,blockSize);

            for(int a=0;a<blockSize;a++)
            {
                checksum = checksum ^ blocks[blockIndex].data[a];
            }
        }
    }

    int numConstBlocks;
    initialSyncStream.Read(numConstBlocks);

    for(int blockIndex=0;blockIndex<numConstBlocks;blockIndex++)
    {
        int blockSize;
        initialSyncStream.Read(blockSize);
        //cout << "BLOCK SIZE FOR INDEX " << blockIndex << ": " << constBlocks[blockIndex].size << endl;

        constBlocks.push_back(MemoryBlock(0));
        constBlocks.back().size = blockSize;
        constBlocks.back().data = (unsigned char*)malloc(constBlocks.back().size);

        initialSyncStream.ReadBits(constBlocks.back().data,constBlocks.back().size*8);
    }

    while(true)
    {
        int peerID;
        initialSyncStream.Read(peerID);
        cout << "Read peer id: " << peerID << endl;
        if(peerID==-1)
            break;

        int numStrings;
        initialSyncStream.Read(numStrings);
        cout << "# strings: " << numStrings << endl;
        for(int a=0;a<numStrings;a++)
        {
            int strlen;
            initialSyncStream.Read(strlen);

            vector<char> buf(strlen,'\0');

            if(buf.size()!=strlen)
            {
                printf("OOPS\n");
                exit(1);
            }

            initialSyncStream.ReadBits((unsigned char*)(&buf[0]),strlen*8);
            if(peerInputs.find(peerID)==peerInputs.end())
            {
                cout << __FILE__ << ":" << __LINE__ << " OOPS!!!!\n";
            }
            peerInputs[peerID].push_back(string((&buf[0]),strlen));

            for(int b=0;b<strlen;b++)
            {
                checksum = checksum ^ buf[b];
            }
        }
    }

    needToSkipAhead=true;

    cout << "CHECKSUM: " << int(checksum) << endl;

    cout << "CLIENT INITIALIZED!\n";
}

void Client::updateSyncCheck()
{
	printf("UPDATING SYNC CHECK\n");
	for(int blockIndex=0;blockIndex<int(blocks.size());blockIndex++)
	{
		memcpy(
			syncCheckBlocks[blockIndex].data,
			blocks[blockIndex].data,
			blocks[blockIndex].size
			);
	}
}


bool printWhenCheck=false;

std::pair<bool,bool> Client::syncAndUpdate()
{
    for(
        map<RakNet::SystemAddress,vector< string > >::iterator it = unknownPeerInputs.begin();
        it != unknownPeerInputs.end();
        )
    {
        if(peerIDs.find(it->first)!=peerIDs.end())
        {
            int ID = peerIDs[it->first];
            for(int a=0;a<it->second.size();a++)
            {
                peerInputs[ID].push_back(it->second[a]);
            }
            map<RakNet::SystemAddress,vector< string > >::iterator itold = it;
            it++;
            unknownPeerInputs.erase(itold);
        }
        else
        {
            it++;
        }
    }

    RakSleep(0);
    if(printWhenCheck)
    {
	    printWhenCheck=false;
	    //printf("Checking for packets\n");
    }
    for(int packetCount=0;;packetCount++)
    {
        RakNet::Packet *p = rakInterface->Receive();
	if(!p)
	{
	    break;
	}
	unsigned char packetID = GetPacketIdentifier(p);
	bool hadToResync;

	switch(packetID)
	{
			case ID_CONNECTION_LOST:
				// Couldn't deliver a reliable packet - i.e. the other system was abnormally
				// terminated
				printf("ID_CONNECTION_LOST\n");
			case ID_DISCONNECTION_NOTIFICATION:
				// Connection lost normally
				printf("ID_DISCONNECTION_NOTIFICATION\n");
				if(peerIDs.find(p->systemAddress)!=peerIDs.end())
				{
                    if(peerIDs[p->systemAddress]==1)
                    {
                        //Server quit, we are done.
                        return std::pair<bool,bool>(false,false);
                    }
                    else
                    {
                        peerIDs.erase(p->systemAddress);
                    }
				}
				break;
			case ID_ALREADY_CONNECTED:
				printf("ID_ALREADY_CONNECTED\n");
			    return std::pair<bool,bool>(false,false);
				break;
			case ID_INCOMPATIBLE_PROTOCOL_VERSION:
				printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
			    return std::pair<bool,bool>(false,false);
				break;
			case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server telling the clients of another client disconnecting gracefully.  You can manually broadcast this in a peer to peer enviroment if you want.
				printf("ID_REMOTE_DISCONNECTION_NOTIFICATION\n");
				break;
			case ID_REMOTE_CONNECTION_LOST: // Server telling the clients of another client disconnecting forcefully.  You can manually broadcast this in a peer to peer enviroment if you want.
				printf("ID_REMOTE_CONNECTION_LOST\n");
				break;
			case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
				printf("ID_REMOTE_NEW_INCOMING_CONNECTION\n");
				break;
			case ID_CONNECTION_BANNED: // Banned from this server
				printf("We are banned from this server.\n");
			    return std::pair<bool,bool>(false,false);
				break;
			case ID_CONNECTION_ATTEMPT_FAILED:
				printf("Connection attempt failed\n");
			    return std::pair<bool,bool>(false,false);
				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				// Sorry, the server is full.  I don't do anything here but
				// A real app should tell the user
				printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
			    return std::pair<bool,bool>(false,false);
				break;

			case ID_INVALID_PASSWORD:
				printf("ID_INVALID_PASSWORD\n");
			    return std::pair<bool,bool>(false,false);
				break;

			case ID_CONNECTION_REQUEST_ACCEPTED:
				// This tells the client they have connected
				printf("ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s\n", p->systemAddress.ToString(true), p->guid.ToString());
				printf("My external address is %s\n", rakInterface->GetExternalID(p->systemAddress).ToString(true));
				break;

            case ID_ADVERTISE_SYSTEM:
                {
                    RakNet::SystemAddress sa;
                    memcpy(&(sa.binaryAddress),p->data+1,sizeof(uint32_t));
                    memcpy(&(sa.port),p->data+1+sizeof(uint32_t),sizeof(unsigned short));
                    RakNet::SystemAddress sa2 = ConnectBlocking(sa.ToString(false),sa.port);
                    if(sa2 != RakNet::UNASSIGNED_SYSTEM_ADDRESS)
                    {
                        cout << "Sending ID\n";
                        //Tell the new guy your ID
                        char buf[4096];
                        buf[0] = ID_SEND_PEER_ID;
                        memcpy(buf+1,&selfPeerID,sizeof(int));
                        rakInterface->Send(buf,1+sizeof(int),HIGH_PRIORITY,RELIABLE_ORDERED,0,sa2,false);
                    }
                    RakNet::RakNetGUID guid = rakInterface->GetGuidFromSystemAddress(sa);
                    cout << sa.binaryAddress << ':' << sa.port << endl;
                    cout << sa2.binaryAddress << ':' << sa2.port << endl;
                    if(sa2==RakNet::UNASSIGNED_SYSTEM_ADDRESS)
                    {
                        //Tell the boss that you can't accept this guy
                        char buf[4096];
                        buf[0] = ID_REJECT_NEW_HOST;
                        memcpy(buf+1,&(sa.binaryAddress),sizeof(uint32_t));
                        memcpy(buf+1+sizeof(uint32_t),&(sa.port),sizeof(unsigned short));
                        rakInterface->Send(buf,1+sizeof(uint32_t)+sizeof(unsigned short),HIGH_PRIORITY,RELIABLE_ORDERED,0,rakInterface->GetSystemAddressFromIndex(0),false);
                    }
                    else
                    {
                        //Tell the boss that you can't accept this guy
                        char buf[4096];
                        buf[0] = ID_ACCEPT_NEW_HOST;
                        memcpy(buf+1,&(sa.binaryAddress),sizeof(uint32_t));
                        memcpy(buf+1+sizeof(uint32_t),&(sa.port),sizeof(unsigned short));
                        rakInterface->Send(buf,1+sizeof(uint32_t)+sizeof(unsigned short),HIGH_PRIORITY,RELIABLE_ORDERED,0,rakInterface->GetSystemAddressFromIndex(0),false);
                    }

                }
                break;

            case ID_HOST_ACCEPTED:
                {
                    int peerID;
                    memcpy(&peerID,p->data+1,sizeof(int));
                    RakNet::SystemAddress sa;
                    memcpy(&(sa.binaryAddress),p->data+1+sizeof(int),sizeof(uint32_t));
                    memcpy(&(sa.port),p->data+1+sizeof(int)+sizeof(uint32_t),sizeof(unsigned short));
                    if(rakInterface->GetExternalID(p->systemAddress)==sa)
                    {
                        //This is me, set my own ID
                        selfPeerID = peerID;
                    }
                    else
                    {
                        //This is someone else, set their ID
                        peerIDs[sa] = peerID;
                    }
                    if(peerInputs.find(peerID)!=peerInputs.end())
                    {
                        printf("A PEER INPUT ALREADY EXITED, THIS IS WEIRD\n");
                    }
                    peerInputs[peerID] = vector<string>();
                }
                break;

			case ID_RESYNC_PARTIAL:
				memcpy(syncPtr,GetPacketData(p),GetPacketSize(p));
				syncPtr += GetPacketSize(p);
				//printf("GOT PARTIAL RESYNC\n");
				printWhenCheck=true;
				break;
			case ID_RESYNC_COMPLETE:
				printf("GOT COMPLETE RESYNC\n");
				memcpy(syncPtr,GetPacketData(p),GetPacketSize(p));
				syncPtr += GetPacketSize(p);
			        hadToResync = resync(compressedBuffer,int(syncPtr-compressedBuffer));

	                    //We have to return here because processing two syncs without a frame
           	            //in between can cause crashes
			    syncPtr = compressedBuffer;
			    if(firstResync || hadToResync)
			    {
                    printf("BEGINNING VIDEO SKIP\n");
                    needToSkipAhead=true;
				    firstResync=false;
				    return std::pair<bool,bool>(true,true);
			    }
			    else
			    {
				    return std::pair<bool,bool>(true,false);
			    }
			    break;
			case ID_CONST_DATA:
			    addConstData(GetPacketData(p),GetPacketSize(p));
			    break;
            case ID_CLIENT_INPUTS:
                if(peerIDs.find(p->systemAddress)==peerIDs.end() || peerInputs.find(peerIDs[p->systemAddress])==peerInputs.end())
                {
                    cout << __FILE__ << ":" << __LINE__ << " OOPS!!!!\n";
                }
                peerInputs[peerIDs[p->systemAddress]].push_back(string((char*)GetPacketData(p),(int)GetPacketSize(p)));
                break;
            case ID_SERVER_INPUTS:
            {
                peerInputs[1].push_back(string(((char*)GetPacketData(p)),(int)GetPacketSize(p)));
            }
            break;
            case ID_SETTINGS:
                memcpy(&secondsBetweenSync,p->data+1,sizeof(int));
			    break;
			default:
			    printf("GOT AN INVALID PACKET TYPE: %d\n",int(packetID));
			    return std::pair<bool,bool>(false,false);
	}

	rakInterface->DeallocatePacket(p);
    }

    return std::pair<bool,bool>(true,false);
}

extern unsigned char gotAnonymous;

bool Client::resync(unsigned char *data,int size)
{
	int uncompressedSize,compressedSize;
	memcpy(&uncompressedSize,data,sizeof(int));
	data += sizeof(int);
	memcpy(&compressedSize,data,sizeof(int));
	data += sizeof(int);
	uLongf destLen = MAX_ZLIB_BUF_SIZE;
	uncompress(uncompressedBuffer,&destLen,data,compressedSize);
	unsigned char *uncompressedPtr = uncompressedBuffer;


	//Check to see if the client is clean
	bool clientIsClean=true;

	int badBlockCount=0;
	int totalBlockCount=0;

            while(true)
            {
                int blockIndex;
		memcpy(
			&blockIndex,
			uncompressedPtr,
			sizeof(int)
		      );
		uncompressedPtr += sizeof(int);

                if(blockIndex==-1)
                {
                    //cout << "GOT TERMINAL BLOCK INDEX\n";
                    break;
                }

                if(blockIndex >= int(blocks.size()) || blockIndex<0)
                {
                    cout << "GOT AN INVALID BLOCK INDEX: " << blockIndex << endl;
                    break;
                }

                //boost::asio::read(*clientSocket,boost::asio::buffer(&clientSizeOfNextMessage, sizeof(int)));

                MemoryBlock &block = blocks[blockIndex];
                MemoryBlock &syncCheckBlock = syncCheckBlocks[blockIndex];
                MemoryBlock &staleBlock = staleBlocks[blockIndex];
                MemoryBlock &xorBlock = xorBlocks[blockIndex];

                //cout << "CLIENT: GOT MESSAGE FOR INDEX: " << blockIndex << endl;

                //if(clientSizeOfNextMessage!=block.size)
                //{
                //cout << "ERROR!: SIZE MISMATCH " << clientSizeOfNextMessage
                //<< " != " << block.size << endl;
                //}

		memcpy(
			xorBlock.data,
			uncompressedPtr,
			xorBlock.size
		      );
        	uncompressedPtr += xorBlock.size;
                //cout << "BYTES READ: " << (xorBlock.size-strm.avail_out) << endl;
                for(int a=0;a<block.size;a++)
                {
			totalBlockCount++;
			if(syncCheckBlock.data[a] != (xorBlock.data[a] ^ staleBlock.data[a]))
			{
				badBlockCount++;
				if(badBlockCount<50)
				{
					printf("BLOCK %d BYTE %d IS BAD\n",blockIndex,a);
					clientIsClean=false;
				}
				//break;
			}
                }
            }

	    if(clientIsClean)
	    {
		    printf("CLIENT IS CLEAN\n");
            for(int a=0;a<int(blocks.size());a++)
       	    {
                	memcpy(staleBlocks[a].data,syncCheckBlocks[a].data,syncCheckBlocks[a].size);
		    }
		    return false;
	    }

	    if(gotAnonymous)
	    {
	        printf("CLIENT IS DIRTY BUT HAD ANONYMOUS TIMER SO CAN'T FIX!\n");
		    return false;
	    }

	    printf("CLIENT IS DIRTY (%d bad blocks, %f%% of total)\n",badBlockCount,float(badBlockCount)*100.0f/totalBlockCount);
	    ui_popup_time(3, "You are out of sync with the server, resyncing...");

	//The client is not clean and needs to be resynced

            //If the client has predicted anything, erase the prediction
            for(int a=0;a<int(blocks.size());a++)
            {
                memcpy(blocks[a].data,staleBlocks[a].data,blocks[a].size);
            }

#if 1
	uncompressedPtr = uncompressedBuffer;
            unsigned char blockChecksum=0;
            unsigned char xorChecksum=0;
            unsigned char staleChecksum=0;
            while(true)
            {
                int blockIndex;
		memcpy(
			&blockIndex,
			uncompressedPtr,
			sizeof(int)
		      );
		uncompressedPtr += sizeof(int);

                if(blockIndex==-1)
                {
                    //cout << "GOT TERMINAL BLOCK INDEX\n";
                    break;
                }

                if(blockIndex >= int(blocks.size()) || blockIndex<0)
                {
                    cout << "GOT AN INVALID BLOCK INDEX: " << blockIndex << endl;
                    break;
                }

                //boost::asio::read(*clientSocket,boost::asio::buffer(&clientSizeOfNextMessage, sizeof(int)));

                MemoryBlock &block = blocks[blockIndex];
                MemoryBlock &staleBlock = staleBlocks[blockIndex];
                MemoryBlock &xorBlock = xorBlocks[blockIndex];

                //cout << "CLIENT: GOT MESSAGE FOR INDEX: " << blockIndex << endl;

                //if(clientSizeOfNextMessage!=block.size)
                //{
                //cout << "ERROR!: SIZE MISMATCH " << clientSizeOfNextMessage
                //<< " != " << block.size << endl;
                //}

		memcpy(
			xorBlock.data,
			uncompressedPtr,
			xorBlock.size
		      );
        uncompressedPtr += xorBlock.size;
                //cout << "BYTES READ: " << (xorBlock.size-strm.avail_out) << endl;
                for(int a=0;a<block.size;a++)
                {
                    block.data[a] = xorBlock.data[a] ^ staleBlock.data[a];
                    blockChecksum = blockChecksum ^ block.data[a];
                    xorChecksum = xorChecksum ^ xorBlock.data[a];
                    staleChecksum = staleChecksum ^ staleBlock.data[a];
                }
                memcpy(staleBlock.data,block.data,block.size);

                //cout << "CLIENT: FINISHED GETTING MESSAGE\n";
            }
            printf("BLOCK CHECKSUM: %d\n",int(blockChecksum));
            printf("XOR CHECKSUM: %d\n",int(xorChecksum));
            printf("STALE CHECKSUM: %d\n",int(staleChecksum));
#endif
	    return true;
}

void Client::addConstData(unsigned char *data,int size)
{
        //cout << "READING CONST BLOCK\n";
        constBlocks.push_back(MemoryBlock(0));

	    int blockSize;
	    memcpy(&blockSize,data,sizeof(int));
	    data += sizeof(int);
	    int compressedblockSize;
	    memcpy(&compressedblockSize,data,sizeof(int));
	    data += sizeof(int);

	    constBlocks.back().size = blockSize;
        constBlocks.back().data = (unsigned char*)malloc(blockSize);

        uLongf destLen = blockSize;

        uncompress(
            constBlocks.back().data,
            &destLen,
            data,
            size-(sizeof(int)*2)
            );

	if(constBlocks.back().data[0]>1)
	{
		printf("ERROR! GOT CONST BLOCK WITH ID OUT OF RANGE!!!\n");
	}
}

void Client::checkMatch(Server *server)
{
    static int errorCount=0;

    if(errorCount>=10)
        exit(1);

    if(blocks.size()!=server->getNumBlocks())
    {
        cout << "CLIENT AND SERVER ARE OUT OF SYNC (different block counts)\n";
        errorCount++;
        return;
    }

    for(int a=0;a<int(blocks.size());a++)
    {
        if(getMemoryBlock(a).size != server->getMemoryBlock(a).size)
        {
            cout << "CLIENT AND SERVER ARE OUT OF SYNC (different block sizes)\n";
            errorCount++;
            return;
        }

        if(memcmp(getMemoryBlock(a).data,server->getMemoryBlock(a).data,getMemoryBlock(a).size))
        {
            cout << "CLIENT AND SERVER ARE OUT OF SYNC\n";
            cout << "CLIENT BITCOUNT: " << getMemoryBlock(a).getBitCount()
                << " SERVER BITCOUNT: " << server->getMemoryBlock(a).getBitCount()
                << endl;
            errorCount++;
            return;
        }
    }
    cout << "CLIENT AND SERVER ARE IN SYNC\n";
}

int Client::getNumSessions()
{
    return rakInterface->NumberOfConnections();
}

void Client::sendInputs(const string &inputString)
{
    peerInputs[selfPeerID].push_back(inputString);
    char* dataToSend = (char*)malloc(inputString.length()+1);
    dataToSend[0] = ID_CLIENT_INPUTS;
    memcpy(dataToSend+1,inputString.c_str(),inputString.length());
        //cout << "SENDING MESSAGE WITH LENGTH: " << intSize << endl;
    rakInterface->Send(
		    dataToSend,
		    (int)(inputString.length()+1),
		    HIGH_PRIORITY,
		    RELIABLE_ORDERED,
		    ORDERING_CHANNEL_CLIENT_INPUTS,
		    RakNet::UNASSIGNED_SYSTEM_ADDRESS,
		    true
		   );
    free(dataToSend);
}


#include "RakPeerInterface.h"
#include "RakNetStatistics.h"
#include "RakNetTypes.h"
#include "BitStream.h"
#include "PacketLogger.h"
#include "RakNetTypes.h"
#include "GetTime.h"
#include "RakSleep.h"

#include "NSM_Server.h"

#include <assert.h>
#include <cstdio>
#include <cstring>
#include <stdlib.h>

Server *netServer=NULL;

volatile bool memoryBlocksLocked = false;

Server *createGlobalServer(string _username,unsigned short _port)
{
    cout << "Creating server on port " << _port << endl;
    netServer = new Server(_username,_port);
    return netServer;
}

void deleteGlobalServer()
{
    if(netServer) delete netServer;
    netServer = NULL;
}

#include "osdcore.h"
#include "emu.h"

// Copied from Multiplayer.cpp
// If the first byte is ID_TIMESTAMP, then we want the 5th byte
// Otherwise we want the 1st byte
extern unsigned char GetPacketIdentifier(RakNet::Packet *p);
extern unsigned char *GetPacketData(RakNet::Packet *p);
extern int GetPacketSize(RakNet::Packet *p);

unsigned char compressedBuffer[MAX_ZLIB_BUF_SIZE];
unsigned char syncBuffer[MAX_ZLIB_BUF_SIZE];
unsigned char uncompressedBuffer[MAX_ZLIB_BUF_SIZE];

Server::Server(string username,int _port)
    :
    Common(username),
    port(_port),
    lastUsedPeerID(1),
    maxPeerID(4)
{
    rakInterface = RakNet::RakPeerInterface::GetInstance();

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    firstSync=true;

    selfPeerID=1;
    peerIDs[RakNet::UNASSIGNED_SYSTEM_ADDRESS] = 1;
    peerNames[1] = username;
    peerInputs[1] = vector<string>();

    syncTime = getTimeSinceStartup();
}

Server::~Server()
{
    // Be nice and let the server know we quit.
    rakInterface->Shutdown(300);

    // We're done with the network
    RakNet::RakPeerInterface::DestroyInstance(rakInterface);
}

void Server::acceptPeer(RakNet::SystemAddress saToAccept)
{
    printf("ACCEPTED PEER\n");
    if(acceptedPeers.size()>=maxPeerID-1)
    {
        //Whoops! Someone took the last spot
        rakInterface->CloseConnection(saToAccept,true);
        return;
    }

    //Accept this host
    acceptedPeers.push_back(saToAccept);
    char buf[4096];
    buf[0] = ID_HOST_ACCEPTED;
    int assignID=-1;
    for(
        std::map<RakNet::SystemAddress,int>::iterator it = deadPeerIDs.begin();
        it != deadPeerIDs.end();
        it++
    )
    {
        if(it->first==saToAccept)
        {
            assignID = it->second;
        }
    }
    if(assignID==-1)
    {
        bool usingNextPeerID=true;
        while(usingNextPeerID)
        {
            //Peer ID's are 1-based between 1 and maxPeerID inclusive, with 1 being reserved for the server
            lastUsedPeerID = (lastUsedPeerID+1)%(maxPeerID+1);
            if(!lastUsedPeerID) lastUsedPeerID=2;
            usingNextPeerID=false;

            for(
                std::map<RakNet::SystemAddress,int>::iterator it = peerIDs.begin();
                it != peerIDs.end();
                it++
            )
            {
                if(it->second==lastUsedPeerID)
                {
                    usingNextPeerID=true;
                    break;
                }
            }
            if(!usingNextPeerID)
            {
                //We took a dead person's ID, delete their history
                for(
                    std::map<RakNet::SystemAddress,int>::iterator it = deadPeerIDs.begin();
                    it != deadPeerIDs.end();
                )
                {
                    if(it->second==lastUsedPeerID)
                    {
                        std::map<RakNet::SystemAddress,int>::iterator itold = it;
                        it++;
                        deadPeerIDs.erase(itold);
                    }
                    else
                    {
                        it++;
                    }
                }
            }
        }
        assignID = lastUsedPeerID;
    }
    peerIDs[saToAccept] = assignID;
    peerNames[assignID] = candidateNames[saToAccept];
    candidateNames.erase(candidateNames.find(saToAccept));
    peerInputs[assignID] = vector<string>();

    memcpy(buf+1,&assignID,sizeof(int));
    memcpy(buf+1+sizeof(int),&(saToAccept.binaryAddress),sizeof(uint32_t));
    memcpy(buf+1+sizeof(int)+sizeof(uint32_t),&(saToAccept.port),sizeof(unsigned short));
    strcpy(
           buf+1+sizeof(int)+sizeof(uint32_t)+sizeof(unsigned short),
           peerNames[assignID].c_str()
           );
    rakInterface->Send(
        buf,
        1+sizeof(int)+sizeof(uint32_t)+sizeof(unsigned short)+strlen(peerNames[assignID].c_str())+1, //add 1 so we get the \0 at the end
        HIGH_PRIORITY,
        RELIABLE_ORDERED,
        ORDERING_CHANNEL_SYNC,
        RakNet::UNASSIGNED_SYSTEM_ADDRESS,
        true
    );

    //Perform initial sync with player
    initialSync(saToAccept);
}

void Server::removePeer(RakNet::SystemAddress sa)
{
    //Add peer to the dead peer list
    for(
        std::map<RakNet::SystemAddress,int>::iterator it = peerIDs.begin();
        it != peerIDs.end();
    )
    {
        if(it->first==sa)
        {
            deadPeerIDs[sa] = it->second;
            std::map<RakNet::SystemAddress,int>::iterator itold = it;
            it++;
            peerIDs.erase(itold);
        }
        else
        {
            it++;
        }
    }
    for(int a=0; a<(int)acceptedPeers.size(); a++)
    {
        if(acceptedPeers[a]==sa)
        {
            acceptedPeers.erase(acceptedPeers.begin()+a);
            a--;
        }
    }
    if(waitingForAcceptFrom.find(sa)!=waitingForAcceptFrom.end())
    {
        waitingForAcceptFrom.erase(waitingForAcceptFrom.find(sa));
    }
    else
    {
        for(std::map<RakNet::SystemAddress,std::vector<RakNet::SystemAddress> >::iterator it = waitingForAcceptFrom.begin();
                it != waitingForAcceptFrom.end();
           )
        {
            for(int a=0; a<(int)it->second.size(); a++)
            {
                if(it->second[a]==sa)
                {
                    it->second.erase(it->second.begin()+a);
                    a--;
                }
            }
            if(it->second.empty())
            {
                //A peer is now accepted because the person judging them disconnected
                RakNet::SystemAddress accpetedPeer = it->first;
                std::map<RakNet::SystemAddress,std::vector<RakNet::SystemAddress> >::iterator oldit = it;
                it++;
                waitingForAcceptFrom.erase(oldit);
                acceptPeer(accpetedPeer);
            }
            else
            {
                it++;
            }
        }
    }
}

bool Server::initializeConnection()
{
    RakNet::SocketDescriptor sd(0,0);
    printf("PORT: %d\n",port);
    sd.port = port;
    RakNet::StartupResult retval = rakInterface->Startup(512,&sd,1);
    rakInterface->SetMaximumIncomingConnections(512);
    rakInterface->SetIncomingPassword("MAME",(int)strlen("MAME"));
    rakInterface->SetTimeoutTime(30000,RakNet::UNASSIGNED_SYSTEM_ADDRESS);
    rakInterface->SetOccasionalPing(true);
    rakInterface->SetUnreliableTimeout(1000);

    if(retval != RakNet::RAKNET_STARTED)
    {
        printf("Server failed to start. Terminating\n");
        return false;
    }

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

extern attotime globalCurtime;
extern bool waitingForClientCatchup;

void Server::initialSync(const RakNet::SystemAddress &sa)
{
    unsigned char checksum = 0;

    waitingForClientCatchup=true;

    RakNet::BitStream uncompressedStream;
    //unsigned char *data = uncompressedBuffer;

    uncompressedStream.Write(syncTime);

    uncompressedStream.Write(globalCurtime);

    if(getSecondsBetweenSync())
    {
    while(memoryBlocksLocked)
    {
        ;
    }
    memoryBlocksLocked=true;
    cout << "IN CRITICAL SECTION\n";
    cout << "SERVER: Sending initial snapshot\n";

    int numBlocks = int(blocks.size());
    cout << "NUMBLOCKS: " << numBlocks << endl;
    uncompressedStream.Write(numBlocks);

    // NOTE: The server must send stale data to the client for the first time
    // So that future syncs will be accurate
    for(int blockIndex=0; blockIndex<int(staleBlocks.size()); blockIndex++)
    {
        //cout << "BLOCK SIZE FOR INDEX " << blockIndex << ": " << staleBlocks[blockIndex].size << endl;
        uncompressedStream.Write(staleBlocks[blockIndex].size);
        uncompressedStream.WriteBits(staleBlocks[blockIndex].data,staleBlocks[blockIndex].size*8);

        for(int a=0; a<staleBlocks[blockIndex].size; a++)
        {
            checksum = checksum ^ staleBlocks[blockIndex].data[a];
            }
        }
    }

    int numConstBlocks = int(constBlocks.size());
    uncompressedStream.Write(numConstBlocks);

    // NOTE: The server must send stale data to the client for the first time
    // So that future syncs will be accurate
    for(int blockIndex=0; blockIndex<int(constBlocks.size()); blockIndex++)
    {
        //cout << "BLOCK SIZE FOR INDEX " << blockIndex << ": " << constBlocks[blockIndex].size << endl;
        uncompressedStream.Write(constBlocks[blockIndex].size);
        uncompressedStream.WriteBits(constBlocks[blockIndex].data,constBlocks[blockIndex].size*8);
    }

    for(
        map<int,vector< string > >::iterator it = peerInputs.begin();
        it != peerInputs.end();
        it++
    )
    {
        uncompressedStream.Write(it->first);
        uncompressedStream.Write(int(oldPeerInputs[it->first].size()) + int(it->second.size()));
        for(int a=0; a<(int)it->second.size(); a++)
        {
            uncompressedStream.Write(int(it->second[a].length()));
            uncompressedStream.WriteBits((const unsigned char*)it->second[a].c_str(),it->second[a].length()*8);

            for(int b=0;b<it->second[a].length();b++)
            {
                checksum = checksum ^ it->second[a][b];
            }
        }
        for(int a=0; a<int(oldPeerInputs[it->first].size()); a++)
        {
            uncompressedStream.Write(int(oldPeerInputs[it->first][a].length()));
            uncompressedStream.WriteBits((const unsigned char*)oldPeerInputs[it->first][a].c_str(),oldPeerInputs[it->first][a].length()*8);

            for(int b=0;b<oldPeerInputs[it->first][a].length();b++)
            {
                checksum = checksum ^ oldPeerInputs[it->first][a][b];
            }
        }
    }
    uncompressedStream.Write(int(-1));

    cout << "CHECKSUM: " << int(checksum) << endl;

    //The application should ensure that this value be at least (sourceLen 1.001) + 12.
    //JJG: Doing more than that to account for header and provide some padding
    vector<unsigned char> compressedInitialSyncBuffer(uncompressedStream.GetNumberOfBytesUsed()*1.01 + 128, '\0');

    uLongf compressedSizeLong = compressedInitialSyncBuffer.size();

    compress2(
        (&compressedInitialSyncBuffer[0])+sizeof(int)+sizeof(int),
        &compressedSizeLong,
        uncompressedStream.GetData(),
        uncompressedStream.GetNumberOfBytesUsed(),
        9
    );

    int uncompressedSize = (int)uncompressedStream.GetNumberOfBytesUsed();
    memcpy((&compressedInitialSyncBuffer[0]),&uncompressedSize,sizeof(int));

    int compressedSize = (int)compressedSizeLong;
    memcpy((&compressedInitialSyncBuffer[0])+sizeof(int),&compressedSize,sizeof(int));

    printf("COMPRESSED SIZE: %d\n",compressedSize/8/1024);

    /*
    rakInterface->Send(
    	(char*)(&compressedInitialSyncBuffer[0]),
    	compressedSize+1+sizeof(int),
    	HIGH_PRIORITY,
    	RELIABLE_ORDERED,
    	ORDERING_CHANNEL_SYNC,
    	guid,
    	false
           );
    */

    //
    unsigned char *sendPtr = (&compressedInitialSyncBuffer[0]);
    int sizeRemaining = compressedSize+sizeof(int)+sizeof(int);
    printf("INITIAL STATE SIZE: %dKB\n",sizeRemaining/1024);
    int packetSize = max(256,min(512,sizeRemaining/100));
    while(sizeRemaining>packetSize)
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
            RELIABLE_ORDERED,
            ORDERING_CHANNEL_SYNC,
            sa,
            false
        );
        RakSleep(10);
    }
    {
        RakNet::BitStream bitStreamPart(65536);
        unsigned char header = ID_INITIAL_SYNC_COMPLETE;
        bitStreamPart.WriteBits((const unsigned char*)&header,8*sizeof(unsigned char));
        bitStreamPart.WriteBits((const unsigned char*)sendPtr,8*sizeRemaining);
        rakInterface->Send(
            &bitStreamPart,
            HIGH_PRIORITY,
            RELIABLE_ORDERED,
            ORDERING_CHANNEL_SYNC,
            sa,
            false
        );
        RakSleep(10);
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
    for(
        map<RakNet::SystemAddress,vector< string > >::iterator it = unknownPeerInputs.begin();
        it != unknownPeerInputs.end();
        )
    {
        cout << "GOT UNKNOWN PEER INPUT\n";
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

    //cout << "SERVER TIME: " << RakNet::GetTimeMS()/1000.0f/60.0f << endl;
    //printf("Updating server\n");
    RakSleep(0);
    RakNet::Packet *p;
    for (p=rakInterface->Receive(); p; rakInterface->DeallocatePacket(p), p=rakInterface->Receive())
    {
        // We got a packet, get the identifier with our handy function
        unsigned char packetIdentifier = GetPacketIdentifier(p);

        //printf("GOT PACKET %d\n",int(packetIdentifier));

        // Check if this is a network message packet
        switch (packetIdentifier)
        {
        case ID_CONNECTION_LOST:
            // Couldn't deliver a reliable packet - i.e. the other system was abnormally
            // terminated
        case ID_DISCONNECTION_NOTIFICATION:
            // Connection lost normally
            printf("ID_DISCONNECTION_NOTIFICATION from %s\n", p->systemAddress.ToString(true));
            removePeer(p->systemAddress);
            break;


        case ID_NEW_INCOMING_CONNECTION:
            // Somebody connected.  We have their IP now
            printf("ID_NEW_INCOMING_CONNECTION from %s with GUID %s\n", p->systemAddress.ToString(true), p->guid.ToString());
            break;

        case ID_CLIENT_INFO:
            cout << "GOT ID_CLIENT_INFO\n";
            //This client is requesting candidacy, set their info
            {
                char buf[4096];
                strcpy(buf,(char*)(p->data+1));
                candidateNames[p->systemAddress] = buf;
            }

            //Find a session index for the player
            {
                char buf[4096];
                buf[0] = ID_SETTINGS;
                memcpy(buf+1,&secondsBetweenSync,sizeof(int));
                strcpy(buf+1+sizeof(int),username.c_str());
                rakInterface->Send(
                    buf,
                    1+sizeof(int)+username.length()+1,
                    HIGH_PRIORITY,
                    RELIABLE_ORDERED,
                    ORDERING_CHANNEL_SYNC,
                    p->guid,
                    false
                );
            }
            if(acceptedPeers.size()>=maxPeerID-1)
            {
                //Sorry, no room
                rakInterface->CloseConnection(p->systemAddress,true);
            }
            else if(acceptedPeers.size())
            {
                printf("Asking other peers to accept %s\n",p->systemAddress.ToString());
                waitingForAcceptFrom[p->systemAddress] = std::vector<RakNet::SystemAddress>();
                for(int a=0; a<acceptedPeers.size(); a++)
                {
                    RakNet::SystemAddress sa = acceptedPeers[a];
                    waitingForAcceptFrom[p->systemAddress].push_back(sa);
                    cout << "SENDING ADVERTIZE TO " << sa.ToString() << endl;
                    if(sa != RakNet::UNASSIGNED_SYSTEM_ADDRESS && sa != p->systemAddress)
                    {
                        char buf[4096];
                        buf[0] = ID_ADVERTISE_SYSTEM;
                        memcpy(buf+1,&(p->systemAddress.binaryAddress),sizeof(uint32_t));
                        memcpy(buf+1+sizeof(uint32_t),&(p->systemAddress.port),sizeof(unsigned short));
                        rakInterface->Send(buf,1+sizeof(uint32_t)+sizeof(unsigned short),HIGH_PRIORITY,RELIABLE_ORDERED,ORDERING_CHANNEL_SYNC,sa,false);
                    }
                }
                printf("Asking other peers to accept\n");
            }
            else
            {
                //First client, automatically accept
                acceptPeer(p->systemAddress);
            }
            break;

        case ID_INCOMPATIBLE_PROTOCOL_VERSION:
            printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
            break;

        case ID_ACCEPT_NEW_HOST:
        {
            printf("Accepting new host\n");
            RakNet::SystemAddress saToAccept;
            memcpy(&(saToAccept.binaryAddress),p->data+1,sizeof(uint32_t));
            memcpy(&(saToAccept.port),p->data+1+sizeof(uint32_t),sizeof(unsigned short));
            for(int a=0; a<waitingForAcceptFrom[saToAccept].size(); a++)
            {
                if(waitingForAcceptFrom[saToAccept][a]==p->systemAddress)
                {
                    waitingForAcceptFrom[saToAccept].erase(waitingForAcceptFrom[saToAccept].begin()+a);
                    break;
                }
            }
            if(waitingForAcceptFrom[saToAccept].empty())
            {
                cout << "Accepting: " << saToAccept.ToString() << endl;
                waitingForAcceptFrom.erase(waitingForAcceptFrom.find(saToAccept));
                acceptPeer(saToAccept);
            }
        }
        break;

        case ID_REJECT_NEW_HOST:
        {
            RakNet::SystemAddress saToAccept;
            memcpy(&(saToAccept.binaryAddress),p->data+1,sizeof(uint32_t));
            memcpy(&(saToAccept.port),p->data+1+sizeof(uint32_t),sizeof(unsigned short));
            printf("Rejecting new host %s\n",saToAccept.ToString());
            if(waitingForAcceptFrom.find(saToAccept)==waitingForAcceptFrom.end())
                printf("Could not find waitingForAcceptFrom for this SA, weird");
            else
                waitingForAcceptFrom.erase(waitingForAcceptFrom.find(saToAccept));
            rakInterface->CloseConnection(saToAccept,true);
        }
        break;

        case ID_CLIENT_INPUTS:
            if(peerIDs.find(p->systemAddress)==peerIDs.end() || peerInputs.find(peerIDs[p->systemAddress])==peerInputs.end())
            {
                cout << __FILE__ << ":" << __LINE__ << " OOPS!!!!\n";
            }
            peerInputs[peerIDs[p->systemAddress]].push_back(string((char*)GetPacketData(p),(int)GetPacketSize(p)));
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

    syncTime = getTimeSinceStartup();

    int bytesSynched=0;
    //cout << "IN CRITICAL SECTION\n";
    //cout << "SERVER: Syncing with clients\n";
    bool anyDirty=false;
    unsigned char blockChecksum=0;
    unsigned char xorChecksum=0;
    unsigned char staleChecksum=0;
    unsigned char *uncompressedPtr = uncompressedBuffer;
    for(int blockIndex=0; blockIndex<int(blocks.size()); blockIndex++)
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
        for(int a=0; a<block.size; a++)
        {
            xorBlock.data[a] = block.data[a] ^ staleBlock.data[a];
            if(xorBlock.data[a]) dirty=true;
        }
        if(dirty)
        {
            for(int a=0; a<block.size; a++)
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
            SYNC_PACKET_SIZE = max(1,compressedSize/60/syncTransferSeconds);

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
    else
    {
        printf("No dirty blocks found\n");
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

void Server::sendInputs(const string &inputString)
{
    peerInputs[selfPeerID].push_back(inputString);
    char* dataToSend = (char*)malloc(inputString.length()+1);
    dataToSend[0] = ID_SERVER_INPUTS;
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


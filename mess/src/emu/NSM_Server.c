#define DISABLE_EMUALLOC

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
#include <algorithm>
#include <stdlib.h>

#include "emu.h"

#include "unicode.h"
#include "ui.h"
#include "osdcore.h"
#include "emuopts.h"

Server *netServer=NULL;
Server server;

volatile bool memoryBlocksLocked = false;

Server *createGlobalServer(string _username,unsigned short _port)
{
    cout << "Creating server on port " << _port << endl;
    server = Server(_username,_port);
    netServer = &server;
    return netServer;
}

void deleteGlobalServer()
{
    if(netServer)
    {
	    netServer->shutdown();
    }
    netServer = NULL;
}

// Copied from Multiplayer.cpp
// If the first byte is ID_TIMESTAMP, then we want the 5th byte
// Otherwise we want the 1st byte
extern unsigned char GetPacketIdentifier(RakNet::Packet *p);
extern unsigned char *GetPacketData(RakNet::Packet *p);
extern int GetPacketSize(RakNet::Packet *p);

#define INITIAL_BUFFER_SIZE (1024*1024*32)
unsigned char *compressedBuffer = (unsigned char*)malloc(INITIAL_BUFFER_SIZE);
int compressedBufferSize = INITIAL_BUFFER_SIZE;
unsigned char *syncBuffer = (unsigned char*)malloc(INITIAL_BUFFER_SIZE);
int syncBufferSize = INITIAL_BUFFER_SIZE;
unsigned char *uncompressedBuffer = (unsigned char*)malloc(INITIAL_BUFFER_SIZE);
int uncompressedBufferSize = INITIAL_BUFFER_SIZE;

Server::Server(string username,int _port)
    :
    Common(username),
    port(_port),
    maxPeerID(10),
    syncHappend(false)
{
    rakInterface = RakNet::RakPeerInterface::GetInstance();

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    firstSync=true;

    selfPeerID=1;
    peerIDs[rakInterface->GetMyGUID()] = 1;
    peerNames[1] = username;
    peerInputs[1] = vector<string>();

    syncTime = getTimeSinceStartup();
}

void Server::shutdown()
{
    // Be nice and let the server know we quit.
    rakInterface->Shutdown(300);

    // We're done with the network
    RakNet::RakPeerInterface::DestroyInstance(rakInterface);
}

void Server::acceptPeer(RakNet::SystemAddress saToAccept,running_machine *machine)
{
    RakNet::RakNetGUID guidToAccept = rakInterface->GetGuidFromSystemAddress(saToAccept);
    printf("ACCEPTED PEER\n");
    if(acceptedPeers.size()>=maxPeerID-1)
    {
        //Whoops! Someone took the last spot
        rakInterface->CloseConnection(saToAccept,true);
        return;
    }

    //Accept this host
    acceptedPeers.push_back(guidToAccept);
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
    int lastUsedPeerID=1;
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
                std::map<RakNet::RakNetGUID,int>::iterator it = peerIDs.begin();
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
    peerIDs[guidToAccept] = assignID;
    peerNames[assignID] = candidateNames[saToAccept];
    candidateNames.erase(candidateNames.find(saToAccept));
    if(peerInputs.find(assignID)==peerInputs.end())
        peerInputs[assignID] = vector<string>();

    printf("ASSIGNING ID %d TO NEW CLIENT\n",assignID);
    memcpy(buf+1,&assignID,sizeof(int));
    memcpy(buf+1+sizeof(int),&(guidToAccept.g),sizeof(uint64_t));
    strcpy(
           buf+1+sizeof(int)+sizeof(uint64_t),
           peerNames[assignID].c_str()
           );
    rakInterface->Send(
        buf,
        1+sizeof(int)+sizeof(uint64_t)+strlen(peerNames[assignID].c_str())+1, //add 1 so we get the \0 at the end
        HIGH_PRIORITY,
        RELIABLE_ORDERED,
        ORDERING_CHANNEL_SYNC,
        RakNet::UNASSIGNED_SYSTEM_ADDRESS,
        true
    );

    //Perform initial sync with player
    initialSync(saToAccept,machine);
}

void Server::removePeer(RakNet::RakNetGUID guid,running_machine *machine)
{
    RakNet::SystemAddress sa = rakInterface->GetSystemAddressFromGuid(guid);

    if(waitingForAcceptFrom.find(sa)!=waitingForAcceptFrom.end())
    {
        waitingForAcceptFrom.erase(waitingForAcceptFrom.find(sa));
    }
    //else
    {
        for(int a=0; a<(int)acceptedPeers.size(); a++)
        {
            if(acceptedPeers[a]==guid)
            {
                acceptedPeers.erase(acceptedPeers.begin()+a);
                //Add peer to the dead peer list
                for(
                    std::map<RakNet::RakNetGUID,int>::iterator it = peerIDs.begin();
                    it != peerIDs.end();
                )
                {
                    if(it->first==guid)
                    {
                        deadPeerIDs[sa] = it->second;
                        std::map<RakNet::RakNetGUID,int>::iterator itold = it;
                        it++;
                        peerIDs.erase(itold);
                    }
                    else
                    {
                        it++;
                    }
                }
                break;
            }
        }
        for(std::map<RakNet::SystemAddress,std::vector<RakNet::RakNetGUID> >::iterator it = waitingForAcceptFrom.begin();
                it != waitingForAcceptFrom.end();
           )
        {
            for(int a=0; a<(int)it->second.size(); a++)
            {
                if(it->second[a]==guid)
                {
                    it->second.erase(it->second.begin()+a);
                    a--;
                }
            }
            if(it->second.empty())
            {
                //A peer is now accepted because the person judging them disconnected
                RakNet::SystemAddress accpetedPeer = it->first;
                std::map<RakNet::SystemAddress,std::vector<RakNet::RakNetGUID> >::iterator oldit = it;
                it++;
                waitingForAcceptFrom.erase(oldit);
                acceptPeer(accpetedPeer,machine);
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
    rakInterface->SetTimeoutTime(5000,RakNet::UNASSIGNED_SYSTEM_ADDRESS);
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
    staleBlocks.push_back(MemoryBlock(size));
    initialBlocks.push_back(MemoryBlock(size));
    return blocks.back();
}

vector<MemoryBlock> Server::createMemoryBlock(unsigned char *ptr,int size)
{
    if(blocks.size()==293)
    {
        //throw emu_fatalerror("OOPS");
    }
    vector<MemoryBlock> retval;
    const int BYTES_IN_MB=1024*1024;
    if(size>BYTES_IN_MB)
    {
        for(int a=0;;a+=BYTES_IN_MB)
        {
            if(a+BYTES_IN_MB>=size)
            {
                vector<MemoryBlock> tmp = createMemoryBlock(ptr+a,size-a);
                retval.insert(retval.end(),tmp.begin(),tmp.end());
                break;
            }
            else
            {
                vector<MemoryBlock> tmp = createMemoryBlock(ptr+a,BYTES_IN_MB);
                retval.insert(retval.end(),tmp.begin(),tmp.end());
            }
        }
        return retval;
    }
    //printf("Creating memory block at %X with size %d\n",ptr,size);
    blocks.push_back(MemoryBlock(ptr,size));
    staleBlocks.push_back(MemoryBlock(size));
    initialBlocks.push_back(MemoryBlock(size));
    retval.push_back(blocks.back());
    return retval;
}

extern attotime globalCurtime;
extern bool waitingForClientCatchup;
extern attotime oldInputTime;

void Server::initialSync(const RakNet::SystemAddress &sa,running_machine *machine)
{
    unsigned char checksum = 0;

    waitingForClientCatchup=true;
    machine->osd().pauseAudio(true);

    int syncBytes;
    {
    RakNet::BitStream uncompressedStream;

    uncompressedStream.Write(startupTime);

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
        for(int blockIndex=0; blockIndex<int(initialBlocks.size()); blockIndex++)
        {
            //cout << "BLOCK SIZE FOR INDEX " << blockIndex << ": " << staleBlocks[blockIndex].size << endl;
            uncompressedStream.Write(initialBlocks[blockIndex].size);

            cout << "BLOCK " << blockIndex << ": ";
            for(int a=0; a<staleBlocks[blockIndex].size; a++)
            {
                checksum = checksum ^ staleBlocks[blockIndex].data[a];
                //cout << int(staleBlocks[blockIndex].data[a]) << ' ';
                unsigned char value = initialBlocks[blockIndex].data[a] ^ staleBlocks[blockIndex].data[a];
                uncompressedStream.WriteBits(&value,8);
            }
            cout << int(checksum) << endl;
        }
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
    uncompressedStream.Write(checksum);
    cout << "CHECKSUM: " << int(checksum) << endl;

    if(uncompressedBufferSize<uncompressedStream.GetNumberOfBytesUsed()+sizeof(int))
    {
        uncompressedBufferSize = uncompressedStream.GetNumberOfBytesUsed()+sizeof(int);
	free(uncompressedBuffer);
        uncompressedBuffer = (unsigned char*)malloc(uncompressedBufferSize);
        if(!uncompressedBuffer)
        {
            cout << __FILE__ << ":" << __LINE__ << " OUT OF MEMORY\n";
            exit(1);
        }
    }
    syncBytes = uncompressedStream.GetNumberOfBytesUsed();
    memcpy(uncompressedBuffer,uncompressedStream.GetData(),uncompressedStream.GetNumberOfBytesUsed());

    int uncompressedStateSize = (int)uncompressedStream.GetNumberOfBytesUsed();
    printf("INITIAL UNCOMPRESSED STATE SIZE: %d\n",uncompressedStateSize);
    }

    cout << "PUTTING NVRAM AT LOCATION " << syncBytes << endl;

	// open the file; if it exists, call everyone to read from it
	emu_file file(machine->options().nvram_directory(), OPEN_FLAG_READ);
	if (file.open(machine->basename(), ".nv") == FILERR_NONE && file.size()<=1024*1024*64) //Don't bother sending huge NVRAM's
    {
        int nvramSize = file.size();
        if(uncompressedBufferSize<syncBytes+sizeof(int)+nvramSize)
        {
            uncompressedBufferSize = syncBytes+sizeof(int)+nvramSize;
	    free(uncompressedBuffer);
            uncompressedBuffer = (unsigned char*)malloc(uncompressedBufferSize);
            if(!uncompressedBuffer)
            {
                cout << __FILE__ << ":" << __LINE__ << " OUT OF MEMORY\n";
                exit(1);
            }
        }
        cout << "SENDING NVRAM OF SIZE: " << nvramSize << endl;
        memcpy(uncompressedBuffer+syncBytes,&nvramSize,sizeof(int));
        file.read(uncompressedBuffer+syncBytes+sizeof(int),nvramSize);
        file.close();
        syncBytes += sizeof(int) + nvramSize;
    }
	else
	{
	    int dummy=0;
        memcpy(uncompressedBuffer+syncBytes,&dummy,sizeof(int));
        syncBytes += sizeof(int);
	}

    cout << "BYTES USED: " << syncBytes << endl;

    //The application should ensure that this value be at least (sourceLen 1.001) + 12.
    //JJG: Doing more than that to account for header and provide some padding
    //vector<unsigned char> compressedInitialSyncBuffer(
            //sizeof(int)*2 + lzmaGetMaxCompressedSize(uncompressedStream.GetNumberOfBytesUsed()), '\0');

    //JJG: Take a risk and assume the compressed size will be smaller than the uncompressed size
    int newCompressedSize = max(1024*1024,int(sizeof(int)*2 + lzmaGetMaxCompressedSize(syncBytes)));
    if(compressedBufferSize < newCompressedSize )
    {
        compressedBufferSize = newCompressedSize;
        cout << "NEW COMPRESSED BUFFER SIZE: " << compressedBufferSize << endl;
	free(compressedBuffer);
        compressedBuffer = (unsigned char*)malloc(compressedBufferSize);
        if(!compressedBuffer)
        {
            cout << __FILE__ << ":" << __LINE__ << " OUT OF MEMORY\n";
            exit(1);
        }
    }

    int compressedSizeLong = compressedBufferSize;

    //FILE *stateptr = fopen("initialState.dat","wb");
    //fwrite(uncompressedStream.GetData(),uncompressedStream.GetNumberOfBytesUsed(),1,stateptr);
    //fclose(stateptr);

    lzmaCompress(
        compressedBuffer+sizeof(int)+sizeof(int),
        compressedSizeLong,
        uncompressedBuffer,
        syncBytes,
        6
    );

    int uncompressedSize = (int)syncBytes;
    memcpy(compressedBuffer,&uncompressedSize,sizeof(int));

    int compressedSize = (int)compressedSizeLong;
    memcpy(compressedBuffer+sizeof(int),&compressedSize,sizeof(int));

    printf("INITIAL UNCOMPRESSED SIZE: %d\n",uncompressedSize);
    printf("INITIAL COMPRESSED SIZE: %d\n",compressedSize);

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
    unsigned char *sendPtr = compressedBuffer;
    int sizeRemaining = compressedSize+sizeof(int)+sizeof(int);
    printf("INITIAL COMPRESSED SIZE: %dKB\n",sizeRemaining/1024);
    int packetSize = max(256,min(1024,sizeRemaining/100));

    oldInputTime.seconds = oldInputTime.attoseconds = 0;

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
        ui_update_and_render(*machine, &machine->render().ui_container());
        machine->osd().update(false);
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
        ui_update_and_render(*machine, &machine->render().ui_container());
        machine->osd().update(false);
        RakSleep(10);
    }
    //

    cout << "FINISHED SENDING BLOCKS TO CLIENT\n";
    cout << "SERVER: Done with initial snapshot\n";
    cout << "OUT OF CRITICAL AREA\n";
    cout.flush();
    memoryBlocksLocked=false;
}

void Server::update(running_machine *machine)
{
    for(
        map<RakNet::RakNetGUID,vector< string > >::iterator it = unknownPeerInputs.begin();
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
            map<RakNet::RakNetGUID,vector< string > >::iterator itold = it;
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
            removePeer(p->guid,machine);
            break;


        case ID_NEW_INCOMING_CONNECTION:
            // Somebody connected.  We have their IP now
            printf("ID_NEW_INCOMING_CONNECTION from %s with GUID %s\n", p->systemAddress.ToString(true), p->guid.ToString());
            if(syncHappend)
            {
                //Sorry, too late
                //rakInterface->CloseConnection(p->systemAddress,true);
            }
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
                waitingForAcceptFrom[p->systemAddress] = std::vector<RakNet::RakNetGUID>();
                for(int a=0; a<acceptedPeers.size(); a++)
                {
                    RakNet::RakNetGUID guid = acceptedPeers[a];
                    waitingForAcceptFrom[p->systemAddress].push_back(guid);
                    cout << "SENDING ADVERTIZE TO " << guid.ToString() << endl;
                    char buf[4096];
                    buf[0] = ID_ADVERTISE_SYSTEM;
                    strcpy(buf+1,p->systemAddress.ToString(true));
                    rakInterface->Send(buf,1+strlen(p->systemAddress.ToString(true))+1,HIGH_PRIORITY,RELIABLE_ORDERED,ORDERING_CHANNEL_SYNC,guid,false);
                }
                printf("Asking other peers to accept\n");
            }
            else
            {
                //First client, automatically accept
                acceptPeer(p->systemAddress,machine);
            }
            break;

        case ID_INCOMPATIBLE_PROTOCOL_VERSION:
            printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
            break;

        case ID_ACCEPT_NEW_HOST:
        {
            printf("Accepting new host\n");
            RakNet::SystemAddress saToAccept;
            saToAccept.SetBinaryAddress(((char*)p->data)+1);
            for(int a=0; a<waitingForAcceptFrom[saToAccept].size(); a++)
            {
                if(waitingForAcceptFrom[saToAccept][a]==p->guid)
                {
                    waitingForAcceptFrom[saToAccept].erase(waitingForAcceptFrom[saToAccept].begin()+a);
                    break;
                }
            }
            if(waitingForAcceptFrom[saToAccept].empty())
            {
                cout << "Accepting: " << saToAccept.ToString() << endl;
                waitingForAcceptFrom.erase(waitingForAcceptFrom.find(saToAccept));
                acceptPeer(saToAccept,machine);
            }
        }
        break;

        case ID_REJECT_NEW_HOST:
        {
            RakNet::SystemAddress saToReject;
            saToReject.SetBinaryAddress(((char*)p->data)+1);
            printf("Rejecting new client\n");
            cout << p->guid.ToString() << " REJECTS " << saToReject.ToString() << endl;
            if(waitingForAcceptFrom.find(saToReject)==waitingForAcceptFrom.end())
                printf("Could not find waitingForAcceptFrom for this GUID, weird");
            else
                waitingForAcceptFrom.erase(waitingForAcceptFrom.find(saToReject));
            rakInterface->CloseConnection(saToReject,true);
        }
        break;

        case ID_CLIENT_INPUTS:
            if(peerIDs.find(p->guid)==peerIDs.end() || peerInputs.find(peerIDs[p->guid])==peerInputs.end())
            {
                cout << __FILE__ << ":" << __LINE__ << " OOPS!!!!\n";
            }
            //cout << "GOT CLIENT INPUTS\n";
            peerInputs[peerIDs[p->guid]].push_back(string((char*)GetPacketData(p),(int)GetPacketSize(p)));
            break;
        default:
            printf("UNEXPECTED PACKET ID: %d\n",int(packetIdentifier));
            break;
        }

    }
}

void Server::sync()
{
    cout << "SERVER SYNCING\n";
    while(memoryBlocksLocked)
    {
        ;
    }
    memoryBlocksLocked=true;

    if(!firstSync)
    {
        syncHappend = true;
    }
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

        if(block.size != staleBlock.size)
        {
            cout << "BLOCK SIZE MISMATCH\n";
        }

        bool dirty=false;
        for(int a=0; a<block.size; a++)
        {
            if(block.data[a] ^ staleBlock.data[a]) dirty=true;
        }
        if(dirty)
        {
            for(int a=0; a<block.size; a++)
            {
                blockChecksum = blockChecksum ^ block.data[a];
                staleChecksum = staleChecksum ^ staleBlock.data[a];
            }
        }
        //dirty=true;
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
            bytesSynched += block.size;
            int bytesUsed = uncompressedPtr-uncompressedBuffer;
            while(bytesUsed+sizeof(int)+block.size >= uncompressedBufferSize)
            {
                uncompressedBufferSize *= 1.5;
		free(uncompressedBuffer);
                uncompressedBuffer = (unsigned char*)malloc(uncompressedBufferSize);
                uncompressedPtr = uncompressedBuffer + bytesUsed;
                if(!uncompressedBuffer)
                {
                    cout << __FILE__ << ":" << __LINE__ << " OUT OF MEMORY\n";
                    exit(1);
                }
            }
            memcpy(
                uncompressedPtr,
                &blockIndex,
                sizeof(int)
            );
            uncompressedPtr += sizeof(int);
            for(int a=0;a<block.size;a++)
            {
                *uncompressedPtr = block.data[a] ^ staleBlock.data[a];
                uncompressedPtr++;
            }
        }
        memcpy(staleBlock.data,block.data,block.size);
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
        if(compressedBufferSize <= uncompressedSize*1.01 + 128)
        {
            compressedBufferSize = zlibGetMaxCompressedSize(uncompressedSize);
	    free(compressedBuffer);
            compressedBuffer = (unsigned char*)malloc(compressedBufferSize);
            if(!compressedBuffer)
            {
                cout << __FILE__ << ":" << __LINE__ << " OUT OF MEMORY\n";
                exit(1);
            }
        }
        uLongf compressedSizeLong = compressedBufferSize;

        if(compress2(
            compressedBuffer,
            &compressedSizeLong,
            uncompressedBuffer,
            uncompressedSize,
            9
        )!=Z_OK)
        {
            cout << "ERROR COMPRESSING ZLIB STREAM\n";
            exit(1);
        }
        int compressedSize = (int)compressedSizeLong;
        printf("SYNC SIZE: %d\n",compressedSize);

        int SYNC_PACKET_SIZE=1024*1024*64;
        if(syncTransferSeconds)
        {
            int actualSyncTransferSeconds=max(1,syncTransferSeconds);
            while(true)
            {
                SYNC_PACKET_SIZE = compressedSize/60/actualSyncTransferSeconds;

		//This sends the data at 20 KB/sec minimum
                if(SYNC_PACKET_SIZE>=350 || actualSyncTransferSeconds==1) break;

                actualSyncTransferSeconds--;
            }
        }

        int sendMessageSize = 1+sizeof(int)+sizeof(int)+min(SYNC_PACKET_SIZE,compressedSize);
        int totalSendSizeEstimate = sendMessageSize*(compressedSize/SYNC_PACKET_SIZE + 2);
        if(syncBufferSize <= totalSendSizeEstimate)
        {
            syncBufferSize = totalSendSizeEstimate*1.5;
	    free(syncBuffer);
            syncBuffer = (unsigned char*)malloc(totalSendSizeEstimate);
            if(!syncBuffer)
            {
                cout << __FILE__ << ":" << __LINE__ << " OUT OF MEMORY\n";
                exit(1);
            }
        }
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

        if(int(sendMessage-syncBuffer) >= totalSendSizeEstimate)
        {
            cout << "INVALID SEND SIZE ESTIMATE!\n";
            exit(1);
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
		    IMMEDIATE_PRIORITY,
		    RELIABLE_ORDERED,
		    ORDERING_CHANNEL_CLIENT_INPUTS,
		    RakNet::UNASSIGNED_SYSTEM_ADDRESS,
		    true
		   );
    free(dataToSend);
}


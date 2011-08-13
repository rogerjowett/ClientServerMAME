#define DISABLE_EMUALLOC

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

#include "osdcore.h"
#include "emu.h"
#include "emuopts.h"

#include "unicode.h"
#include "ui.h"
#include "osdcore.h"

Client *netClient=NULL;
Client client;

Client *createGlobalClient(string _username)
{
    client = Client(_username);
    netClient = &client;
    return netClient;
}

void deleteGlobalClient()
{
    if(netClient) 
    {
	    netClient->shutdown();
    }
    netClient = NULL;
}

extern unsigned char *compressedBuffer;
extern int compressedBufferSize;
extern unsigned char *uncompressedBuffer;
extern int uncompressedBufferSize;

bool hasCompleteResync = false;


Client::Client(string _username)
    :
    Common(_username)
{
    initialSyncBuffer.reserve(1024*1024);

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

void Client::shutdown()
{
    // Be nice and let the server know we quit.
    rakInterface->Shutdown(300);

    // We're done with the network
    RakNet::RakPeerInterface::DestroyInstance(rakInterface);
}

MemoryBlock Client::createMemoryBlock(int size)
{
    if(initComplete)
    {
        cout << "ERROR: CREATED MEMORY BLOCK TOO LATE\n";
        exit(1);
    }
    blocks.push_back(MemoryBlock(size));
    staleBlocks.push_back(MemoryBlock(size));
    syncCheckBlocks.push_back(MemoryBlock(size));
    return blocks.back();
}

vector<MemoryBlock> Client::createMemoryBlock(unsigned char *ptr,int size)
{
    if(initComplete)
    {
        cout << "ERROR: CREATED MEMORY BLOCK TOO LATE\n";
        exit(1);
    }
    vector<MemoryBlock> retval;
    const int BYTES_IN_MB=1024*1024;
    if(size>BYTES_IN_MB)
    {
        for(int a=0;; a+=BYTES_IN_MB)
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
    syncCheckBlocks.push_back(MemoryBlock(size));
    retval.push_back(blocks.back());
    return retval;
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

int initialSyncPercentComplete=0;
extern bool waitingForClientCatchup;

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

    peerIDs[rakInterface->GetGuidFromSystemAddress(sa)] = 1;

    while(initComplete==false)
    {
        RakNet::Packet *p = rakInterface->Receive();
        if(!p)
        {
            //printf("WAITING FOR SERVER TO SEND GAME WORLD...\n");
            machine->video().frame_update();
            RakSleep(10);
            continue; //We need the first few packets, so stall until we get them
        }
        unsigned char packetID = GetPacketIdentifier(p);

        //printf("GOT PACKET: %d\n",int(packetID));

        switch(packetID)
        {
        case ID_DISCONNECTION_NOTIFICATION:
        {
            // Connection lost normally
            printf("ID_DISCONNECTION_NOTIFICATION\n");
            if(selfPeerID==0)
            {
                printf("Disconnected because you tried to connect too late, there were no slots available, or a connection could not be made between you and another peer.\n");
            }
            std::map<RakNet::RakNetGUID,int>::iterator it = peerIDs.find(p->guid);
            if(it != peerIDs.end())
            {
                std::map<int,string>::iterator it2 = peerNames.find(it->second);
                peerNames.erase(it2);
                peerIDs.erase(it);
            }
            return false;
        }
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
            RakNet::RakNetGUID guid;
            memcpy(&(guid.g),p->data+1+sizeof(int),sizeof(uint64_t));

            char buf[4096];
            strcpy(buf,(char*)(p->data+1+sizeof(int)+sizeof(uint64_t)));
            cout << "HOSTNAME " << sa.ToString() << " ACCEPTED (MY HOSTNAME IS " << rakInterface->GetExternalID(p->systemAddress).ToString() << ")\n";
            if(rakInterface->GetMyGUID()==guid)
            {
                //This is me, set my own ID and name
                selfPeerID = peerID;
                peerIDs[guid] = peerID;
                peerNames[peerID] = buf;
            }
            else
            {
                //This is someone else, set their ID and name
                peerIDs[guid] = peerID;
                peerNames[peerID] = buf;
                waitingForClientCatchup=true;
                machine->osd().pauseAudio(true);
            }
            if(peerInputs.find(peerID)==peerInputs.end())
                peerInputs[peerID] = vector<string>();
        }
        break;

        case ID_SEND_PEER_ID:
        {
            int peerID;
            memcpy(&peerID,p->data+1,sizeof(int));
            cout << "Matching: " << p->systemAddress.ToString() << " To " << peerID << endl;
            peerIDs[p->guid] = peerID;
            char buf[4096];
            strcpy(buf,(char*)(p->data+1+sizeof(int)));
            cout << "Matching: " << p->systemAddress.ToString() << " To " << buf << endl;
            peerNames[peerID] = buf;
        }
        break;


        case ID_INITIAL_SYNC_PARTIAL:
        {
            //printf("GOT PARTIAL SYNC FROM SERVER\n");
            int curPos = (int)initialSyncBuffer.size();
            initialSyncBuffer.resize(initialSyncBuffer.size()+GetPacketSize(p));
            memcpy(&initialSyncBuffer[curPos],GetPacketData(p),GetPacketSize(p));

            int totalSize;
            memcpy(&totalSize,(&initialSyncBuffer[sizeof(int)]),sizeof(int));
            initialSyncPercentComplete = initialSyncBuffer.size()*1000/totalSize;
        }
        break;
        case ID_INITIAL_SYNC_COMPLETE:
        {
            printf("GOT INITIAL SYNC FROM SERVER!\n");
            int curPos = (int)initialSyncBuffer.size();
            initialSyncBuffer.resize(initialSyncBuffer.size()+GetPacketSize(p));
            memcpy(&initialSyncBuffer[curPos],GetPacketData(p),GetPacketSize(p));
            loadInitialData(&initialSyncBuffer[0],(int)initialSyncBuffer.size(),machine);
            initComplete=true;
        }
        break;
        case ID_CLIENT_INPUTS:
            if(peerIDs.find(p->guid)==peerIDs.end())
            {
                cout << "GOT INPUTS FROM UNKNOWN USER: " << p->systemAddress.ToString() << endl;
            }
            else
            {
                if(peerInputs.find(peerIDs[p->guid])==peerInputs.end())
                    peerInputs[peerIDs[p->guid]] = vector<string>();
                peerInputs[peerIDs[p->guid]].push_back(string((char*)GetPacketData(p),(int)GetPacketSize(p)));
            }
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

void Client::loadInitialData(unsigned char *data,int size,running_machine *machine)
{
    unsigned char checksum = 0;

    cout << "INITIAL DATA SIZE: " << size << endl;

    int uncompressedSize;
    memcpy(&uncompressedSize,data,sizeof(int));
    data += sizeof(int);

    int compressedSize;
    memcpy(&compressedSize,data,sizeof(int));
    data += sizeof(int);

    vector<unsigned char> uncompressedSyncBuffer(uncompressedSize + 128);

    cout << "INITIAL DATA COMPRESSED SIZE: " << compressedSize << " UNCOMPRESSED SIZE: " << uncompressedSize << endl;

    int destLen = uncompressedSize+128;
    lzmaUncompress(&uncompressedSyncBuffer[0],destLen,data,compressedSize);

    RakNet::BitStream initialSyncStream(&uncompressedSyncBuffer[0],uncompressedSize + 128,false);

    printf("COMPRESSED SIZE: %d\n",compressedSize);

    initialSyncStream.Read(startupTime);

    initialSyncStream.Read(zeroInputMinTime);
    zeroInputMinTime.seconds += 2;
    waitingForClientCatchup=true;

    if(getSecondsBetweenSync())
    {
        cout << "READING NUM BLOCKS\n";
        int numBlocks;
        initialSyncStream.Read(numBlocks);

        cout << "LOADING " << numBlocks << " BLOCKS\n";
        {
            //Server and client must match on blocks
            if(blocks.size() != numBlocks)
            {
                cout << "ERROR: CLIENT AND SERVER BLOCK COUNTS DO NOT MATCH!\n";
            }

            for(int blockIndex=0; blockIndex<numBlocks; blockIndex++)
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

                //initialSyncStream.ReadBits(staleBlocks[blockIndex].data,blockSize*8);

                cout << "BLOCK " << blockIndex << ": ";
                for(int a=0; a<blockSize; a++)
                {
                    unsigned char xorValue;
                    initialSyncStream.ReadBits(&xorValue,8);
                    //cout << int(blocks[blockIndex].data[a] ^ xorValue) << ' ';
                    staleBlocks[blockIndex].data[a] = blocks[blockIndex].data[a] ^ xorValue;
                    checksum = checksum ^ staleBlocks[blockIndex].data[a];
                }
                cout << int(checksum) << endl;
            }
        }
    }

    while(true)
    {
        int peerID;
        initialSyncStream.Read(peerID);
        cout << "Read peer id: " << peerID << endl;
        if(peerID==-1)
        {
            break;
        }

        int numStrings;
        initialSyncStream.Read(numStrings);
        cout << "# strings: " << numStrings << endl;
        vector<string> inputsToPush = peerInputs[peerID];
        peerInputs[peerID].clear();
        for(int a=0; a<numStrings; a++)
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

            for(int b=0; b<strlen; b++)
            {
                checksum = checksum ^ buf[b];
            }
        }
        for(int c=0; c<inputsToPush.size(); c++)
        {
            peerInputs[peerID].push_back(inputsToPush[c]);
        }
    }
    unsigned char serverChecksum;
    initialSyncStream.Read(serverChecksum);
    if(checksum != serverChecksum)
    {
        cout << "CHECKSUM ERROR!!!\n";
        exit(1);
    }

    int syncBytes = initialSyncStream.GetReadOffset()/8;
    cout << "LOOKING FOR NVRAM AT LOCATION: " << syncBytes << endl;

    int nvramSize;
    memcpy(&nvramSize,&uncompressedSyncBuffer[0]+syncBytes,sizeof(int));

    cout << "GOT NVRAM OF SIZE: " << nvramSize << endl;

    if(nvramSize)
    {
        //save nvram to file
	    emu_file file(machine->options().nvram_directory(), OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS);
    	if (file.open(machine->basename(), ".nv") == FILERR_NONE) 
    	{
    	    file.write(&uncompressedSyncBuffer[0]+syncBytes+sizeof(int),nvramSize);
    	    file.close();
    	}
    }

    cout << "CHECKSUM: " << int(checksum) << endl;

    cout << "CLIENT INITIALIZED!\n";
}

void Client::updateSyncCheck()
{
    printf("UPDATING SYNC CHECK\n");
    for(int blockIndex=0; blockIndex<int(blocks.size()); blockIndex++)
    {
        memcpy(
            syncCheckBlocks[blockIndex].data,
            blocks[blockIndex].data,
            blocks[blockIndex].size
        );
    }
}


bool printWhenCheck=false;

bool Client::sync(running_machine *machine)
{
    if(!hasCompleteResync) return false;
    hasCompleteResync=false;

    bool hadToResync = resync(compressedBuffer,int(syncPtr-compressedBuffer),machine);

    //We have to return here because processing two syncs without a frame
    //in between can cause crashes
    syncPtr = compressedBuffer;
    if(firstResync || hadToResync)
    {
        printf("BEGINNING VIDEO SKIP\n");
        firstResync=false;
        return true;
    }
    else
    {
        return false;
    }
}

bool Client::update(running_machine *machine)
{
    for(
        map<RakNet::RakNetGUID,vector< string > >::iterator it = unknownPeerInputs.begin();
        it != unknownPeerInputs.end();
    )
    {
        if(peerIDs.find(it->first)!=peerIDs.end())
        {
            int ID = peerIDs[it->first];
            for(int a=0; a<it->second.size(); a++)
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

    RakSleep(0);
    if(printWhenCheck)
    {
        printWhenCheck=false;
        //printf("Checking for packets\n");
    }

    for(int packetCount=0;; packetCount++)
    {
        RakNet::Packet *p = rakInterface->Receive();
        if(!p)
        {
            break;
        }
        unsigned char packetID = GetPacketIdentifier(p);

        switch(packetID)
        {
        case ID_CONNECTION_LOST:
            // Couldn't deliver a reliable packet - i.e. the other system was abnormally
            // terminated
            printf("ID_CONNECTION_LOST\n");
        case ID_DISCONNECTION_NOTIFICATION:
            // Connection lost normally
            printf("ID_DISCONNECTION_NOTIFICATION\n");
            if(peerIDs.find(p->guid)!=peerIDs.end())
            {
                if(peerIDs[p->guid]==1)
                {
                    //Server quit, we are done.
                    return false;
                }
                else
                {
                    peerIDs.erase(p->guid);
                }
            }
            break;
        case ID_ALREADY_CONNECTED:
            printf("ID_ALREADY_CONNECTED\n");
            return false;
            break;
        case ID_INCOMPATIBLE_PROTOCOL_VERSION:
            printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
            return false;
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
            return false;
            break;
        case ID_CONNECTION_ATTEMPT_FAILED:
            printf("Connection attempt failed\n");
            return false;
            break;
        case ID_NO_FREE_INCOMING_CONNECTIONS:
            // Sorry, the server is full.  I don't do anything here but
            // A real app should tell the user
            printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
            return false;
            break;

        case ID_INVALID_PASSWORD:
            printf("ID_INVALID_PASSWORD\n");
            return false;
            break;

        case ID_CONNECTION_REQUEST_ACCEPTED:
            // This tells the client they have connected
            printf("ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s\n", p->systemAddress.ToString(true), p->guid.ToString());
            printf("My external address is %s\n", rakInterface->GetExternalID(p->systemAddress).ToString(true));
            break;

        case ID_ADVERTISE_SYSTEM:
        {
            RakNet::SystemAddress sa;
            sa.SetBinaryAddress(((char*)p->data)+1);
            RakNet::SystemAddress sa2 = ConnectBlocking(sa.ToString(false),sa.port);
            if(sa2 != RakNet::UNASSIGNED_SYSTEM_ADDRESS)
            {
                cout << "Sending ID\n";
                //Tell the new guy your ID
                char buf[4096];
                buf[0] = ID_SEND_PEER_ID;
                memcpy(buf+1,&selfPeerID,sizeof(int));
                strcpy(buf+1+sizeof(int),username.c_str());
                rakInterface->Send(buf,1+sizeof(int)+username.length()+1,HIGH_PRIORITY,RELIABLE_ORDERED,0,sa2,false);
            }
            cout << sa.binaryAddress << ':' << sa.port << endl;
            cout << sa2.binaryAddress << ':' << sa2.port << endl;
            if(sa2==RakNet::UNASSIGNED_SYSTEM_ADDRESS)
            {
                //Tell the boss that you can't accept this guy
                char buf[4096];
                buf[0] = ID_REJECT_NEW_HOST;
                strcpy(buf+1,((char*)p->data)+1);
                rakInterface->Send(buf,1+strlen(((char*)p->data)+1)+1,HIGH_PRIORITY,RELIABLE_ORDERED,0,rakInterface->GetSystemAddressFromIndex(0),false);
            }
            else
            {
                //Tell the boss that you can't accept this guy
                char buf[4096];
                buf[0] = ID_ACCEPT_NEW_HOST;
                strcpy(buf+1,((char*)p->data)+1);
                rakInterface->Send(buf,1+strlen(((char*)p->data)+1)+1,HIGH_PRIORITY,RELIABLE_ORDERED,0,rakInterface->GetSystemAddressFromIndex(0),false);
            }

        }
        break;

        case ID_HOST_ACCEPTED:
        {
            int peerID;
            memcpy(&peerID,p->data+1,sizeof(int));
            RakNet::RakNetGUID guid;
            memcpy(&(guid.g),p->data+1+sizeof(int),sizeof(uint64_t));

            char buf[4096];
            strcpy(buf,(char*)(p->data+1+sizeof(int)+sizeof(uint64_t)));
            cout << "HOST ACCEPTED\n";
            if(rakInterface->GetMyGUID()==guid)
            {
                //This is me, set my own ID and name
                selfPeerID = peerID;
                peerIDs[guid] = peerID;
                peerNames[peerID] = buf;
            }
            else
            {
                //This is someone else, set their ID and name
                peerIDs[guid] = peerID;
                peerNames[peerID] = buf;
                waitingForClientCatchup=true;
                machine->osd().pauseAudio(true);
            }
            if(peerInputs.find(peerID)==peerInputs.end())
                peerInputs[peerID] = vector<string>();
        }
        break;

        case ID_RESYNC_PARTIAL:
        {
            //printf("GOT PARTIAL RESYNC\n");
            if(hasCompleteResync)
            {
                //hasCompleteResync=false;
                //syncPtr = compressedBuffer;
                printf("ERROR: GOT NEW RESYNC WHILE ANOTHER RESYNC WAS ON THE QUEUE!\n");
                return false;
            }
            int bytesUsed = syncPtr-compressedBuffer;
            while(bytesUsed+GetPacketSize(p) >= compressedBufferSize)
            {
                compressedBufferSize *= 1.5;
		free(compressedBuffer);
                compressedBuffer = (unsigned char*)malloc(compressedBufferSize);
                if(!compressedBuffer)
                {
                    cout << __FILE__ << ":" << __LINE__ << " OUT OF MEMORY\n";
                    exit(1);
                }
                syncPtr = compressedBuffer+bytesUsed;
            }
            memcpy(syncPtr,GetPacketData(p),GetPacketSize(p));
            syncPtr += GetPacketSize(p);
            printWhenCheck=true;
            break;
        }
        case ID_RESYNC_COMPLETE:
        {
            printf("GOT COMPLETE RESYNC\n");
            int bytesUsed = syncPtr-compressedBuffer;
            while(bytesUsed+GetPacketSize(p) >= compressedBufferSize)
            {
                compressedBufferSize *= 1.5;
		free(compressedBuffer);
                compressedBuffer = (unsigned char*)malloc(compressedBufferSize);
                if(!compressedBuffer)
                {
                    cout << __FILE__ << ":" << __LINE__ << " OUT OF MEMORY\n";
                    exit(1);
                }
                syncPtr = compressedBuffer+bytesUsed;
            }
            memcpy(syncPtr,GetPacketData(p),GetPacketSize(p));
            syncPtr += GetPacketSize(p);
            hasCompleteResync=true;
            return true;
            break;
        }
        case ID_CLIENT_INPUTS:
            if(peerIDs.find(p->guid)==peerIDs.end() || peerInputs.find(peerIDs[p->guid])==peerInputs.end())
            {
                cout << __FILE__ << ":" << __LINE__ << " OOPS!!!!\n";
            }
            peerInputs[peerIDs[p->guid]].push_back(string((char*)GetPacketData(p),(int)GetPacketSize(p)));
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
            return false;
        }

        rakInterface->DeallocatePacket(p);
    }

    return true;
}

bool Client::resync(unsigned char *data,int size,running_machine *machine)
{
    int uncompressedSize,compressedSize;
    memcpy(&uncompressedSize,data,sizeof(int));
    data += sizeof(int);
    memcpy(&compressedSize,data,sizeof(int));
    data += sizeof(int);

    if(uncompressedSize+128 >= uncompressedBufferSize)
    {
        uncompressedBufferSize = uncompressedSize+256;
	free(uncompressedBuffer);
        uncompressedBuffer = (unsigned char*)malloc(uncompressedBufferSize);
        if(!uncompressedBuffer)
        {
            cout << __FILE__ << ":" << __LINE__ << " OUT OF MEMORY\n";
            exit(1);
        }
    }
    uLongf destLen = uncompressedBufferSize;
    if(uncompress(uncompressedBuffer,&destLen,data,compressedSize)!=Z_OK)
    {
        cout << "ERROR: ZLIB UNCOMPRESS FAILED\n";
        exit(1);
    }
    unsigned char *uncompressedPtr = uncompressedBuffer;


    //Check to see if the client is clean
    bool clientIsClean=true;

    int badByteCount=0;
    int totalByteCount=0;

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

        //cout << "CLIENT: GOT MESSAGE FOR INDEX: " << blockIndex << endl;

        //if(clientSizeOfNextMessage!=block.size)
        //{
        //cout << "ERROR!: SIZE MISMATCH " << clientSizeOfNextMessage
        //<< " != " << block.size << endl;
        //}

        //cout << "BYTES READ: " << (xorBlock.size-strm.avail_out) << endl;
        for(int a=0; a<block.size; a++)
        {
            totalByteCount++;
            if(syncCheckBlock.data[a] != (uncompressedPtr[a] ^ staleBlock.data[a]))
            {
                badByteCount++;
                if(badByteCount<50)
                {
                    printf("BLOCK %d BYTE %d IS BAD\n",blockIndex,a);
                }
                if(badByteCount>64)
                    clientIsClean=false;
                //break;
            }
        }
        uncompressedPtr += block.size;
    }

    if(badByteCount*100/totalByteCount>=1) clientIsClean=false; //1% or more of the bytes are bad

    if(clientIsClean)
    {
        printf("CLIENT IS CLEAN\n");
        for(int a=0; a<int(blocks.size()); a++)
        {
            memcpy(staleBlocks[a].data,syncCheckBlocks[a].data,syncCheckBlocks[a].size);
        }
        return false;
    }

    if (machine->scheduler().can_save()==false)
    {
        printf("CLIENT IS DIRTY BUT HAD ANONYMOUS TIMER SO CAN'T FIX!\n");
        uncompressedPtr = uncompressedBuffer;
        //unsigned char blockChecksum=0;
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

            //cout << "CLIENT: GOT MESSAGE FOR INDEX: " << blockIndex << endl;

            //if(clientSizeOfNextMessage!=block.size)
            //{
            //cout << "ERROR!: SIZE MISMATCH " << clientSizeOfNextMessage
            //<< " != " << block.size << endl;
            //}

            //cout << "BYTES READ: " << (xorBlock.size-strm.avail_out) << endl;
            for(int a=0; a<block.size; a++)
            {
                staleBlock.data[a] = uncompressedPtr[a] ^ staleBlock.data[a];
                xorChecksum = xorChecksum ^ uncompressedPtr[a];
                staleChecksum = staleChecksum ^ staleBlock.data[a];
            }
            uncompressedPtr += block.size;

            //cout << "CLIENT: FINISHED GETTING MESSAGE\n";
        }
        printf("XOR CHECKSUM: %d\n",int(xorChecksum));
        printf("STALE CHECKSUM: %d\n",int(staleChecksum));
        return false;
    }

    printf("CLIENT IS DIRTY (%d bad blocks, %f%% of total)\n",badByteCount,float(badByteCount)*100.0f/totalByteCount);
    ui_popup_time(3, "You are out of sync with the server, resyncing...");


    //The client is not clean and needs to be resynced

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

        //cout << "CLIENT: GOT MESSAGE FOR INDEX: " << blockIndex << endl;

        //if(clientSizeOfNextMessage!=block.size)
        //{
        //cout << "ERROR!: SIZE MISMATCH " << clientSizeOfNextMessage
        //<< " != " << block.size << endl;
        //}

        //cout << "BYTES READ: " << (xorBlock.size-strm.avail_out) << endl;
        for(int a=0; a<block.size; a++)
        {
            staleBlock.data[a] = uncompressedPtr[a] ^ staleBlock.data[a];
            blockChecksum = blockChecksum ^ staleBlock.data[a];
            xorChecksum = xorChecksum ^ uncompressedPtr[a];
            staleChecksum = staleChecksum ^ staleBlock.data[a];
        }
        uncompressedPtr += block.size;

        //cout << "CLIENT: FINISHED GETTING MESSAGE\n";
    }
    printf("BLOCK CHECKSUM: %d\n",int(blockChecksum));
    printf("XOR CHECKSUM: %d\n",int(xorChecksum));
    printf("STALE CHECKSUM: %d\n",int(staleChecksum));
#endif

    revert(machine);

    return true;
}

void Client::revert(running_machine *machine)
{
    machine->save().doPreSave();

    //If the client has predicted anything, erase the prediction
    for(int a=0;a<blocks.size();a++)
    {
        memcpy(blocks[a].data,staleBlocks[a].data,blocks[a].size);
    }

    machine->save().doPostLoad();
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

    for(int a=0; a<int(blocks.size()); a++)
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
        IMMEDIATE_PRIORITY,
        RELIABLE_ORDERED,
        ORDERING_CHANNEL_CLIENT_INPUTS,
        RakNet::UNASSIGNED_SYSTEM_ADDRESS,
        true
    );
    free(dataToSend);
}


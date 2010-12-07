#include "RakPeerInterface.h"
#include "RakNetStatistics.h"
#include "RakNetTypes.h"
#include "BitStream.h"
#include "PacketLogger.h"
#include "RakNetTypes.h"

#include "NSM_Common.h"

#include "osdcore.h"

extern volatile bool memoryBlocksLocked;

// Copied from Multiplayer.cpp
// If the first byte is ID_TIMESTAMP, then we want the 5th byte
// Otherwise we want the 1st byte
extern unsigned char GetPacketIdentifier(RakNet::Packet *p);
extern unsigned char *GetPacketData(RakNet::Packet *p);
extern int GetPacketSize(RakNet::Packet *p);

extern unsigned char compressedBuffer[MAX_ZLIB_BUF_SIZE];
extern unsigned char syncBuffer[MAX_ZLIB_BUF_SIZE];
extern unsigned char uncompressedBuffer[MAX_ZLIB_BUF_SIZE];

Common::Common(string _username)
:
selfPeerID(0),
username(_username),
startupTime(RakNet::GetTimeUS())
{
    if(username.length()>16)
    {
        username = username.substr(0,16);
    }
}

RakNet::SystemAddress Common::ConnectBlocking(const char *defaultAddress, unsigned short defaultPort)
{
    char ipAddr[64];
    {
        {
            strcpy(ipAddr, defaultAddress);
        }
    }
    if (rakInterface->Connect(ipAddr, defaultPort, "MAME", (int)strlen("MAME"))!=RakNet::CONNECTION_ATTEMPT_STARTED)
    {
        printf("Failed connect call for %s : %d.\n", defaultAddress, int(defaultPort) );
        return RakNet::UNASSIGNED_SYSTEM_ADDRESS;
    }
    printf("Connecting to %s:%d...",ipAddr,defaultPort);
    RakNet::Packet *packet;
    while (1)
    {
        for (packet=rakInterface->Receive(); packet; rakInterface->DeallocatePacket(packet), packet=rakInterface->Receive())
        {
            if (packet->data[0]==ID_CONNECTION_REQUEST_ACCEPTED)
            {
                printf("Connected!\n");
                return packet->systemAddress;
            }
            else if(packet->data[0]==ID_SERVER_INPUTS)
            {
                peerInputs[1].push_back(string(((char*)GetPacketData(packet)),(int)GetPacketSize(packet)));
            }
            else if(packet->data[0]==ID_CLIENT_INPUTS)
            {
                if(peerIDs.find(packet->systemAddress)==peerIDs.end() || peerInputs.find(peerIDs[packet->systemAddress])==peerInputs.end())
                {
                    if(unknownPeerInputs.find(packet->systemAddress)==unknownPeerInputs.end())
                    {
                        unknownPeerInputs[packet->systemAddress] = vector<string>();
                    }
                    unknownPeerInputs[packet->systemAddress].push_back(string((char*)GetPacketData(packet),(int)GetPacketSize(packet)));
                }
                else
                {
                    peerInputs[peerIDs[packet->systemAddress]].push_back(string((char*)GetPacketData(packet),(int)GetPacketSize(packet)));
                }
            }
            else
            {
                printf("Failed: %d\n",int(packet->data[0]));
                return RakNet::UNASSIGNED_SYSTEM_ADDRESS;
            }
            //JJG: Need to sleep for 1/10th a sec here osd_sleep(100);
        }
    }
}

string Common::getLatencyString(int peerID)
{
    for(
        std::map<RakNet::SystemAddress,int>::iterator it = peerIDs.begin();
        it != peerIDs.end();
        it++
        )
    {
        if(it->second==peerID)
        {
            char buf[4096];
            sprintf(buf,"Client %d Ping: %d ms", peerID, rakInterface->GetAveragePing(it->first));
            return string(buf);
        }
    }
    printf("ERROR GETTING LATENCY STRING\n");
    return "";
}

string Common::getStatisticsString()
{
    RakNet::RakNetStatistics *rss;
    string retval;
    for(int a=0; a<rakInterface->NumberOfConnections(); a++)
    {
        char message[4096];
        rss=rakInterface->GetStatistics(rakInterface->GetSystemAddressFromIndex(0));
        StatisticsToString(rss, message, 0);
        retval += string(message) + string("\n");
    }
    return retval;
}

void Common::addConstBlock(unsigned char *tmpdata,int size)
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
        //constBlocks.erase(constBlocks.begin());
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
			RELIABLE_ORDERED,
			ORDERING_CHANNEL_CONST_DATA,
			RakNet::UNASSIGNED_SYSTEM_ADDRESS,
			true
		       );
	memoryBlocksLocked=false;
    //cout << "done\n";
    //cout << "EXITING CRITICAL SECTION\n";
}

int Common::getOtherPeerID(int index)
{
    int count=0;
    for(int a=0;a<rakInterface->NumberOfConnections();a++)
    {
        if(peerIDs.find(rakInterface->GetSystemAddressFromIndex(a))!=peerIDs.end())
        {
            if(count==index)
                return peerIDs[rakInterface->GetSystemAddressFromIndex(a)];
            else
                count++;
        }
    }
    printf("ERROR GETTING PEER ID\n");
    return 0;
}

string Common::popInputBuffer(int clientIndex)
{
    int peerID = getOtherPeerID(clientIndex);
    if(peerInputs[peerID].empty())
        return string("");

    string retval = peerInputs[peerID].back();
    oldPeerInputs[peerID].push_back(retval);
    peerInputs[peerID].pop_back();
    return retval;
}

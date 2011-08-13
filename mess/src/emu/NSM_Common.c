#define DISABLE_EMUALLOC

#include "RakPeerInterface.h"
#include "RakNetStatistics.h"
#include "RakNetTypes.h"
#include "BitStream.h"
#include "PacketLogger.h"
#include "RakNetTypes.h"

#include "NSM_Common.h"

#include "osdcore.h"

#include "LzmaEnc.h"
#include "LzmaDec.h"

SRes OnProgress(void *p, UInt64 inSize, UInt64 outSize)
{
  // Update progress bar.
  return SZ_OK;
}
ICompressProgress g_ProgressCallback = { &OnProgress };

void * AllocForLzma(void *p, size_t size)
{
    void *ptr = malloc(size);
    if(!ptr)
    {
        cout << "FAILED TO ALLOCATE BLOCK OF SIZE " << size/1024.0/1024.0 << " MB " << endl;
    }
    return ptr;
}
void FreeForLzma(void *p, void *address)
{
    free(address);
}
ISzAlloc SzAllocForLzma = { &AllocForLzma, &FreeForLzma };

int zlibGetMaxCompressedSize(int origSize)
{
    return origSize*1.01 + 256;
}
int lzmaGetMaxCompressedSize(int origSize)
{
    return origSize + origSize/3 + 256 + LZMA_PROPS_SIZE;
}

void lzmaCompress(
    unsigned char* destBuf,
    int &destSize,
    unsigned char *srcBuf,
    int srcSize,
    int compressionLevel
)
{
    SizeT propsSize = LZMA_PROPS_SIZE;

    SizeT lzmaDestSize = (SizeT)destSize;

    CLzmaEncProps props;
    LzmaEncProps_Init(&props);
    props.level = compressionLevel; //compression level
    //props.dictSize = 1 << 16;
    //props.dictSize = 1 << 24;
    props.writeEndMark = 1; // 0 or 1

    int res = LzmaEncode(
                  destBuf+LZMA_PROPS_SIZE, &lzmaDestSize,
                  srcBuf, srcSize,
                  &props, destBuf, &propsSize, props.writeEndMark,
                  &g_ProgressCallback, &SzAllocForLzma, &SzAllocForLzma);

    destSize = (int)lzmaDestSize + LZMA_PROPS_SIZE;

    cout << "COMPRESSED " << srcSize << " BYTES DOWN TO " << destSize << endl;

    if(res != SZ_OK || propsSize != LZMA_PROPS_SIZE)
    {
        cout << "ERROR COMPRESSING DATA\n";
        cout << res << ',' << propsSize << endl;
        exit(1);
    }
}

void lzmaUncompress(
    unsigned char* destBuf,
    int destSize,
    unsigned char *srcBuf,
    int srcSize
)
{
    SizeT lzmaDestSize = (SizeT)destSize;
    SizeT lzmaSrcSize = (SizeT)srcSize - LZMA_PROPS_SIZE;

    cout << "DECOMPRESSING " << srcSize << endl;

    ELzmaStatus finishStatus;
    int res = LzmaDecode(
                  destBuf, &lzmaDestSize,
                  srcBuf+LZMA_PROPS_SIZE, &lzmaSrcSize,
                  srcBuf, LZMA_PROPS_SIZE, LZMA_FINISH_END,
                  &finishStatus, &SzAllocForLzma);

    cout << "DECOMPRESSED " << srcSize << " BYTES DOWN TO " << lzmaDestSize << endl;

    if(res != SZ_OK || finishStatus != LZMA_STATUS_FINISHED_WITH_MARK)
    {
        cout << "ERROR DECOMPRESSING DATA\n";
        cout << res << ',' << finishStatus << endl;
        exit(1);
    }
}

extern volatile bool memoryBlocksLocked;

// Copied from Multiplayer.cpp
// If the first byte is ID_TIMESTAMP, then we want the 5th byte
// Otherwise we want the 1st byte
extern unsigned char GetPacketIdentifier(RakNet::Packet *p);
extern unsigned char *GetPacketData(RakNet::Packet *p);
extern int GetPacketSize(RakNet::Packet *p);

Common::Common(string _username)
    :
    secondsBetweenSync(0),
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
                if(peerIDs.find(packet->guid)==peerIDs.end() || peerInputs.find(peerIDs[packet->guid])==peerInputs.end())
                {
                    if(unknownPeerInputs.find(packet->guid)==unknownPeerInputs.end())
                    {
                        unknownPeerInputs[packet->guid] = vector<string>();
                    }
                    unknownPeerInputs[packet->guid].push_back(string((char*)GetPacketData(packet),(int)GetPacketSize(packet)));
                }
                else
                {
                    peerInputs[peerIDs[packet->guid]].push_back(string((char*)GetPacketData(packet),(int)GetPacketSize(packet)));
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

void Common::setSecondsBetweenSync(int _secondsBetweenSync)
{
    secondsBetweenSync = _secondsBetweenSync;
}

int Common::getLargestPing()
{
    int largestPing=1;
    for(int a=0; a<rakInterface->NumberOfConnections(); a++)
    {
        largestPing = max(rakInterface->GetHighestPing(rakInterface->GetSystemAddressFromIndex(a)),largestPing);
    }
    return largestPing;
}

bool Common::hasPeerWithID(int peerID)
{
    if(selfPeerID==peerID) return true;
    for(
        std::map<RakNet::RakNetGUID,int>::iterator it = peerIDs.begin();
        it != peerIDs.end();
        it++
    )
    {
        if(it->second==peerID)
        {
            return true;
        }
    }
    return false;
}

string Common::getLatencyString(int peerID)
{
    for(
        std::map<RakNet::RakNetGUID,int>::iterator it = peerIDs.begin();
        it != peerIDs.end();
        it++
    )
    {
        if(it->second==peerID)
        {
            char buf[4096];
            sprintf(buf,"Peer %d: %d ms", peerID, rakInterface->GetHighestPing(it->first));
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
        rss=rakInterface->GetStatistics(rakInterface->GetSystemAddressFromIndex(a));
        sprintf(
            message,
            "Sent: %d\n"
            "Recv: %d\n"
            "Loss: %.0f%%\n",
            (int)rss->valueOverLastSecond[RakNet::ACTUAL_BYTES_SENT],
            (int)rss->valueOverLastSecond[RakNet::ACTUAL_BYTES_RECEIVED],
            rss->packetlossLastSecond
        );
        retval += string(message) + string("\n");
    }
    return retval;
}

vector<int> Common::getPeerIDs()
{
    vector<int> retval;
    for(
        std::map<RakNet::RakNetGUID,int>::iterator it = peerIDs.begin();
        it != peerIDs.end();
        it++
    )
    {
        retval.push_back(it->second);
    }
    return retval;
}

int Common::getNumOtherPeers()
{
    return int(peerIDs.size())-1;
}

int Common::getOtherPeerID(int index)
{
    int count=0;
    for(int a=0; a<rakInterface->NumberOfConnections(); a++)
    {
        if(peerIDs.find(rakInterface->GetGUIDFromIndex(a))!=peerIDs.end())
        {
            if(count==index)
                return peerIDs[rakInterface->GetGUIDFromIndex(a)];
            else
                count++;
        }
    }
    return 0;
}

string Common::popInputBuffer(int clientIndex)
{
    int peerID = getOtherPeerID(clientIndex);
    if(peerInputs[peerID].empty())
        return string("");

    string retval = peerInputs[peerID].front();
    oldPeerInputs[peerID].push_back(retval);
    peerInputs[peerID].erase(peerInputs[peerID].begin());
    return retval;
}

string Common::popSelfInputBuffer()
{
    int peerID = selfPeerID;
    if(peerInputs[peerID].empty())
        return string("");

    string retval = peerInputs[peerID].front();
    oldPeerInputs[peerID].push_back(retval);
    peerInputs[peerID].erase(peerInputs[peerID].begin());
    return retval;
}

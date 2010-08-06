#include "NSM_Client.h"

#include "NSM_Server.h"

Client *netClient=NULL;

Client *createGlobalClient()
{
    netClient = new Client();
    return netClient;
}

void deleteGlobalClient()
{
    if(netClient) delete netClient;
    netClient = NULL;
}

z_stream clientStrm;
unsigned char compressedInBuf[MAX_COMPRESSED_OUTBUF_SIZE];

Client::Client() 
{
    io_service = new boost::asio::io_service();
    resolver = new tcp::resolver(*io_service);

    /* allocate deflate state */
    clientStrm.zalloc = Z_NULL;
    clientStrm.zfree = Z_NULL;
    clientStrm.opaque = Z_NULL;

    clientSocket = NULL;
}

MemoryBlock Client::createMemoryBlock(int size)
{
    blocks.push_back(MemoryBlock(size));
    xorBlocks.push_back(MemoryBlock(size));
    staleBlocks.push_back(MemoryBlock(size));
    return blocks.back();
}

MemoryBlock Client::createMemoryBlock(unsigned char *ptr,int size)
{
    blocks.push_back(MemoryBlock(ptr,size));
    xorBlocks.push_back(MemoryBlock(size));
    staleBlocks.push_back(MemoryBlock(size));
    return blocks.back();
}

void Client::initializeConnection(const char *hostname,const char *port)
{
    boost::mutex::scoped_lock(memoryBlockMutex);
    tcp::resolver::query query(hostname, port);

    tcp::resolver::iterator iterator = resolver->resolve(query);

    clientSocket = new tcp::socket(*io_service);

    try
    {
        clientSocket->connect(*iterator);    
    }
    catch(std::exception ex)
    {
        cout << "CONNECTION FAILED\n";
        delete clientSocket;
        clientSocket = NULL;
        throw std::runtime_error("FAILED TO CONNECT TO HOST!");
    }

    clientSocket->set_option(tcp::no_delay(true));
    boost::asio::socket_base::non_blocking_io command(false);
    clientSocket->io_control(command); 

    boost::thread t(boost::bind(&boost::asio::io_service::run, io_service));

    cout << "READING NUM BLOCKS\n";
    int numBlocks;
    boost::asio::read(*clientSocket,boost::asio::buffer(&numBlocks, sizeof(int)));

    cout << "LOADING " << numBlocks << " BLOCKS\n";
    if(blocks.size()==0)
    {
        //Assume the memory is not set up on the client yet and the client will assign the memory to something later
        for(int blockIndex=0;blockIndex<numBlocks;blockIndex++)
        {
            int blockSize;
            boost::asio::read(*clientSocket,boost::asio::buffer(&blockSize, sizeof(int)));
            cout << "GOT INITIAL BLOCK OF SIZE: " << blockSize << endl;

            blocks.push_back(MemoryBlock(blockSize));
            xorBlocks.push_back(MemoryBlock(blockSize));
            staleBlocks.push_back(MemoryBlock(blockSize));

            int bytesRead = boost::asio::read(*clientSocket,boost::asio::buffer(blocks.back().data, blockSize));
            cout << bytesRead << " BYTES READ\n";

            cout << "INITIAL BLOCK COUNT: " << blocks.back().getBitCount() << endl;
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
            int headerBytesRead = boost::asio::read(*clientSocket,boost::asio::buffer(&blockSize, sizeof(int)));
            if(headerBytesRead != sizeof(int)) cout << "OH SHIT! HEADER IS PARTIAL READ\n";
            //cout << "GOT INITIAL BLOCK OF SIZE: " << blockSize << endl;

            if(blockSize != blocks[blockIndex].size)
            {
                cout << "ERROR: CLIENT AND SERVER BLOCK SIZES AT INDEX " << blockIndex << " DO NOT MATCH!\n";
                cout << blockSize << " != " << blocks[blockIndex].size << endl;
                exit(1);
            }

            int bytesRead = boost::asio::read(*clientSocket,boost::asio::buffer(blocks[blockIndex].data, blockSize));
            memcpy(staleBlocks[blockIndex].data,blocks[blockIndex].data,blocks[blockIndex].size);
            //cout << bytesRead << " BYTES READ\n";
        }
        cout << "CLIENT INITIALIZED!\n";
    }

    int numConstBlocks;
	boost::asio::read(*clientSocket,boost::asio::buffer(&numConstBlocks, sizeof(int)));

	for(int blockIndex=0;blockIndex<numConstBlocks;blockIndex++)
	{
		//cout << "BLOCK SIZE FOR INDEX " << blockIndex << ": " << constBlocks[blockIndex].size << endl;
        constBlocks.push_back(MemoryBlock(0));
		boost::asio::read(*clientSocket,boost::asio::buffer(&(constBlocks[blockIndex].size), sizeof(int)));
        constBlocks[blockIndex].data = (unsigned char*)malloc(constBlocks[blockIndex].size);
		int bytesWritten = boost::asio::read(*clientSocket,boost::asio::buffer(constBlocks[blockIndex].data, constBlocks[blockIndex].size));
		if(bytesWritten != constBlocks[blockIndex].size) cout << "OH SHIT! BAD NETWORK SEND\n";
	}
}

bool Client::syncAndUpdate() 
{
    if(clientSocket==NULL)
        return false;

    boost::mutex::scoped_lock(memoryBlockMutex);
    int clientSizeOfNextMessage;
    int blockIndex;
    bool gotResync = false;
    //Wait until we have at least 180 bytes in the queue, this is roughly the size of a 
    //input state from the server
    //cout << "CLIENT: BEGINNING SYNC\n";
    //for(int msgCount=0;msgCount<10;msgCount++)
    while(true)
    {
        boost::asio::socket_base::bytes_readable bytesReadableCommand(true);
        clientSocket->io_control(bytesReadableCommand);

        if(bytesReadableCommand.get()<600)
            break;
        //cout << msgCount << " " << bytesReadableCommand.get() << " BYTES READABLE\n";
        int header;
        boost::asio::read(*clientSocket,boost::asio::buffer(&header, sizeof(int)));
        if(header==1)
        {
            gotResync=true;
            //If the client has predicted anything, erase the prediction
            for(int a=0;a<int(blocks.size());a++)
            {
                memcpy(blocks[a].data,staleBlocks[a].data,blocks[a].size);
            }
            int bufLen;
            boost::asio::read(*clientSocket,boost::asio::buffer(&bufLen, sizeof(int)));
            //cout << "READING " << bufLen << " BYTES\n";
            boost::asio::read(*clientSocket,boost::asio::buffer(compressedInBuf, bufLen));

            char checksum = 0;
            for(int a=0;a<bufLen;a++)
            {
                checksum = checksum ^ compressedInBuf[a];
            }
            //cout << "CHECKSUM: " << int(checksum) << endl;

            clientStrm.next_in = compressedInBuf;
            clientStrm.avail_in = bufLen;
            int ret = inflateInit(&clientStrm);
            if (ret != Z_OK)
                cout << "CREATING ZLIB STREAM FAILED\n";

#if 0
            clientStrm.next_out = (Bytef*)uncompressedInBuf;
            clientStrm.avail_out = MAX_COMPRESSED_OUTBUF_SIZE;
            int retval = inflate(&clientStrm,Z_NO_FLUSH);
            cout << "INFLATE RETVAL: " << retval << endl;
            if(clientStrm.msg) cout << "ERROR MESSAGE: " << clientStrm.msg << endl;
            if(retval==Z_DATA_ERROR) exit(1);
#endif		

            while(true)
            {
                //boost::asio::read(*clientSocket,boost::asio::buffer(&blockIndex, sizeof(int)));
                clientStrm.next_out = (Bytef*)&blockIndex;
                clientStrm.avail_out = sizeof(int);
                int retval = inflate(&clientStrm,Z_SYNC_FLUSH);
                //cout << "BYTES READ: " << (sizeof(int)-clientStrm.avail_out) << endl;
                //cout << "INFLATE RETVAL: " << retval << endl;
                if(retval==Z_DATA_ERROR) exit(1);

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

                //boost::asio::read(*clientSocket,boost::asio::buffer(xorBlock.data, clientSizeOfNextMessage));
                clientStrm.next_out = xorBlock.data;
                clientStrm.avail_out = xorBlock.size;
                retval = inflate(&clientStrm,Z_SYNC_FLUSH);
                //cout << "BYTES READ: " << (xorBlock.size-clientStrm.avail_out) << endl;
                //cout << "INFLATE RETVAL: " << retval << endl;
                if(retval==Z_DATA_ERROR) exit(1);
                for(int a=0;a<block.size;a++)
                {
                    block.data[a] = xorBlock.data[a] ^ staleBlock.data[a];
                }
                memcpy(staleBlock.data,block.data,block.size);

                //cout << "CLIENT: FINISHED GETTING MESSAGE\n";
            }
        }
        else if(header==2)
        {
            cout << "READING MESSAGE LENGTH\n";

            int msgLength;
            boost::asio::read(*clientSocket,boost::asio::buffer(&msgLength, sizeof(int)));

            incomingMsg.resize(msgLength);
            boost::asio::read(*clientSocket,boost::asio::buffer(&incomingMsg[0], msgLength));

            inputBufferQueue.push_back(string((const char*)&incomingMsg[0],msgLength));
        }
        else if(header==3)
        {
            //cout << "READING CONST BLOCK\n";
            constBlocks.push_back(MemoryBlock(0));
		    boost::asio::read(*clientSocket,boost::asio::buffer(&(constBlocks.back().size), sizeof(int)));
            //cout << "GOT CONST BLOCK OF : " << constBlocks.back().size << " BYTES FROM SERVER\n";
            constBlocks.back().data = (unsigned char*)malloc(constBlocks.back().size);
		    int bytesWritten = boost::asio::read(*clientSocket,boost::asio::buffer(constBlocks.back().data, constBlocks.back().size));
		    if(bytesWritten != constBlocks.back().size) cout << "OH SHIT! BAD NETWORK SEND\n";
            //cout << "DONE\n";
        }
        else
        {
            throw std::runtime_error("OOPS");
        }
    }
    return gotResync;
}

void Client::checkMatch(Server *server)
{
    boost::mutex::scoped_lock(memoryBlockMutex);
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

void Client::sendString(const string &outputString)
{
    if(clientSocket)
    {
        try
        {
            int intSize = int(outputString.length());
            //cout << "SENDING MESSAGE WITH LENGTH: " << intSize << endl;
            boost::asio::write(*clientSocket,boost::asio::buffer(&intSize, sizeof(int)));
            boost::asio::write(*clientSocket,boost::asio::buffer(outputString.c_str(), intSize));
        }
        catch(std::exception ex)
        {
            throw std::runtime_error("CONNECTION TO SERVER WAS TERMINATED!");
        }
    }
}

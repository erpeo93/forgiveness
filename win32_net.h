#pragma once
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "net.h"

#define WriteBarrier _WriteBarrier(); _mm_sfence();
#define ArrayC( array ) ( sizeof( array ) / sizeof( ( array )[0] ) )

#if 0
void Win32InitializeSSL()
{
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

void Win32ShutdownSSL(SSL* ssl)
{
    SSL_shutdown(ssl);
    SSL_free(ssl);
}

void Win32ShowCerts(SSL* ssl)
{   X509 *cert;
    char *line;
    
    cert = SSL_get_peer_certificate(ssl);	/* get the server's certificate */
    if(cert != NULL)
    {
        printf( "Server certificates:\n" );
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);							/* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);							/* free the malloc'ed string */
        X509_free(cert);					/* free the malloc'ed certificate copy */
    }
    else
    {
        printf("No certificates.\n");
    }
}


OpenSSLConnection()
{
    if(useSSL)
    {
        SSL_CTX* sslProtocol = SSL_CTX_new(SSLv23_client_method());
        SSL* ssl = SSL_new(sslProtocol);						
        SSL_set_fd(ssl, fd);				
        connection->SSLHandle = ssl;
        while(1)
        {
            i32 errorCode = SSL_connect(ssl);
            if(errorCode == 1)
            {
                break;
            }
            
            int error = SSL_get_error(ssl, errorCode);
            switch(error)
            {
                case SSL_ERROR_WANT_WRITE:
                {
                } break;
                case SSL_ERROR_WANT_READ:                
                {
                } break;
                
                default: break;
            }
        }
    }
}



ServerSSLSecureSocekt()
{
    
#if 0    
    if(secureSocket)
    {
        SSL_CTX* sslctx = SSL_CTX_new(SSLv23_server_method());
        SSL_CTX_set_options(sslctx, SSL_OP_SINGLE_DH_USE);
        result.sslProtocol = sslctx;
        
        int use_cert = SSL_CTX_use_certificate_file(sslctx, "cert.pem" , SSL_FILETYPE_PEM);
        int use_prv = SSL_CTX_use_PrivateKey_file(sslctx, "key.pem", SSL_FILETYPE_PEM);
    }
#endif
    
}

#endif








inline b32 Win32InitNetwork()
{
    b32 result = false;
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) == NO_ERROR)
    {
        result = true;
    }
    
    return result;
}

inline u32 Win32GetNonBlockingSockedDescriptorClient(char* serverHost, char* serverPort, sockaddr* addr, u32* addrSize)
{
    addrinfo hints;
    addrinfo *servinfo;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    
    int rv;
    if((rv = getaddrinfo(serverHost, serverPort, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 0;
    }
    
    u32 result = 0;
    for(addrinfo* p = servinfo; p; p = p->ai_next) 
    {
        i32 sockfd = (i32) socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(sockfd > 0)
        {
            *addr = *p->ai_addr;
            *addrSize = (u32) p->ai_addrlen;
            result = sockfd;
            int n = 1024 * 1024 * 8;
            if(setsockopt(result, SOL_SOCKET, SO_RCVBUF, (const char*) &n, sizeof(n)) == -1) 
            {
                InvalidCodePath;
            }
            
            n = 1024 * 1024 * 8;
            if(setsockopt(result, SOL_SOCKET, SO_SNDBUF, (const char*) &n, sizeof(n)) == -1) 
            {
                InvalidCodePath;
            }
            
            
            u_long nonblocking_enabled = TRUE;
            ioctlsocket(sockfd, FIONBIO, &nonblocking_enabled);
            
            break;
        }
    }
    freeaddrinfo(servinfo);
    return result;
}

inline u32 Win32GetNonBlockingSockedDescriptorServer(char* port)
{
    addrinfo hints;
    addrinfo *servinfo;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    
    int rv;
    if((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 0;
    }
    
    u32 result = 0;
    for(addrinfo* p = servinfo; p; p = p->ai_next)
    {
        i32 sockfd = (i32) socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(sockfd > 0)
        {
            if(bind(sockfd, p->ai_addr, (int) p->ai_addrlen) == -1) 
            {
                InvalidCodePath;
            }
            result = sockfd;
            
            int n = 1024 * 1024;
            if(setsockopt(result, SOL_SOCKET, SO_RCVBUF, (const char*) &n, sizeof(n)) == -1) 
            {
                InvalidCodePath;
            }
            
            n = 1024 * 1024 * 16;
            if(setsockopt(result, SOL_SOCKET, SO_SNDBUF, (const char*) &n, sizeof(n)) == -1) 
            {
                InvalidCodePath;
            }
            
            u_long nonblocking_enabled = TRUE;
            ioctlsocket(sockfd, FIONBIO, &nonblocking_enabled);
            
            break;
        }
        else
        {
            InvalidCodePath;
        }
    }
    freeaddrinfo(servinfo);
    return result;
}

inline b32 PacketIndexGreater(u16 s1, u16 s2)
{
    b32 result = (( s1 > s2) && (s1 - s2 <= 32768)) || 
        ((s1 < s2) && (s2 - s1  > 32768));
    return result;
}

inline b32 PacketIndexSmaller(u16 s1, u16 s2)
{
    b32 result = (s1 != s2) && (!PacketIndexGreater(s1, s2));
    return result;
}

inline b32 IsSet(u32 ackedBits, u16 bitIndex)
{
    Assert(bitIndex < 32);
    b32 result = (ackedBits & (1 << bitIndex));
    return result;
}

inline u32 SetBitIndex(u32 bits, u16 bitIndex)
{
    Assert(bitIndex < 32);
    u32 result = bits | (1 << bitIndex);
    return result;
}

inline u16 Delta(u16 packetIndex1, u16 packetIndex2)
{
    Assert(PacketIndexSmaller(packetIndex2, packetIndex1));
    u32 p1 = (u32) packetIndex1;
    u32 p2 = (u32) packetIndex2;
    
    if(packetIndex2 > packetIndex1)
    {
        p1 += (0xffff + 1);
    }
    
    u16 result = (u16) (p1 - p2);
    return result;
}

inline b32 Acked(NetworkAck ack, u16 packetIndex)
{
    b32 result;
    
    if(packetIndex == ack.progressiveIndex)
    {
        result = true;
    }
    else
    {
        result = false;
        if(PacketIndexSmaller(packetIndex, ack.progressiveIndex))
        {
            u16 delta = Delta(ack.progressiveIndex, packetIndex);
			Assert(delta > 0);
            if(delta <= 32)
            {
                result = IsSet(ack.bits, delta - 1);
            }
            
        }
    }
    
    return result;
}

inline void DispatchQueuedAcks(NetworkConnection* connection)
{
    BeginNetMutex(&connection->mutex);
    NetworkBufferedAck* firstToDispatch = connection->firstQueuedAck;
    connection->firstQueuedAck = 0;
    EndNetMutex(&connection->mutex);
    
    if(firstToDispatch)
    {
        NetworkBufferedAck* lastDispatched = 0;
        for(NetworkBufferedAck* toDispatch = firstToDispatch; toDispatch; toDispatch = toDispatch->next)
        {
            for(NetworkBufferedPacket* packet = connection->sendQueueSentinel.next; packet != &connection->sendQueueSentinel;)
            {
                NetworkBufferedPacket* next = packet->next;
                if(Acked(toDispatch->ack, packet->progressiveIndex))
                {
                    if(packet == connection->lastSent)
                    {
                        connection->lastSent = packet->prev;
                    }
                    
                    NETDLLIST_REMOVE(packet);
                    
                    BeginNetMutex(&connection->mutex);
                    NETFREELIST_DEALLOC(packet, connection->firstFreePacket);
                    EndNetMutex(&connection->mutex);
                }
                
                packet = next;
            }
            
            lastDispatched = toDispatch;
        }
        
        BeginNetMutex(&connection->mutex);
        lastDispatched->nextFree = connection->firstFreeAck;
        connection->firstFreeAck = firstToDispatch;
        EndNetMutex(&connection->mutex);
    }
}

inline void QueueAck(NetworkConnection* connection, u16 ackedIndex, u32 ackedBits)
{
    BeginNetMutex(&connection->mutex);
    NetworkBufferedAck* ack;
    NETFREELIST_ALLOC(ack, connection->firstFreeAck, (NetworkBufferedAck*) malloc(sizeof(NetworkAck)));
    ack->ack.progressiveIndex = ackedIndex;
    ack->ack.bits = ackedBits;
    NETFREELIST_INSERT(ack, connection->firstQueuedAck);
    EndNetMutex(&connection->mutex);
}

inline PacketData* GetPacketData(NetworkConnection* connection, u16 packetIndex)
{
	u16 index = packetIndex % ArrayC(connection->receivedPackets);
	PacketData* result = connection->receivedPackets + index;
    
	return result;
}

inline b32 Received(NetworkConnection* connection, u16 packetIndex)
{
	PacketData* packet = GetPacketData(connection, packetIndex);
	b32 result = (packetIndex == packet->packetIndex);
    
	return result;
}

inline void Clear(NetworkConnection* connection, u16 packetIndex)
{
	PacketData* packet = GetPacketData(connection, packetIndex);
	packet->packetIndex = 0xffffffff;
}

inline void AddPacketReceived(NetworkConnection* connection, u16 packetIndex)
{
	PacketData* packet = GetPacketData(connection, packetIndex);
	packet->packetIndex = (u32) packetIndex;
}



inline void UpdateLastReceived(NetworkConnection* connection, u16 packetIndex)
{
    NetworkAck newAckToInclude = connection->ackToInclude;
    
    if(packetIndex != newAckToInclude.progressiveIndex)
    {
        u16 bitsToInclude = 32;
        if(PacketIndexGreater(packetIndex, newAckToInclude.progressiveIndex))
        {
            u16 oldBiggest = newAckToInclude.progressiveIndex;
            u16 newBiggest = packetIndex;
            
            newAckToInclude.progressiveIndex = newBiggest;
            newAckToInclude.bits = 0;
            
            
            u16 index = oldBiggest + 1;
            u16 destIndex = newBiggest;
            while(PacketIndexSmaller(index, destIndex))
            {
                Clear(connection, index++);
            }
            
            
            index = newBiggest - bitsToInclude;
            destIndex = newBiggest;
            
			u16 bitIndex = 31;
            while(true)
            {
                if(Received(connection, index))
                {
                    newAckToInclude.bits = SetBitIndex(newAckToInclude.bits, bitIndex);
                }
                
                if(++index == destIndex)
                {
					Assert(bitIndex == 0);
                    break;
                }
                
				--bitIndex;
            }
            
			AddPacketReceived(connection, newBiggest);
        }
        else
        {
            u16 delta = Delta(newAckToInclude.progressiveIndex, packetIndex);
            Assert(delta > 0);
            if(delta <= bitsToInclude)
            {
                AddPacketReceived(connection, packetIndex);
                newAckToInclude.bits = SetBitIndex(newAckToInclude.bits, delta - 1);
            }
        }
		
		BeginNetMutex(&connection->mutex);
		connection->ackToInclude = newAckToInclude;
		EndNetMutex(&connection->mutex);
    }
}

inline void SignalPacketLost()
{
	//AdjustRTT();
	//AdjustBandWidth();
}

inline u32 GetBandwidth(NetworkConnection* connection)
{
    u32 result = 100 * 1024;
    return result;
}

NETWORK_QUEUE_PACKET(Win32QueuePacket)
{
    NetworkConnection* connection = network->connections + connectionSlot;
    
    NetworkBufferedPacket* newPacket;
    BeginNetMutex(&connection->mutex);
    NETFREELIST_ALLOC(newPacket, connection->firstFreePacket, (NetworkBufferedPacket*) malloc(sizeof(NetworkBufferedPacket)));
    EndNetMutex(&connection->mutex);
    
    newPacket->progressiveIndex = connection->nextProgressiveIndex++;
    
    Assert(size && size <= ArrayC(newPacket->data));
    newPacket->dataSize = size;
    memcpy(newPacket->data, data, size);
    
    newPacket->flags = flags;
    newPacket->timeInFlight = 0;
    
    NETDLLIST_INSERT_AS_LAST(&connection->sendQueueSentinel, newPacket);
}

NETWORK_FLUSH_SEND_QUEUE(Win32FlushSendQueue)
{
    b32 result = false;
    
    
    NetworkConnection* connection = network->connections + connectionSlot;
    DispatchQueuedAcks(connection);
    
    u32 dataToSend = GetBandwidth(connection);
    b32 sendPackets = false;
    if(!connection->lastSent || connection->lastSent == &connection->sendQueueSentinel)
    {
        sendPackets = true;
    }
    
    
    NetworkBufferedPacket* packet = connection->sendQueueSentinel.next;
    while(packet != &connection->sendQueueSentinel)
    {
        NetworkBufferedPacket* next = packet->next;
        Assert(next);
        
        packet->timeInFlight += elapsedTime;
        if(packet->timeInFlight >= 1.0f)
        {
            SignalPacketLost();
            NETDLLIST_REMOVE(packet);
            
            if(packet->flags & NetworkFlags_GuaranteedDelivery && connection->salt)
            {
                packet->timeInFlight = 0;
                packet->progressiveIndex = connection->nextProgressiveIndex++;
                NETDLLIST_INSERT_AS_LAST(&connection->sendQueueSentinel, packet);
            }
            else
            {
                BeginNetMutex(&connection->mutex);
                NETFREELIST_INSERT(packet, connection->firstFreePacket);
                EndNetMutex(&connection->mutex);
            }
        }
        
        
		if(dataToSend > 0)
		{
			if(sendPackets)
			{
				BeginNetMutex(&connection->mutex);
				NetworkAck ackToInclude = connection->ackToInclude;
				EndNetMutex(&connection->mutex);
                
				unsigned char buff_[2048];
                
				u32 totalSize = 0;
				unsigned char* buff = PackHeader_(buff_, STARTNUMBER, connection->counterpartConnectionSlot, packet->flags, packet->progressiveIndex, ackToInclude.progressiveIndex, ackToInclude.bits);
				memcpy(buff, packet->data, packet->dataSize);
				buff += packet->dataSize;
				totalSize = PackTrailer_(buff_, buff);
                
				if(sendto(network->fd, (const char*) buff_, totalSize, 0,
                          (const sockaddr*) connection->counterpartAddress, connection->counterpartAddrSize) < 0)
				{
					InvalidCodePath;
				}
                
				dataToSend -= totalSize;
                connection->lastSent = packet;
			}
            
            
            if(packet == connection->lastSent)
			{
				sendPackets = true;
			}
		}
        
        packet = next;
    }
    
    result = NETDLLIST_ISEMPTY(&connection->sendQueueSentinel);
    return result;
}

inline void Win32FreeConnection(NetworkInterface* network, NetworkConnection* connection)
{
    connection->counterpartConnectionSlot = 0;
    connection->nextProgressiveIndex = 0;
    connection->ackToInclude = {};
    connection->ackToInclude.progressiveIndex = 0xffff;
    
    if(!NETDLLIST_ISEMPTY(&connection->sendQueueSentinel))
    {
        connection->sendQueueSentinel.prev ->next = connection->firstFreePacket;
        connection->firstFreePacket = connection->sendQueueSentinel.next;
        NETDLLIST_INIT(&connection->sendQueueSentinel);
    }
    
    if(!NETDLLIST_ISEMPTY(&connection->recvQueueSentinel))
    {
        connection->recvQueueSentinel.prev ->next = connection->firstFreePacket;
        connection->firstFreePacket = connection->recvQueueSentinel.next;
        NETDLLIST_INIT(&connection->recvQueueSentinel);
    }
    
    NETFREELIST_FREE(connection->firstQueuedAck, NetworkBufferedAck, connection->firstFreeAck);
    
    
    Assert(NETDLLIST_ISEMPTY(&connection->sendQueueSentinel));
    Assert(NETDLLIST_ISEMPTY(&connection->recvQueueSentinel));
    
    BeginNetMutex(&network->connectionMutex);
    connection->nextFree = network->firstFreeConnection;
    network->firstFreeConnection = connection;
    EndNetMutex(&network->connectionMutex);
}

NETWORK_CLOSE_CONNECTION(Win32CloseConnection)
{
    if(!network->nextConnectionIndex)
    {
        Assert(!connectionSlot);
    }
    else
    {
        Assert(connectionSlot);
    }
    NetworkConnection* connection = network->connections + connectionSlot;
    if(connection)
    {
        unsigned char disconnectMsg_[64];
        unsigned char* disconnectMsg = PackHeader_(disconnectMsg_, DISCONNECTNUMBER, connection->counterpartConnectionSlot, 0, 0, 0, 0);
        disconnectMsg += pack(disconnectMsg, "L", connection->salt);
        u32 totalSize = PackTrailer_(disconnectMsg_, disconnectMsg);
        
        for(u32 i = 0; i < 10; ++i)
        {
            if(sendto(network->fd, (const char*) disconnectMsg_, totalSize, 0, (sockaddr*) connection->counterpartAddress, connection->counterpartAddrSize) < 0)
            {
                InvalidCodePath;
            }
        }
        
        WriteBarrier;
        connection->salt = 0;
        WriteBarrier;
        
        Win32FreeConnection(network, connection);
    }
}

static u32 clientSalt = 1111;
NETWORK_OPEN_CONNECTION(Win32OpenConnection)
{
    NetworkConnection* connection = &network->clientConnection;
    Assert(!connection->salt);
    if(network->fd)
    {
        closesocket(network->fd);
    }
    
    char portStr[8];
    sprintf(portStr, "%u", port);
    
    sockaddr sockinfo;
    u32 addrlen;
    network->fd = (u32) Win32GetNonBlockingSockedDescriptorClient(IP, portStr, &sockinfo, &addrlen);
    
    Assert(addrlen <= sizeof(connection->counterpartAddress));
    memcpy(&connection->counterpartAddress, &sockinfo, addrlen);
    connection->counterpartAddrSize = (int) addrlen;
    
    network->maxConnectionCount = 0;
    network->connections = connection;
    
    connection->nextProgressiveIndex = 0;
    connection->lastSent = 0;
    
    BeginNetMutex(&connection->mutex);
    
    
    if(connection->sendQueueSentinel.next)
    {
        if(!NETDLLIST_ISEMPTY(&connection->sendQueueSentinel))
        {
            connection->sendQueueSentinel.prev ->next = connection->firstFreePacket;
            connection->firstFreePacket = connection->sendQueueSentinel.next;
        }
    }
    NETDLLIST_INIT(&connection->sendQueueSentinel);
    
    if(connection->recvQueueSentinel.next)
    {
        if(!NETDLLIST_ISEMPTY(&connection->recvQueueSentinel))
        {
            connection->recvQueueSentinel.prev ->next = connection->firstFreePacket;
            connection->firstFreePacket = connection->recvQueueSentinel.next;
        }
    }
    NETDLLIST_INIT(&connection->recvQueueSentinel);
    
    
    NETFREELIST_FREE(connection->firstQueuedAck, NetworkBufferedAck, connection->firstFreeAck);
    
    connection->ackToInclude = {};
    connection->ackToInclude.progressiveIndex = 0xffff;
    
    EndNetMutex(&connection->mutex);
    
    while(true)
    {
        unsigned char helloMsg_[64];
        unsigned char* endHelloMsg = PackHeader_(helloMsg_, HELLONUMBER, 0, 0, 0, 0, 0);
        endHelloMsg += pack(endHelloMsg, "L", clientSalt);
        u32 totalSize = PackTrailer_(helloMsg_, endHelloMsg);
        
        if(sendto(network->fd, (const char*) helloMsg_, totalSize, 0, &sockinfo, (int) addrlen) < 0)
        {
            InvalidCodePath;
        }
        
        sockaddr_storage server;
        int size = sizeof(server);
        
        if(recvfrom(network->fd, (char*) network->recvBuffer, sizeof(network->recvBuffer), 0, (sockaddr*) &server, &size) > 0)
        {
            PacketHeader header;
            unsigned char* packet = UnpackHeader_(network->recvBuffer, &header);
            u32 salt;
            u32 endNumber;
            packet = unpack(packet, "LL", &salt, &endNumber);
            
            if(header.magicNumber == HELLONUMBER && salt == clientSalt && endNumber == ENDNUMBER)
            {
                connection->counterpartConnectionSlot = header.connectionSlot;
                
                connection->salt = clientSalt;
                break;
            }
        }
        
        Sleep(100);
    }
    
    ++clientSalt;
}

NETWORK_GET_PACKET(Win32GetPacket)
{
    NetworkPacketReceived result = {};
    
    NetworkConnection* connection = network->connections + connectionSlot;
	BeginNetMutex(&connection->mutex);
    NetworkBufferedPacket* firstToReceive = connection->recvQueueSentinel.next;
    
    result.disconnected = (connection->salt == 0);
    if(firstToReceive != &connection->recvQueueSentinel)
    {
        result.dataSize = firstToReceive->dataSize;
        result.flags = firstToReceive->flags;
        memcpy(result.data, firstToReceive->data, firstToReceive->dataSize);
        
        NETDLLIST_REMOVE(firstToReceive);
        NETFREELIST_DEALLOC(firstToReceive, connection->firstFreePacket);
    }
    
	EndNetMutex(&connection->mutex);
    return result;
}


NETWORK_ACCEPT(Win32Accept)
{
    u16 accepted = network->newConnectionCount;
    Assert(accepted < maxAccepted);
    
    for(u32 connectionIndex = 0; connectionIndex < accepted; ++connectionIndex)
    {
        acceptedSlots[connectionIndex] = network->newConnections[connectionIndex].slot;
        network->newConnections[connectionIndex].accepted = true;
    }
    
    WriteBarrier;
    network->newConnectionsAccepted = true;
    
    return accepted;
}

NETWORK_RECEIVE_DATA(Win32ReceiveData)
{
    if(network->newConnectionsAccepted)
    {
        for(u32 acceptedIndex = 0; acceptedIndex < network->newConnectionCount;)
        {
            NetworkNewConnection* newConnection = network->newConnections + acceptedIndex;
            if(newConnection->accepted)
            {
                *newConnection = network->newConnections[--network->newConnectionCount];
            }
            else
            {
                ++acceptedIndex;
            }
        }
        
        network->newConnectionsAccepted = false;
    }
    
    if(network->maxConnectionCount > 0 || network->clientConnection.salt)
    {
        while(true)
        {
            sockaddr_in client;
            int size = sizeof(client);
            i32 bytesRead = recvfrom(network->fd, (char*) network->recvBuffer, sizeof(network->recvBuffer), 0, (sockaddr*) &client, &size);
            
            if(bytesRead <= 0)
            {
                break;
            }
            
            network->totalBytesReceived += bytesRead;
            if(bytesRead > sizeof(PacketHeader))
            {
                PacketHeader header = {};
                unsigned char* start = UnpackHeader_(network->recvBuffer, &header);
                if((header.dataSize + sizeof(PacketHeader) + sizeof(PacketTrailer)) <= sizeof(network->recvBuffer))
                {
                    i32 end = unpacki32(start + header.dataSize);
                    if(end == ENDNUMBER)
                    {
                        if(header.magicNumber == HELLONUMBER)
                        {
                            if(network->maxConnectionCount > 0)
                            {
                                u16 connectionSlot = 0;
                                u32 saltToSendBack = 0;
                                
                                b32 alreadyLogged = false;
                                for(u16 connectionIndex = 0; connectionIndex < network->nextConnectionIndex; ++connectionIndex)
                                {
                                    NetworkConnection* connection = network->connections + connectionIndex;
                                    if(!memcmp(connection->counterpartAddress, &client, size))
                                    {
                                        connectionSlot = connectionIndex;
                                        saltToSendBack = connection->salt;
                                        alreadyLogged = true;
                                        break;
                                    }
                                }
                                
                                if(!alreadyLogged)
                                {
                                    u32 salt;
                                    unpack(start, "L", &salt);
                                    saltToSendBack = salt;
                                    
                                    BeginNetMutex(&network->connectionMutex);
                                    NetworkConnection* connection = network->firstFreeConnection;
                                    if(!connection)
                                    {
                                        connectionSlot = network->nextConnectionIndex++;
                                        connection = network->connections + connectionSlot;
                                    }
                                    else
                                    {
                                        connectionSlot = (u16) (connection - network->connections);
                                        network->firstFreeConnection = connection->nextFree;
                                    }
                                    EndNetMutex(&network->connectionMutex);
                                    
                                    connection->salt = salt;
                                    
                                    
                                    connection->nextProgressiveIndex = 0;
                                    connection->lastSent = 0;
                                    
                                    NETDLLIST_INIT(&connection->sendQueueSentinel);
                                    NETDLLIST_INIT(&connection->recvQueueSentinel);
                                    
                                    Assert(!connection->firstQueuedAck);
                                    connection->ackToInclude = {};
                                    connection->ackToInclude.progressiveIndex = 0xffff;
                                    
                                    
                                    Assert(size <= sizeof(connection->counterpartAddress));
                                    memcpy(connection->counterpartAddress, &client, size);
                                    connection->counterpartAddrSize = size;
                                    
                                    NetworkNewConnection* newConnection = network->newConnections + network->newConnectionCount;
                                    
                                    newConnection->accepted = false;
                                    newConnection->slot = connectionSlot;
                                    
                                    WriteBarrier;
                                    ++network->newConnectionCount;
                                    
                                }
                                
                                unsigned char helloResponse[64];
                                unsigned char* packet = PackHeader_(helloResponse, HELLONUMBER, connectionSlot, 0, 0, 0, 0);
                                packet += pack(packet, "L", saltToSendBack);
                                u32 totalSize = PackTrailer_(helloResponse, packet);
                                
                                if(sendto(network->fd, (const char*) helloResponse, totalSize, 0, (sockaddr*) &client, size) < 0)
                                {
                                    InvalidCodePath;
                                }
                            }
                        }
                        
                        else if(header.magicNumber == STARTNUMBER)
                        {
                            if(!network->nextConnectionIndex || header.connectionSlot < network->nextConnectionIndex)
                            {
                                NetworkConnection* connection = network->connections + header.connectionSlot;
                                
                                if(connection->salt)
                                {
                                    UpdateLastReceived(connection, header.progressiveIndex);
                                    QueueAck(connection, header.acked, header.ackedBits);
                                    
                                    BeginNetMutex(&connection->mutex);
                                    NetworkBufferedPacket* recv;
                                    NETFREELIST_ALLOC(recv, connection->firstFreePacket, (NetworkBufferedPacket*) malloc(sizeof(NetworkBufferedPacket)));
                                    recv->flags = header.flags;
                                    recv->progressiveIndex = header.progressiveIndex;
                                    recv->dataSize = header.dataSize;
                                    memcpy(recv->data, start, header.dataSize);
                                    
                                    NETDLLIST_INSERT_AS_LAST(&connection->recvQueueSentinel, recv);
                                    EndNetMutex(&connection->mutex);
                                }
                            }
                        }
                        else if(header.magicNumber == DISCONNECTNUMBER)
                        {
                            if(!network->nextConnectionIndex || header.connectionSlot < network->nextConnectionIndex)
                            {
                                NetworkConnection* connection = network->connections + header.connectionSlot;
                                
                                u32 salt;
                                unpack(start, "L", &salt);
                                if(connection->salt == salt)
                                {
                                    connection->salt = 0;
                                }
                            }
                            else
                            {
                                InvalidCodePath;
                            }
                        }
                    }
                }
            }
        }
    }
}

NETWORK_INIT_SERVER(Win32InitServer)
{
    char portStr[8];
    sprintf(portStr, "%u", port);
    
    network->fd = Win32GetNonBlockingSockedDescriptorServer(portStr);
    if(!network->fd)
    {
        InvalidCodePath;
    }
    
    network->connections = connections;
    network->maxConnectionCount = maxConnectionCount;
    network->nextConnectionIndex = 1;
}


NetworkAPI Win32NetworkAPI =
{
    Win32QueuePacket,
    Win32ReceiveData,
    Win32FlushSendQueue,
    Win32Accept,
    Win32GetPacket,
    Win32OpenConnection,
    Win32CloseConnection,
    Win32InitServer,
};
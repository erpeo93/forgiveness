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






NETWORK_SEND_DATA(Win32SendData)
{
    NetworkConnection* connection = network->connections + connectionSlot;
    
    Assert(channelIndex < MAX_CHANNELS);
    NetworkChannel* channel = network->channels + channelIndex;
    NetworkChannelInfo* channelInfo = connection->channelInfo + channelIndex;
    
    u16 totalSize = 0;
    unsigned char* buff_ = 0;
    if(size)
    {
        buff_ = network->sendBuffer;
        Assert((size + sizeof(PacketHeader) + sizeof(PacketTrailer)) <= sizeof(network->sendBuffer));
        
        unsigned char* buff = PackHeader_(buff_, STARTNUMBER, connection->counterpartConnectionSlot, channelIndex, channelInfo->nextProgressiveIndexSend++);
        
        memcpy(buff, data, size);
        buff += size;
        totalSize = PackTrailer_(buff_, buff);
        
        Assert(totalSize > 0);
    }
    
    if(channel->ordered)
    {
        if(sendUnackedPackets)
        {
			u32 unackedPacketSendIndex = channelInfo->runningUnackedIndex;
			while(true)
			{
				u16 packetSize = channelInfo->unackedPacketsSize[unackedPacketSendIndex];
                if(packetSize)
                {
                    u8* packet = channelInfo->unackedPackets[unackedPacketSendIndex];
                    PacketHeader resendHeader;
                    UnpackHeader_(packet, &resendHeader);
                    
                    Assert(resendHeader.totalPacketSize);
                    
                    char resendString[64];
                    sprintf(resendString, "Resending: %u\n", resendHeader.progressiveIndex);
                    OutputDebugString(resendString);
                    
                    sendto(network->fd, (const char*) packet, packetSize, 0,
                           (const sockaddr*) connection->counterpartAddress, connection->counterpartAddrSize);
					
					if(++unackedPacketSendIndex == ArrayC(channelInfo->unackedPackets))
					{
						unackedPacketSendIndex = 0;
					}
                    
					if(unackedPacketSendIndex == channelInfo->runningUnackedIndex)
					{
						break;
					}
                }
				else
				{
					break;
				}
			} 
        }
        
        
        if(size)
        {
            Assert(totalSize < ArrayC(channelInfo->unackedPackets[0]));
            u32 insertIndex = channelInfo->runningUnackedIndex;
            while(true)
            {
                u16 packetSize = channelInfo->unackedPacketsSize[insertIndex];
                if(!packetSize)
                {
                    u8* dest = channelInfo->unackedPackets[insertIndex];
                    memcpy(dest, buff_, totalSize);
                    channelInfo->unackedPacketsSize[insertIndex] = totalSize;
                    break;
                }
                
                if(++insertIndex == ArrayC(channelInfo->unackedPackets))
                {
                    insertIndex = 0;
                }
                
                if(insertIndex == channelInfo->runningUnackedIndex)
                {
					InvalidCodePath;
                    break;
                }
            } 
        }
    }
    
    if(size)
    {
        sendto(network->fd, (const char*) buff_, totalSize, 0,
               (const sockaddr*) connection->counterpartAddress, connection->counterpartAddrSize);
    }
	
    return true;
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
    connection->connected = false;
    
    
    unsigned char disconnectMsg_[64];
    unsigned char* disconnectMsg = PackHeader_(disconnectMsg_, DISCONNECTNUMBER, connection->counterpartConnectionSlot, 0, 0);
    disconnectMsg += pack(disconnectMsg, "L", connection->salt);
    u32 totalSize = PackTrailer_(disconnectMsg_, disconnectMsg);
    
    for(u32 i = 0; i < 10; ++i)
    {
        if(sendto(network->fd, (const char*) disconnectMsg_, totalSize, 0, (sockaddr*) connection->counterpartAddress, connection->counterpartAddrSize) < 0)
        {
            InvalidCodePath;
        }
    }
    
    
    
    
    connection->counterpartConnectionSlot = 0;
    connection->contextFirstPacketOffset = 0;
    connection->filledRecvBufferSize = 0;
    
    for(u32 channelIndex = 0; channelIndex < network->channelCount; ++channelIndex)
    {
        NetworkChannelInfo* info = connection->channelInfo + channelIndex;
        info->nextProgressiveIndexSend = 0;
        info->nextProgressiveIndexRecv = 0;
		info->runningUnackedIndex = 0;
        for(u32 packetIndex = 0; packetIndex < ArrayC(info->unackedPacketsSize); ++packetIndex)
        {
            info->unackedPacketsSize[packetIndex] = 0;
        }
    }
}

inline void NetworkAddChannel_(NetworkInterface* network, NetworkChannelParams* params)
{
    Assert(network->channelCount < MAX_CHANNELS);
    NetworkChannel* channel = network->channels + network->channelCount++;
    channel->ordered = params->ordered;
}


NETWORK_OPEN_CONNECTION(Win32OpenConnection)
{
    Assert(!network->clientConnection.connected);
    if(network->fd)
    {
        closesocket(network->fd);
    }
    
    char portStr[8];
    sprintf(portStr, "%u", port);
    
    sockaddr sockinfo;
    u32 addrlen;
    network->fd = (u32) Win32GetNonBlockingSockedDescriptorClient(IP, portStr, &sockinfo, &addrlen);
    
    Assert(addrlen <= sizeof(network->clientConnection.counterpartAddress));
    memcpy(&network->clientConnection.counterpartAddress, &sockinfo, addrlen);
    network->clientConnection.counterpartAddrSize = (int) addrlen;
    
    network->maxConnectionCount = 0;
    network->connections = &network->clientConnection;
    
    Assert(channelCount);
    for(u32 channelIndex = 0; channelIndex < channelCount; ++channelIndex)
    {
        NetworkChannelParams* params = channelParams + channelIndex;
        NetworkAddChannel_(network, params);
        NetworkChannelInfo* channelInfo = network->clientConnection.channelInfo + channelIndex;
        channelInfo->nextProgressiveIndexRecv = 0;
        channelInfo->nextProgressiveIndexSend = 0;
		channelInfo->runningUnackedIndex = 0;
        for(u32 packetIndex = 0; packetIndex < ArrayC(channelInfo->unackedPacketsSize); ++packetIndex)
        {
            channelInfo->unackedPacketsSize[packetIndex] = 0;
        }
    }
    
    network->clientConnection.salt = salt;
    network->clientConnection.contextFirstPacketOffset = 0;
    network->clientConnection.appRecv = appRecv;
    
    while(!network->clientConnection.connected)
    {
        unsigned char helloMsg_[64];
        unsigned char* endHelloMsg = PackHeader_(helloMsg_, HELLONUMBER, 0, 0, 0);
        endHelloMsg += pack(endHelloMsg, "L", salt);
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
            u32 endNumber;
            packet = unpack(packet, "L", &endNumber);
            
            if(header.magicNumber == HELLONUMBER && endNumber == ENDNUMBER)
            {
                network->clientConnection.counterpartConnectionSlot = header.connectionSlot;
                network->clientConnection.connected = true;
            }
        }
        
        Sleep(100);
    }
}

NETWORK_GET_PACKET(Win32GetPacket)
{
    NetworkPacketReceived result = {};
    
    NetworkConnection* connection = network->connections + connectionSlot;
    if(!connection->connected)
    {
        result.disconnected = true;
    }
    
    if(connection->contextFirstPacketOffset < connection->filledRecvBufferSize)
    {
        unsigned char* packet = connection->appRecv.buffer + connection->contextFirstPacketOffset;
        NetworkApplicationHeader* header = (NetworkApplicationHeader*) packet;
        Assert(header->dataSize);
        result.info = *header;
        result.packetPtr = packet + sizeof(NetworkApplicationHeader);
        
        u32 totalPacketSize = sizeof(NetworkApplicationHeader) + header->dataSize;
        Assert(connection->contextFirstPacketOffset <= connection->appRecv.size);
        
        WriteBarrier;
        connection->contextFirstPacketOffset += totalPacketSize;
    }
    
    return result;
}


inline void DispatchToApplicationBuffer(NetworkInterface* network, NetworkConnection* connection, u8* msg, u16 dataSize, u8 channelIndex, u16 progressiveIndex)
{
    NetworkChannel* channel = network->channels + channelIndex;
    NetworkChannelInfo* info = connection->channelInfo + channelIndex;
    if(channel->ordered)
    {
        i64 expected = (i64) info->nextProgressiveIndexRecv;
        i64 arrived = (i64) progressiveIndex;
        
		if(arrived == 53)
		{
			int a = 5;
		}
        
        char debug[64];
        sprintf(debug, "expected: %u, arrived: %u\n", info->nextProgressiveIndexRecv, progressiveIndex); 
        OutputDebugString(debug);
        if(arrived < expected)
        {
            arrived += 0xffff;
        }
        
        i64 delta = arrived - expected;
        Assert(delta >= 0);
        if(delta < SLIDING_WINDOW_SIZE)
        {
            Assert(dataSize < sizeof(info->receivingSlidingWindow[0]));
            u8* dest = info->receivingSlidingWindow[delta];
            memcpy(dest, msg, dataSize);
            info->packetSize[delta] = dataSize;
        }
		else
		{
		}
        
        
        u16 packetCopyied = 0;
        b32 copyPackets = true;
        
        u16 ack = info->nextProgressiveIndexRecv - 1;
        
        for(u16 d = 0; d < SLIDING_WINDOW_SIZE; ++d)
        {
            u32 packetSize = info->packetSize[d];
            u8* packet = info->receivingSlidingWindow[d];
            if(packetSize)
            {
                ++ack;
                if(copyPackets)
                {
                    NetworkApplicationHeader applHeader = {};
                    applHeader.dataSize = packetSize;
                    applHeader.progressiveIndex = ack;
                    applHeader.channelIndex = channelIndex;
                    applHeader.brokeChain = false;
                    
                    u32 size = applHeader.dataSize + sizeof(NetworkApplicationHeader);
                    if(connection->filledRecvBufferSize + packetSize <= connection->appRecv.size)
                    {
                        ++packetCopyied;
                        
                        u8* dest = connection->appRecv.buffer + connection->filledRecvBufferSize;
                        memcpy(dest, &applHeader, sizeof(NetworkApplicationHeader));
                        memcpy(dest + sizeof(NetworkApplicationHeader), packet, packetSize);
                        
                        WriteBarrier;
                        connection->filledRecvBufferSize += size;
                    }
                    else if(connection->filledRecvBufferSize == connection->contextFirstPacketOffset)
                    {
                        ++packetCopyied;
                        
                        u8* dest = connection->appRecv.buffer;
                        memcpy(dest, &applHeader, sizeof(NetworkApplicationHeader));
                        memcpy(dest + sizeof(NetworkApplicationHeader), packet, packetSize);
                        connection->filledRecvBufferSize = size;
                        
                        WriteBarrier;
                        connection->contextFirstPacketOffset = 0;
                    }
                    else
                    {
						OutputDebugString("recv buffer full!\n");
                        copyPackets = false;
                    }
                }
            }
            else
            {
                break;
            }
        }
        
        char debugString[64];
		sprintf(debugString, "Sending Ack: %d\n", ack);
		OutputDebugString(debugString);
        
        unsigned char ackPacket[64];
        unsigned char* packet = PackHeader_(ackPacket, ACKNUMBER, connection->counterpartConnectionSlot, channelIndex, ack);
        u32 totalSize = PackTrailer_(ackPacket, packet);
        if(sendto(network->fd, (const char*) ackPacket, totalSize, 0, (sockaddr*) connection->counterpartAddress, connection->counterpartAddrSize) < 0)
        {
            InvalidCodePath;
        }
        
        if(packetCopyied)
        {
            for(u32 indexToCopy = packetCopyied; indexToCopy < SLIDING_WINDOW_SIZE; ++indexToCopy)
            {
                u32 destIndex = indexToCopy - packetCopyied;
                memcpy(info->receivingSlidingWindow[destIndex], info->receivingSlidingWindow[indexToCopy], info->packetSize[indexToCopy]);
                info->packetSize[destIndex] = info->packetSize[indexToCopy];
            }
            
            for(u32 indexToReset = SLIDING_WINDOW_SIZE - packetCopyied; indexToReset < SLIDING_WINDOW_SIZE; ++indexToReset)
            {
                info->packetSize[indexToReset] = 0;
            }
        }
        info->nextProgressiveIndexRecv += packetCopyied;
    }
    else
    {
        i32 progressiveDelta = (i32) progressiveIndex - (i32) info->nextProgressiveIndexRecv;
        //Assert(progressiveDelta == 0 || progressiveDelta == 0xffff);
        
        if(progressiveDelta >= 0 ||
           -progressiveDelta > (0xffff >> 1))
        {
            b32 brokeChain = (progressiveIndex != info->nextProgressiveIndexRecv);
            NetworkApplicationHeader applHeader = {};
            applHeader.dataSize = dataSize;
            applHeader.progressiveIndex = progressiveIndex;
            applHeader.channelIndex = channelIndex;
            applHeader.brokeChain = brokeChain;
            
            u32 size = applHeader.dataSize + sizeof(NetworkApplicationHeader);
            if(connection->filledRecvBufferSize + size <= connection->appRecv.size)
            {
                u8* dest = connection->appRecv.buffer + connection->filledRecvBufferSize;
                memcpy(dest, &applHeader, sizeof(NetworkApplicationHeader));
                memcpy(dest + sizeof(NetworkApplicationHeader), msg, dataSize);
                
                WriteBarrier;
                connection->filledRecvBufferSize += size;
            }
            else if(connection->filledRecvBufferSize == connection->contextFirstPacketOffset)
            {
                u8* dest = connection->appRecv.buffer;
                memcpy(dest, &applHeader, sizeof(NetworkApplicationHeader));
                memcpy(dest + sizeof(NetworkApplicationHeader), msg, applHeader.dataSize);
                connection->filledRecvBufferSize = size;
                
                WriteBarrier;
                connection->contextFirstPacketOffset = 0;
            }
            else
            {
                // NOTE(Leonardo): dropped!
            }
            info->nextProgressiveIndexRecv = progressiveIndex + 1;
        }
    }
}

NETWORK_RECEIVE_DATA_ACCEPT(Win32ReceiveDataAccept)
{
    network->newConnectionCount = 0;
    
    if(network->maxConnectionCount > 0 || network->clientConnection.connected)
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
				if(header.progressiveIndex == 53 && header.channelIndex == 1)
				{
					int a = 5;
				}
                
                if(header.totalPacketSize <= sizeof(network->recvBuffer))
                {
                    u32 trailerSize = sizeof(PacketTrailer);
                    Assert(trailerSize == 4);
                    u32 toAdvance = (header.totalPacketSize - trailerSize);
                    i32 end = unpacki32(network->recvBuffer + toAdvance);
                    if(end == ENDNUMBER)
                    {
                        if(header.magicNumber == HELLONUMBER)
                        {
                            if(network->maxConnectionCount > 0)
                            {
                                u16 connectionSlot = 0;
                                
                                b32 alreadyLogged = false;
                                for(u16 connectionIndex = 0; connectionIndex < network->nextConnectionIndex; ++connectionIndex)
                                {
                                    NetworkConnection* connection = network->connections + connectionIndex;
                                    if(!memcmp(connection->counterpartAddress, &client, size))
                                    {
                                        connectionSlot = connectionIndex;
                                        alreadyLogged = true;
                                        break;
                                    }
                                }
                                
                                if(!alreadyLogged)
                                {
                                    u32 salt;
                                    unpack(start, "L", &salt);
                                    
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
                                    
                                    connection->salt = salt;
                                    connection->connected = true;
                                    
                                    Assert(size <= sizeof(connection->counterpartAddress));
                                    memcpy(connection->counterpartAddress, &client, size);
                                    connection->counterpartAddrSize = size;
                                    network->newConnections[network->newConnectionCount++] = connectionSlot;
                                    
                                }
                                
                                unsigned char helloResponse[64];
                                unsigned char* packet = PackHeader_(helloResponse, HELLONUMBER, connectionSlot, 0, 0);
                                u32 totalSize = PackTrailer_(helloResponse, packet);
                                
                                if(sendto(network->fd, (const char*) helloResponse, totalSize, 0, (sockaddr*) &client, size) < 0)
                                {
                                    InvalidCodePath;
                                }
                            }
                        }
                        
                        else if(header.magicNumber == ACKNUMBER)
                        {  
                            char ackedString[64];
                            sprintf(ackedString, "arrived ack: %u\n", header.progressiveIndex);
                            OutputDebugString(ackedString);
                            
							u16 arrived = header.progressiveIndex;
                            NetworkConnection* connection = network->connections + header.connectionSlot;
                            NetworkChannelInfo* channelInfo = connection->channelInfo + header.channelIndex;
                            
							u32 startingIndex = channelInfo->runningUnackedIndex;
							while(true)
							{
								u16 packetSize = channelInfo->unackedPacketsSize[startingIndex];
								if(packetSize)
								{
									u8* packet = channelInfo->unackedPackets[startingIndex];
                                    PacketHeader unackedHeader;
                                    UnpackHeader_(packet, &unackedHeader);
                                    sprintf(ackedString, "matching: %u\n", unackedHeader.progressiveIndex);
                                    OutputDebugString(ackedString);
                                    
									if(unackedHeader.progressiveIndex <= arrived ||
                                       (unackedHeader.progressiveIndex - arrived >= (0xffff >> 1)))
                                    {
                                        sprintf(ackedString, "acked packet number: %u\n", unackedHeader.progressiveIndex);
                                        OutputDebugString(ackedString);
                                        channelInfo->unackedPacketsSize[startingIndex] = 0;
                                        
                                        
										if(arrived == unackedHeader.progressiveIndex)
										{
                                            if(++startingIndex == ArrayC(channelInfo->unackedPackets))
                                            {
                                                startingIndex = 0;
                                            }
                                            break;
										}
                                    }
                                    else
                                    {
                                        break;
                                    }
                                    
									if(++startingIndex == ArrayC(channelInfo->unackedPackets))
									{
										startingIndex = 0;
									}
                                    
									if(startingIndex == channelInfo->runningUnackedIndex)
									{
                                        InvalidCodePath;
										break;
									}
								}
								else
								{
									break;
								}
							}
                            
							channelInfo->runningUnackedIndex = startingIndex;
                        }
                        else if(header.magicNumber == STARTNUMBER)
                        {
                            if(!network->nextConnectionIndex || header.connectionSlot < network->nextConnectionIndex)
                            {
                                NetworkConnection* connection = network->connections + header.connectionSlot;
                                
                                if(connection->salt)
                                {
                                    u16 dataSize = header.totalPacketSize - sizeof(PacketHeader) - sizeof(PacketTrailer);
                                    DispatchToApplicationBuffer(network, connection, start, dataSize, header.channelIndex, header.progressiveIndex);
                                }
                            }
                            else
                            {
                                InvalidCodePath;
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
                                    connection->connected = false;
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

NETWORK_RECYCLE_CONNECTION(Win32RecycleConnection)
{
    NetworkConnection* connection = network->connections + connectionSlot;
    connection->nextFree = network->firstFreeConnection;
    network->firstFreeConnection = connection;
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
    
    for(u32 channelIndex = 0; channelIndex < channelCount; ++channelIndex)
    {
        NetworkChannelParams* params = channelParams + channelIndex;
        NetworkAddChannel_(network, params);
    }
    
    network->connections = connections;
    network->maxConnectionCount = maxConnectionCount;
    network->nextConnectionIndex = 1;
}


NetworkAPI Win32NetworkAPI =
{
    Win32SendData,
    Win32ReceiveDataAccept,
    Win32GetPacket,
    Win32OpenConnection,
    Win32CloseConnection,
    Win32InitServer,
    Win32RecycleConnection,
};
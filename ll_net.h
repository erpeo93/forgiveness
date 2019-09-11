#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

inline u64 pack754(long double f, unsigned bits, unsigned expbits)
{
    long double fnorm;
    int shift;
    long long sign, exp, significand;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit
    
    if (f == 0.0) return 0; // get this special case out of the way
    
    // check sign and begin normalization
    if (f < 0) { sign = 1; fnorm = -f; }
    else { sign = 0; fnorm = f; }
    
    // get the normalized form of f and track the exponent
    shift = 0;
    while(fnorm >= 2.0) { fnorm /= 2.0; shift++; }
    while(fnorm < 1.0) { fnorm *= 2.0; shift--; }
    fnorm = fnorm - 1.0;
    
    // calculate the binary form (non-float) of the significand data
    significand = ( __int64 ) ( fnorm * ((1LL<<significandbits) + 0.5f) );
    
    // get the biased exponent
    exp = shift + ((1<<(expbits-1)) - 1); // shift + bias
    
    // return the final answer
    return (sign<<(bits-1)) | (exp<<(bits-expbits-1)) | significand;
}

inline long double unpack754(u64 i, unsigned bits, unsigned expbits)
{
    long double result;
    long long shift;
    unsigned bias;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit
    
    if (i == 0) return 0.0;
    
    // pull the significand
    result = ( long double ) (i&((1LL<<significandbits)-1)); // mask
    result /= (1LL<<significandbits); // convert back to float
    result += 1.0f; // add the one back on
    
    // deal with the exponent
    bias = (1<<(expbits-1)) - 1;
    shift = ((i>>significandbits)&((1LL<<expbits)-1)) - bias;
    while(shift > 0) { result *= 2.0; shift--; }
    while(shift < 0) { result /= 2.0; shift++; }
    
    // sign it
    result *= (i>>(bits-1))&1? -1.0: 1.0;
    
    return result;
}

/*
** packi16() -- store a 16-bit int into a char buffer (like htons())
*/ 
inline void packi16(unsigned char *buf, unsigned int i)
{
    *buf++ = ( unsigned char ) ( i>>8 ); *buf++ = ( unsigned char ) i;
}

/*
** packi32() -- store a 32-bit int into a char buffer (like htonl())
*/ 
inline void packi32(unsigned char *buf, unsigned long int i)
{
    *buf++ = ( unsigned char ) ( i>>24 ); *buf++ = ( unsigned char ) ( i>>16 );
    *buf++ = ( unsigned char ) ( i>>8 ); *buf++ = ( unsigned char ) ( i );
}

/*
** packi64() -- store a 64-bit int into a char buffer (like htonl())
*/ 
inline void packi64(unsigned char *buf, unsigned long long int i)
{
    *buf++ = ( unsigned char ) ( i>>56 ); *buf++ = ( unsigned char ) ( i>>48 );
    *buf++ = ( unsigned char ) ( i>>40 ); *buf++ = ( unsigned char ) ( i>>32 );
    *buf++ = ( unsigned char ) ( i>>24 ); *buf++ = ( unsigned char ) ( i>>16 );
    *buf++ = ( unsigned char ) ( i>>8 );  *buf++ = ( unsigned char ) ( i );
}

/*
** unpacki16() -- unpack a 16-bit int from a char buffer (like ntohs())
*/ 
inline int unpacki16(unsigned char *buf)
{
    unsigned int i2 = ((unsigned int)buf[0]<<8) | buf[1];
    int i;
    
    // change unsigned numbers to signed
    if (i2 <= 0x7fffu) { i = i2; }
    else { i = -1 - (unsigned int)(0xffffu - i2); }
    
    return i;
}

/*
** unpacku16() -- unpack a 16-bit unsigned from a char buffer (like ntohs())
*/ 
inline unsigned int unpacku16(unsigned char *buf)
{
    return ((unsigned int)buf[0]<<8) | buf[1];
}

/*
** unpacki32() -- unpack a 32-bit int from a char buffer (like ntohl())
*/ 
inline long int unpacki32(unsigned char *buf)
{
    unsigned long int i2 = ((unsigned long int)buf[0]<<24) |
        ((unsigned long int)buf[1]<<16) |
        ((unsigned long int)buf[2]<<8)  |
        buf[3];
    long int i;
    
    // change unsigned numbers to signed
    if (i2 <= 0x7fffffffu) { i = i2; }
    else { i = -1 - (long int)(0xffffffffu - i2); }
    
    return i;
}

/*
** unpacku32() -- unpack a 32-bit unsigned from a char buffer (like ntohl())
*/ 
inline unsigned long int unpacku32(unsigned char *buf)
{
    return ((unsigned long int)buf[0]<<24) |
        ((unsigned long int)buf[1]<<16) |
        ((unsigned long int)buf[2]<<8)  |
        buf[3];
}

/*
** unpacki64() -- unpack a 64-bit int from a char buffer (like ntohl())
*/ 
inline long long int unpacki64(unsigned char *buf)
{
    unsigned long long int i2 = ((unsigned long long int)buf[0]<<56) |
        ((unsigned long long int)buf[1]<<48) |
        ((unsigned long long int)buf[2]<<40) |
        ((unsigned long long int)buf[3]<<32) |
        ((unsigned long long int)buf[4]<<24) |
        ((unsigned long long int)buf[5]<<16) |
        ((unsigned long long int)buf[6]<<8)  |
        buf[7];
    long long int i;
    
    // change unsigned numbers to signed
    if (i2 <= 0x7fffffffffffffffu) { i = i2; }
    else { i = -1 -(long long int)(0xffffffffffffffffu - i2); }
    
    return i;
}

/*
** unpacku64() -- unpack a 64-bit unsigned from a char buffer (like ntohl())
*/ 
inline unsigned long long int unpacku64(unsigned char *buf)
{
    return ((unsigned long long int)buf[0]<<56) |
        ((unsigned long long int)buf[1]<<48) |
        ((unsigned long long int)buf[2]<<40) |
        ((unsigned long long int)buf[3]<<32) |
        ((unsigned long long int)buf[4]<<24) |
        ((unsigned long long int)buf[5]<<16) |
        ((unsigned long long int)buf[6]<<8)  |
        buf[7];
}

/*
** pack() -- store data dictated by the format string in the buffer
**
**   bits |signed   unsigned   float   string
**   -----+----------------------------------
**      8 |   c        C         
**     16 |   h        H         f
**     32 |   l        L         d
**     64 |   q        Q         g
**      - |                               s
**
**  (16-bit unsigned length is automatically prepended to strings)
*/ 

inline unsigned int pack(unsigned char *buf, char *format, ...)
{
    va_list ap;
    
    signed char c;              // 8-bit
    unsigned char C;
    
    int h;                      // 16-bit
    unsigned int H;
    
    long int l;                 // 32-bit
    unsigned long int L;
    
    long long int q;            // 64-bit
    unsigned long long int Q;
    
    double d;
    long double g;
    unsigned long long int fhold;
    
    Vec2 v;
    Vec3 V;
    
    char *s;                    // strings
    unsigned int len;
    
    unsigned int size = 0;
    
    va_start(ap, format);
    
    for(; *format != '\0'; format++) {
        switch(*format) {
            case 'c': // 8-bit
            size += 1;
            c = (signed char)va_arg(ap, int); // promoted
            *buf++ = c;
            break;
            
            case 'C': // 8-bit unsigned
            size += 1;
            C = (unsigned char)va_arg(ap, unsigned int); // promoted
            *buf++ = C;
            break;
            
            case 'h': // 16-bit
            size += 2;
            h = va_arg(ap, int);
            packi16(buf, h);
            buf += 2;
            break;
            
            case 'H': // 16-bit unsigned
            size += 2;
            H = va_arg(ap, unsigned int);
            packi16(buf, H);
            buf += 2;
            break;
            
            case 'l': // 32-bit
            size += 4;
            l = va_arg(ap, long int);
            packi32(buf, l);
            buf += 4;
            break;
            
            case 'L': // 32-bit unsigned
            size += 4;
            L = va_arg(ap, unsigned long int);
            packi32(buf, L);
            buf += 4;
            break;
            
            case 'q': // 64-bit
            size += 8;
            q = va_arg(ap, long long int);
            packi64(buf, q);
            buf += 8;
            break;
            
            case 'Q': // 64-bit unsigned
            size += 8;
            Q = va_arg(ap, unsigned long long int);
            packi64(buf, Q);
            buf += 8;
            break;
            
            case 'd': // float-32
            size += 4;
            d = va_arg(ap, double);
            fhold = pack754_32(d); // convert to IEEE 754
            packi32(buf, ( unsigned long ) fhold);
            buf += 4;
            break;
            
            case 'v': // vec2
            size += 8;
            v = va_arg(ap, Vec2);
            fhold = pack754_32(v.x); // convert to IEEE 754
            packi32(buf, ( unsigned long ) fhold);
            buf += 4;
            fhold = pack754_32(v.y); // convert to IEEE 754
            packi32(buf, ( unsigned long ) fhold);
            buf += 4;
            break;
            
            case 'V': // vec3
            size += 12;
            V = va_arg(ap, Vec3);
            fhold = pack754_32(V.x); // convert to IEEE 754
            packi32(buf, ( unsigned long ) fhold);
            buf += 4;
            fhold = pack754_32(V.y); // convert to IEEE 754
            packi32(buf, ( unsigned long ) fhold);
            buf += 4;
            fhold = pack754_32(V.z); // convert to IEEE 754
            packi32(buf, ( unsigned long ) fhold);
            buf += 4;
            break;
            
            case 'g': // float-64
            size += 8;
            g = va_arg(ap, long double);
            fhold = pack754_64(g); // convert to IEEE 754
            packi64(buf, fhold);
            buf += 8;
            break;
            
            case 's': // string
            s = va_arg(ap, char*);
            len = (int) strlen(s);
            size += len + 2;
            packi16(buf, len);
            buf += 2;
            memcpy(buf, s, len);
            buf += len;
            break;
        }
    }
    
    va_end(ap);
    
    return size;
}

/*
** unpack() -- unpack data dictated by the format string into the buffer
**
**   bits |signed   unsigned   float   string
**   -----+----------------------------------
**      8 |   c        C         
**     16 |   h        H         f
**     32 |   l        L         d
**     64 |   q        Q         g
**      - |                               s
**
**  (string is extracted based on its stored length, but 's' can be
**  prepended with a max length)
*/
inline unsigned char* unpack(unsigned char *buf, char *format, ...)
{
    va_list ap;
    
    signed char *c;              // 8-bit
    unsigned char *C;
    
    i16 *h;                      // 16-bit
    u16 *H;
    
    i32 *l;                 // 32-bit
    u32 *L;
    
    i64 *q;            // 64-bit
    u64*Q;
    
    float *d;
    double *g;
    unsigned long long int fhold;
    
    Vec2* v;
    Vec3* V;
    
    char *s;
    unsigned int len, maxstrlen=0, count;
    
    va_start(ap, format);
    
    for(; *format != '\0'; format++) {
        switch(*format) {
            case 'c': // 8-bit
            c = va_arg(ap, signed char*);
            if (*buf <= 0x7f) { *c = *buf;} // re-sign
            else { *c = -1 - (unsigned char)(0xffu - *buf); }
            buf++;
            break;
            
            case 'C': // 8-bit unsigned
            C = va_arg(ap, unsigned char*);
            *C = *buf++;
            break;
            
            case 'h': // 16-bit
            h = va_arg(ap, i16*);
            *h = ( i16 ) unpacki16(buf);
            buf += 2;
            break;
            
            case 'H': // 16-bit unsigned
            H = va_arg(ap, u16*);
            *H = ( u16 ) unpacku16(buf);
            buf += 2;
            break;
            
            case 'l': // 32-bit
            l = va_arg(ap, i32*);
            *l = unpacki32(buf);
            buf += 4;
            break;
            
            case 'L': // 32-bit unsigned
            L = va_arg(ap, u32*);
            *L = unpacku32(buf);
            buf += 4;
            break;
            
            case 'q': // 64-bit
            q = va_arg(ap, i64*);
            *q = unpacki64(buf);
            buf += 8;
            break;
            
            case 'Q': // 64-bit unsigned
            Q = va_arg(ap, u64*);
            *Q = unpacku64(buf);
            buf += 8;
            break;
            
            case 'd': // float-32
            d = va_arg(ap, float*);
            fhold = unpacku32(buf);
            *d = ( r32 ) unpack754_32(fhold);
            buf += 4;
            break;
            
            case 'v': // float-32
            v= va_arg(ap, Vec2*);
            fhold = unpacku32(buf);
            v->x = ( r32 ) unpack754_32(fhold);
            buf += 4;
            fhold = unpacku32(buf);
            v->y = ( r32 ) unpack754_32(fhold);
            buf += 4;
            break;
            
            case 'V': // float-32
            V= va_arg(ap, Vec3*);
            fhold = unpacku32(buf);
            V->x = ( r32 ) unpack754_32(fhold);
            buf += 4;
            fhold = unpacku32(buf);
            V->y = ( r32 ) unpack754_32(fhold);
            buf += 4;
            fhold = unpacku32(buf);
            V->z = ( r32 ) unpack754_32(fhold);
            buf += 4;
            break;
            
            
            
            
            
            case 'g': // float-64
            g = va_arg(ap, double*);
            fhold = unpacku64(buf);
            *g = ( r64 ) unpack754_64(fhold);
            buf += 8;
            break;
            
            case 's': // string
            s = va_arg(ap, char*);
            len = unpacku16(buf);
            buf += 2;
            if (maxstrlen > 0 && len > maxstrlen) count = maxstrlen - 1;
            else count = len;
            memcpy(s, buf, count);
            s[count] = '\0';
            buf += len;
            break;
            
            default:
            if (isdigit(*format)) { // track max str len
                maxstrlen = maxstrlen * 10 + (*format-'0');
            }
        }
        
        if (!isdigit(*format)) maxstrlen = 0;
    }
    
    va_end(ap);
    return buf;
}


#define STARTNUMBER 123456789
#define HELLONUMBER 1234554321
#define ACKNUMBER 12332123
#define DISCONNECTNUMBER 999888777
#pragma pack(push, 1)
struct AckedBits
{
    u32 bits[2];
};

struct PacketHeader
{
    u16 dataSize;
    i32 magicNumber;
    u16 connectionSlot;
    u16 salt;
    u8 flags;
    u16 progressiveIndex;
    
    u16 acked;
    AckedBits ackedBits;
};
#pragma pack(pop)


#define GUARANTEED_DELIVERY_STARTING_BIT 2
enum GuaranteedDelivery
{
    GuaranteedDelivery_None = 0,
    GuaranteedDelivery_Guaranteed = 1,
    GuaranteedDelivery_Ordered = 2,
    
    GuaranteedDelivery_Count,
};

#define ACK_BITS_NUMBER_STARTING_BIT 4
enum NetworkAckBitNumber
{
    NetworkBits_32,
    NetworkBits_64,
    NetworkBits_96,
    NetworkBits_128,
};

struct NetworkSendParams
{
    GuaranteedDelivery guaranteedDelivery;
};

inline u32 Get2BitFlags(u8 flags, u8 startingBit)
{
    Assert(startingBit <= 6);
    u32 result = (flags >> startingBit) & 3;
    return result;
}

inline u8 Set2BitsFlags(u8 flags, u8 startingBit, u8 value)
{
    Assert(startingBit <= 6);
    Assert(value < 4);
    
    u8 result = flags | (value << startingBit);
    
    return result;
}

inline u32 GetBitCount(u8 flags)
{
    u32 result = Get2BitFlags(flags, ACK_BITS_NUMBER_STARTING_BIT) + 1;
    Assert(result && result <= 2);
    return result;
}

inline unsigned char* PackHeader_(unsigned char* buff, i32 magicNumber, u16 connectionSlot, u16 salt, u8 flags, u16 progressiveIndex, u16 acked, AckedBits ackedBits)
{
    u16 dataSize = 0;
    unsigned char* result = buff;
    result += pack(result, "HlHHCHH", dataSize, (i32) magicNumber, connectionSlot, salt, flags, progressiveIndex, acked);
    
    
    u32 bitCount = GetBitCount(flags);
    for(u32 bitIndex = 0; bitIndex < bitCount; ++bitIndex)
    {
        result += pack(result, "L", ackedBits.bits[bitIndex]);
    }
    
    return result;
}

inline unsigned char* UnpackHeader_(unsigned char* buff, PacketHeader* header)
{
    buff = unpack(buff, "HlHHCHH", &header->dataSize, &header->magicNumber, &header->connectionSlot, &header->salt, &header->flags, &header->progressiveIndex, &header->acked);
    
    
    u32 bitCount = GetBitCount(header->flags);
    for(u32 bitIndex = 0; bitIndex < bitCount; ++bitIndex)
    {
        buff = unpack(buff, "L", &header->ackedBits.bits[bitIndex]);
    }
    
    return buff;
}

inline u16 PackTrailer_(unsigned char* original, unsigned char* current, u8 headerFlags)
{
    u16 totalSize = (u16) (current - original);
    
    u32 bitCount = GetBitCount(headerFlags);
    u32 headerSize = sizeof(PacketHeader) -sizeof(AckedBits) + (sizeof(u32) * bitCount);
    
    u16 dataSize = (u16) (totalSize - headerSize);
    pack(original, "H", dataSize);
    return totalSize;
}

#define PACKET_SIZE 1024 + 128
struct NetworkPacketReceived
{
    b32 disconnected;
    u16 dataSize;
    unsigned char data[PACKET_SIZE];
};

struct PendingConnection
{
    u32 challenge;
};


struct NetworkBufferedPacket
{
    u16 progressiveIndex;
    u8 flags;
    
    u16 dataSize;
    u8 data[PACKET_SIZE];
    
    r32 inFlightTimer;
    union
    {
        NetworkBufferedPacket* next;
        NetworkBufferedPacket* nextFree;
    };
    NetworkBufferedPacket* prev;
};

struct NetworkAck
{
    u16 progressiveIndex;
    AckedBits bits;
};

struct NetworkBufferedAck
{
    NetworkAck ack;
    union
    {
        NetworkBufferedAck* next;
        NetworkBufferedAck* nextFree;
    };
};


struct PacketData
{
	u32 packetIndex;
};


struct NetworkConnection
{
    u16 salt;
    
    u16 counterpartConnectionSlot;
    u8 counterpartAddress[64];
    int counterpartAddrSize;
    
    u16 nextProgressiveIndex;
    NetworkBufferedPacket toSendQueueSentinel;
    NetworkBufferedPacket sentQueueSentinel;
    NetworkBufferedPacket recvQueueSentinel;
    NetworkBufferedPacket* firstFreePacket;
    
    TicketMutex mutex;
    NetworkBufferedAck* firstQueuedAck;
    NetworkBufferedAck* firstFreeAck;
    
    NetworkAck ackToInclude;
    PacketData receivedPackets[1024];
    
    
    NetworkConnection* nextFree;
    
    
    u32 receivedPacketsNoSend;
    u32 receivedPacketsLastTick;
    r32 receivingPacketPerSecond;
    
    r32 estimatedRTT;
    r32 sendAllowedBandwidth;
};

struct NetworkNewConnection
{
    b32 accepted;
    u16 slot;
};

struct NetworkInterface
{
    u32 totalBytesReceived;
    u32 fd;
    
    b32 newConnectionsAccepted;
    u16 newConnectionCount;
    NetworkNewConnection newConnections[128];
    
    TicketMutex connectionMutex;
    u16 nextConnectionIndex;
    NetworkConnection* firstFreeConnection;
    
    u16 maxConnectionCount;
    NetworkConnection* connections;
    NetworkConnection clientConnection;
    
    u8 recvBuffer[2048 + sizeof(PacketHeader)];
    u8 sendBuffer[2048 + sizeof(PacketHeader)];
};

#define NETWORK_QUEUE_PACKET(name) void name(NetworkInterface* network, u16 connectionSlot, NetworkSendParams params,void* data, u16 size)
typedef NETWORK_QUEUE_PACKET(network_platform_queue_packet);

#define NETWORK_FLUSH_SEND_QUEUE(name) void name(NetworkInterface* network, u16 connectionSlot, r32 elapsedTime)
typedef NETWORK_FLUSH_SEND_QUEUE(network_platform_flush_send_queue);

#define NETWORK_RECEIVE_DATA(name) void name(NetworkInterface* network)
typedef NETWORK_RECEIVE_DATA(network_platform_receive_data);

#define NETWORK_ACCEPT(name) u16 name(NetworkInterface* network, u16* acceptedSlots, u16 maxAccepted)
typedef NETWORK_ACCEPT(network_platform_accept);

#define NETWORK_GET_PACKET(name) NetworkPacketReceived name(NetworkInterface* network, u16 connectionSlot)
typedef NETWORK_GET_PACKET(network_platform_get_packet);

#define NETWORK_OPEN_CONNECTION(name) void name(NetworkInterface* network, char* IP, u16 port)
typedef NETWORK_OPEN_CONNECTION(network_platform_open_connection);

#define NETWORK_CLOSE_CONNECTION(name) void name(NetworkInterface* network, u16 connectionSlot)
typedef NETWORK_CLOSE_CONNECTION(network_platform_close_connection);

#define NETWORK_INIT_SERVER(name) void name(NetworkInterface* network, u16 port, NetworkConnection* connections, u16 maxConnectionCount)
typedef NETWORK_INIT_SERVER(network_platform_init_server);



struct NetworkAPI
{
    network_platform_queue_packet* QueuePacket;
    network_platform_receive_data* ReceiveData;
    network_platform_flush_send_queue* FlushSendQueue;
    network_platform_accept* Accept;
    network_platform_get_packet* GetPacketOnSlot;
    
    network_platform_open_connection* OpenConnection;
    network_platform_close_connection* CloseConnection;
    network_platform_init_server* InitServer;
};


#ifdef LL_NET_IMPLEMENTATION
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
            
			int iSetOption = 1;
			setsockopt(result, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
            
            
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

inline b32 IsSet(AckedBits ackedBits, u16 bitIndex)
{
    Assert(bitIndex < 128);
    
    u16 index = bitIndex / 32;
    u16 offset = index * 32;
    
    b32 result = (ackedBits.bits[index] & (1 << (bitIndex - offset)));
    return result;
}

inline AckedBits SetBitIndex(AckedBits bits, u16 bitIndex)
{
    Assert(bitIndex < 128);
    
    AckedBits result = bits;
    
    u16 index = bitIndex / 32;
    u16 offset = index * 32;
    
    result.bits[index] = result.bits[index] | (1 << (bitIndex - offset));
    
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


#define LOST_TIME 1.0f
#define MINIMUM_BANDWIDTH KiloBytes(32)
inline r32 NetLerp(r32 value1, r32 lerp, r32 value2)
{
    r32 result = ( 1.0f - lerp ) * value1 + lerp * value2;
    return result;
}

inline void AdjustRTT(NetworkConnection* connection, r32 packetRTT)
{
    Assert(packetRTT >= 0);
    if(!connection->estimatedRTT)
    {
        connection->estimatedRTT = 0.2f;
    }
    connection->estimatedRTT = NetLerp(connection->estimatedRTT, 0.2f, packetRTT);
}

inline void ReduceBandwidth(NetworkConnection* connection)
{
    connection->sendAllowedBandwidth /= 1.2f;
}

global_variable u32 packetLostCount;
inline void SignalPacketLost(NetworkConnection* connection, r32 time)
{
    printf("lost packet! %d\n", packetLostCount++);
	AdjustRTT(connection, time);
    ReduceBandwidth(connection);
}

inline u32 GetBandwidth(NetworkConnection* connection, r32 elapsedTime)
{
    connection->sendAllowedBandwidth = Max(connection->sendAllowedBandwidth, MINIMUM_BANDWIDTH);
    u32 result = (u32) (connection->sendAllowedBandwidth / elapsedTime);
    return result;
}

inline void DispatchQueuedAcks(NetworkConnection* connection)
{
    BeginTicketMutex(&connection->mutex);
    NetworkBufferedAck* firstToDispatch = connection->firstQueuedAck;
    connection->firstQueuedAck = 0;
    EndTicketMutex(&connection->mutex);
    
    if(firstToDispatch)
    {
        NetworkBufferedAck* lastDispatched = 0;
        for(NetworkBufferedAck* toDispatch = firstToDispatch; toDispatch; toDispatch = toDispatch->next)
        {
            for(NetworkBufferedPacket* packet = connection->sentQueueSentinel.next; packet != &connection->sentQueueSentinel;)
            {
                NetworkBufferedPacket* next = packet->next;
                if(Acked(toDispatch->ack, packet->progressiveIndex))
                {
                    AdjustRTT(connection, packet->inFlightTimer);
                    connection->sendAllowedBandwidth += (r32) KiloBytes(1);
                    DLLIST_REMOVE(packet);
                    
                    BeginTicketMutex(&connection->mutex);
                    FREELIST_DEALLOC(packet, connection->firstFreePacket);
                    EndTicketMutex(&connection->mutex);
                }
                
                packet = next;
            }
            
            lastDispatched = toDispatch;
        }
        
        BeginTicketMutex(&connection->mutex);
        lastDispatched->nextFree = connection->firstFreeAck;
        connection->firstFreeAck = firstToDispatch;
        EndTicketMutex(&connection->mutex);
    }
}

inline void QueueAck(NetworkConnection* connection, u16 ackedIndex, AckedBits bits)
{
    NetworkBufferedAck* ack;
    FREELIST_ALLOC(ack, connection->firstFreeAck, (NetworkBufferedAck*) malloc(sizeof(NetworkAck)));
    ack->ack.progressiveIndex = ackedIndex;
    ack->ack.bits = bits;
    FREELIST_INSERT(ack, connection->firstQueuedAck);
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



inline void SignalReceivedPacket(NetworkConnection* connection, u16 packetIndex)
{
    ++connection->receivedPacketsLastTick;
    NetworkAck newAckToInclude = connection->ackToInclude;
    if(packetIndex != newAckToInclude.progressiveIndex)
    {
        u16 bitsToInclude = sizeof(AckedBits) * 8;
        if(PacketIndexGreater(packetIndex, newAckToInclude.progressiveIndex))
        {
            u16 oldBiggest = newAckToInclude.progressiveIndex;
            u16 newBiggest = packetIndex;
            
            newAckToInclude.progressiveIndex = newBiggest;
            newAckToInclude.bits = {};
            
            
            u16 index = oldBiggest + 1;
            u16 destIndex = newBiggest;
            while(PacketIndexSmaller(index, destIndex))
            {
                Clear(connection, index++);
            }
            
            
            index = newBiggest - bitsToInclude;
            destIndex = newBiggest;
            
			u16 bitIndex = bitsToInclude - 1;
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
		
		BeginTicketMutex(&connection->mutex);
		connection->ackToInclude = newAckToInclude;
		EndTicketMutex(&connection->mutex);
    }
}

NETWORK_QUEUE_PACKET(Win32QueuePacket)
{
    NetworkConnection* connection = network->connections + connectionSlot;
    
    NetworkBufferedPacket* newPacket;
    BeginTicketMutex(&connection->mutex);
    FREELIST_ALLOC(newPacket, connection->firstFreePacket, (NetworkBufferedPacket*) malloc(sizeof(NetworkBufferedPacket)));
    EndTicketMutex(&connection->mutex);
    
    
    Assert(size && size <= ArrayC(newPacket->data));
    newPacket->dataSize = size;
    memcpy(newPacket->data, data, size);
    
    
    u8 flags = 0;
    flags = Set2BitsFlags(flags, GUARANTEED_DELIVERY_STARTING_BIT, (u8) params.guaranteedDelivery);
    
    u32 receivingSpeedLimit = 256;
    if(connection->receivingPacketPerSecond > receivingSpeedLimit)
    {
        flags = Set2BitsFlags(flags, ACK_BITS_NUMBER_STARTING_BIT, (u8) NetworkBits_64);
    }
    
    newPacket->flags = flags;
    newPacket->progressiveIndex = connection->nextProgressiveIndex++;
    DLLIST_INSERT_AS_LAST(&connection->toSendQueueSentinel, newPacket);
}

internal void UpdateSentPackets(NetworkConnection* connection, r32 elapsedTime)
{
    
    r32 targetTime = Max(0.1f, 2.0f * connection->estimatedRTT);
    NetworkBufferedPacket* packet = connection->sentQueueSentinel.next;
    
    while(packet != &connection->sentQueueSentinel)
    {
        NetworkBufferedPacket* next = packet->next;
        
        packet->inFlightTimer += elapsedTime;
        if(packet->inFlightTimer >= targetTime)
        {
            SignalPacketLost(connection, packet->inFlightTimer);
            
            GuaranteedDelivery timeType = (GuaranteedDelivery) Get2BitFlags(packet->flags, GUARANTEED_DELIVERY_STARTING_BIT);
            DLLIST_REMOVE(packet);
            if(timeType > GuaranteedDelivery_None)
            {
                packet->progressiveIndex = connection->nextProgressiveIndex++;
                DLLIST_INSERT_AS_LAST(&connection->toSendQueueSentinel, packet);
            }
            else
            {
                BeginTicketMutex(&connection->mutex);
                FREELIST_INSERT(packet, connection->firstFreePacket);
                EndTicketMutex(&connection->mutex);
            }
        }
        
        packet = next;
    }
}

NETWORK_FLUSH_SEND_QUEUE(Win32FlushSendQueue)
{
    elapsedTime = Min(elapsedTime, 0.2f);
    
    NetworkConnection* connection = network->connections + connectionSlot;
    if(connection)
    {
        DispatchQueuedAcks(connection);
        UpdateSentPackets(connection, elapsedTime);
        
        u32 dataToSend = GetBandwidth(connection, elapsedTime);        
        r32 newReceivingSpeed = connection->receivedPacketsLastTick / elapsedTime;
        connection->receivingPacketPerSecond = NetLerp(connection->receivingPacketPerSecond, 0.5f, newReceivingSpeed);
        WriteBarrier;
        connection->receivedPacketsLastTick = 0;
        connection->receivedPacketsNoSend = 0;
        
        
        NetworkBufferedPacket* packet = connection->toSendQueueSentinel.next;
        while(packet != &connection->toSendQueueSentinel && (dataToSend > 0))
        {
            NetworkBufferedPacket* next = packet->next;
            
            BeginTicketMutex(&connection->mutex);
            NetworkAck ackToInclude = connection->ackToInclude;
            EndTicketMutex(&connection->mutex);
            
            unsigned char buff_[2048];
            u32 totalSize = 0;
            unsigned char* buff = PackHeader_(buff_, STARTNUMBER, connection->counterpartConnectionSlot, connection->salt, packet->flags, packet->progressiveIndex, ackToInclude.progressiveIndex, ackToInclude.bits);
            memcpy(buff, packet->data, packet->dataSize);
            buff += packet->dataSize;
            totalSize = PackTrailer_(buff_, buff, packet->flags);
            
            if(sendto(network->fd, (const char*) buff_, totalSize, 0, (const sockaddr*) connection->counterpartAddress, connection->counterpartAddrSize) < 0)
            {
                printf("send buffer full!");
                ReduceBandwidth(connection);
                break;
            }
            
            if(totalSize >= dataToSend)
            {
                dataToSend = 0;
            }
            else
            {
                dataToSend -= totalSize;
            }
            
            DLLIST_REMOVE(packet);
            packet->inFlightTimer = 0;
            DLLIST_INSERT_AS_LAST(&connection->sentQueueSentinel, packet);
            packet = next;
        }
    }
}

inline void Win32FreeConnection(NetworkInterface* network, NetworkConnection* connection)
{
    connection->counterpartConnectionSlot = 0;
    connection->nextProgressiveIndex = 0;
    connection->ackToInclude = {};
    connection->ackToInclude.progressiveIndex = 0xffff;
    
    if(!DLLIST_ISEMPTY(&connection->toSendQueueSentinel))
    {
        connection->toSendQueueSentinel.prev ->next = connection->firstFreePacket;
        connection->firstFreePacket = connection->toSendQueueSentinel.next;
        DLLIST_INIT(&connection->toSendQueueSentinel);
    }
    
    if(!DLLIST_ISEMPTY(&connection->sentQueueSentinel))
    {
        connection->sentQueueSentinel.prev ->next = connection->firstFreePacket;
        connection->firstFreePacket = connection->sentQueueSentinel.next;
        DLLIST_INIT(&connection->sentQueueSentinel);
    }
    
    if(!DLLIST_ISEMPTY(&connection->recvQueueSentinel))
    {
        connection->recvQueueSentinel.prev ->next = connection->firstFreePacket;
        connection->firstFreePacket = connection->recvQueueSentinel.next;
        DLLIST_INIT(&connection->recvQueueSentinel);
    }
    
    FREELIST_FREE(connection->firstQueuedAck, NetworkBufferedAck, connection->firstFreeAck);
    
    Assert(DLLIST_ISEMPTY(&connection->toSendQueueSentinel));
    Assert(DLLIST_ISEMPTY(&connection->sentQueueSentinel));
    Assert(DLLIST_ISEMPTY(&connection->recvQueueSentinel));
    
    BeginTicketMutex(&network->connectionMutex);
    connection->nextFree = network->firstFreeConnection;
    network->firstFreeConnection = connection;
    EndTicketMutex(&network->connectionMutex);
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
        unsigned char* disconnectMsg = PackHeader_(disconnectMsg_, DISCONNECTNUMBER, connection->counterpartConnectionSlot, 0, 0, 0, 0, {});
        disconnectMsg += pack(disconnectMsg, "L", connection->salt);
        u32 totalSize = PackTrailer_(disconnectMsg_, disconnectMsg, 0);
        
        for(u32 i = 0; i < 10; ++i)
        {
            if(sendto(network->fd, (const char*) disconnectMsg_, totalSize, 0, (sockaddr*) connection->counterpartAddress, connection->counterpartAddrSize) < 0)
            {
                InvalidCodePath;
            }
        }
        
        BeginTicketMutex(&connection->mutex);
        connection->salt = 0;
        EndTicketMutex(&connection->mutex);
        
        Win32FreeConnection(network, connection);
    }
}

static u16 clientSalt = 1111;
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
    
    BeginTicketMutex(&connection->mutex);
    
    if(connection->toSendQueueSentinel.next)
    {
        if(!DLLIST_ISEMPTY(&connection->toSendQueueSentinel))
        {
            connection->toSendQueueSentinel.prev ->next = connection->firstFreePacket;
            connection->firstFreePacket = connection->toSendQueueSentinel.next;
        }
    }
    DLLIST_INIT(&connection->toSendQueueSentinel);
    
    if(connection->sentQueueSentinel.next)
    {
        if(!DLLIST_ISEMPTY(&connection->sentQueueSentinel))
        {
            connection->sentQueueSentinel.prev ->next = connection->firstFreePacket;
            connection->firstFreePacket = connection->sentQueueSentinel.next;
        }
    }
    DLLIST_INIT(&connection->sentQueueSentinel);
    
    if(connection->recvQueueSentinel.next)
    {
        if(!DLLIST_ISEMPTY(&connection->recvQueueSentinel))
        {
            connection->recvQueueSentinel.prev->next = connection->firstFreePacket;
            connection->firstFreePacket = connection->recvQueueSentinel.next;
        }
    }
    DLLIST_INIT(&connection->recvQueueSentinel);
    
    FREELIST_FREE(connection->firstQueuedAck, NetworkBufferedAck, connection->firstFreeAck);
    
    connection->ackToInclude = {};
    connection->ackToInclude.progressiveIndex = 0xffff;
    
    EndTicketMutex(&connection->mutex);
    
    while(true)
    {
        unsigned char helloMsg_[64];
        unsigned char* endHelloMsg = PackHeader_(helloMsg_, HELLONUMBER, 0, 0, 0, 0, 0, {});
        endHelloMsg += pack(endHelloMsg, "H", clientSalt);
        u32 totalSize = PackTrailer_(helloMsg_, endHelloMsg, 0);
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
            u16 salt;
            packet = unpack(packet, "H", &salt);
            
            if(header.magicNumber == HELLONUMBER && salt == clientSalt)
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
	BeginTicketMutex(&connection->mutex);
    NetworkBufferedPacket* firstToReceive = connection->recvQueueSentinel.next;
    
    result.disconnected = (connection->salt == 0);
    if(firstToReceive != &connection->recvQueueSentinel)
    {
        result.dataSize = firstToReceive->dataSize;
        memcpy(result.data, firstToReceive->data, firstToReceive->dataSize);
        
        DLLIST_REMOVE(firstToReceive);
        FREELIST_DEALLOC(firstToReceive, connection->firstFreePacket);
    }
    
	EndTicketMutex(&connection->mutex);
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
            if(bytesRead > sizeof(PacketHeader) - sizeof(AckedBits))
            {
                PacketHeader header = {};
                unsigned char* start = UnpackHeader_(network->recvBuffer, &header);
                if((header.dataSize + sizeof(PacketHeader)) <= sizeof(network->recvBuffer))
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
                                u16 salt;
                                unpack(start, "H", &salt);
                                saltToSendBack = salt;
                                
                                BeginTicketMutex(&network->connectionMutex);
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
                                EndTicketMutex(&network->connectionMutex);
                                
                                connection->salt = salt;
                                connection->sendAllowedBandwidth = KiloBytes(256);
                                
                                
                                connection->nextProgressiveIndex = 0;
                                DLLIST_INIT(&connection->toSendQueueSentinel);
                                DLLIST_INIT(&connection->sentQueueSentinel);
                                DLLIST_INIT(&connection->recvQueueSentinel);
                                
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
                            unsigned char* packet = PackHeader_(helloResponse, HELLONUMBER, connectionSlot, 0, 0, 0, 0, {});
                            packet += pack(packet, "H", saltToSendBack);
                            u32 totalSize = PackTrailer_(helloResponse, packet, 0);
                            
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
                            if(header.salt)
                            {
                                NetworkConnection* connection = network->connections + header.connectionSlot;
                                SignalReceivedPacket(connection, header.progressiveIndex);
                                
                                BeginTicketMutex(&connection->mutex);
                                if(connection->salt == header.salt)
                                {
                                    if(connection->receivedPacketsNoSend++ >= 32)
                                    {
                                        connection->receivedPacketsNoSend = 0;
                                        NetworkAck ackToInclude = connection->ackToInclude;
                                        unsigned char buff_[2048];
                                        unsigned char* buff = PackHeader_(buff_, ACKNUMBER, connection->counterpartConnectionSlot, connection->salt, 0, 0, ackToInclude.progressiveIndex, ackToInclude.bits);
                                        u32 totalSize = PackTrailer_(buff_, buff, 0);
                                        
                                        sendto(network->fd, (const char*) buff_, totalSize, 0, (const sockaddr*) connection->counterpartAddress, connection->counterpartAddrSize);
                                        
                                    }
                                    
                                    
                                    
                                    QueueAck(connection, header.acked, header.ackedBits);
                                    NetworkBufferedPacket* recv;
                                    FREELIST_ALLOC(recv, connection->firstFreePacket, (NetworkBufferedPacket*) malloc(sizeof(NetworkBufferedPacket)));
                                    recv->flags = header.flags;
                                    recv->progressiveIndex = header.progressiveIndex;
                                    recv->dataSize = header.dataSize;
                                    memcpy(recv->data, start, header.dataSize);
                                    
                                    DLLIST_INSERT_AS_LAST(&connection->recvQueueSentinel, recv);
                                }
                                EndTicketMutex(&connection->mutex);
                            }
                        }
                    }
                    else if(header.magicNumber == ACKNUMBER)
                    {
                        if(!network->nextConnectionIndex || header.connectionSlot < network->nextConnectionIndex)
                        {
                            if(header.salt)
                            {
                                NetworkConnection* connection = network->connections + header.connectionSlot;
                                QueueAck(connection, header.acked, header.ackedBits);
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
#endif
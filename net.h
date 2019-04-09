#pragma once
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float r32;
typedef double r64;

typedef i32 b32;

typedef size_t memory_index;
typedef uintptr_t unm;
typedef intptr_t snm;


#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

struct NetMutex
{
    u64 volatile ticket;
    u64 volatile serving;
};

inline u64 NetAtomicIncrementU64(u64 volatile* value, u64 addend)
{
    // NOTE( Leonardo ): return the value that was there _Before_ the add!
    u64 result = (InterlockedExchangeAdd64(( __int64* ) value, addend));
    return result;
}

inline void BeginNetMutex(NetMutex* mutex)
{
    u64 ticket = NetAtomicIncrementU64(&mutex->ticket, 1);
    while(ticket != mutex->serving);{_mm_pause();}
}

inline void EndNetMutex(NetMutex* mutex)
{
    NetAtomicIncrementU64(&mutex->serving, 1);
}

#define NETDLLIST_INSERT( sentinel, element ) \
(element)->next = (sentinel)->next; \
(element)->prev = (sentinel); \
(element)->prev->next = (element); \
(element)->next->prev = (element);

#define NETDLLIST_REMOVE(element) \
if((element)->next)\
{\
    (element)->prev->next = (element)->next; \
    (element)->next->prev = (element)->prev;\
    (element)->next = 0;\
    (element)->prev = 0;\
}

#define NETDLLIST_ISEMPTY(sentinel) ((sentinel)->next == (sentinel))

#define NETDLLIST_INSERT_AS_LAST( sentinel, element ) \
(element)->next = (sentinel); \
(element)->prev = (sentinel)->prev; \
(element)->prev->next = (element); \
(element)->next->prev = (element);


#define NETDLLIST_INIT( sentinel ) \
(sentinel)->next = ( sentinel );\
(sentinel)->prev = ( sentinel );

#define NETFREELIST_ALLOC( result, firstFreePtr, allocationCode ) if( ( result ) = ( firstFreePtr ) ) { ( firstFreePtr ) = ( result )->nextFree; } else{ ( result ) = allocationCode; } Assert( ( result ) != ( firstFreePtr ) ); 
#define NETFREELIST_DEALLOC( result, firstFreePtr ) Assert( ( result ) != ( firstFreePtr ) ); if( ( result ) ) { ( result )->nextFree = ( firstFreePtr ); ( firstFreePtr ) = ( result ); Assert( ( firstFreePtr )->next != ( result ) ); }
#define NETFREELIST_INSERT( newFirst, firstPtr ) Assert( ( firstPtr ) != ( newFirst ) ); ( newFirst )->next = ( firstPtr ); ( firstPtr ) = ( newFirst );

#define NETFREELIST_INSERT_AS_LAST(newLast, firstPtr, lastPtr) \
if(lastPtr){(lastPtr)->next = newLast; lastPtr = newLast;} else{ (firstPtr) = (lastPtr) = dest; }


#define NETFREELIST_FREE( listPtr, type, firstFreePtr ) for( type* element = ( listPtr ); element; ) { Assert( element != element->next ); type* nextElement = element->next; FREELIST_DEALLOC( element, firstFreePtr ); element = nextElement; } ( listPtr ) = 0;
#define NETFREELIST_COPY( destList, type, toCopy, firstFree, pool, ... ){ type* currentElement_ = ( toCopy ); while( currentElement_ ) { type* elementToCopy_; FREELIST_ALLOC( elementToCopy_, ( firstFree ), PushStruct( ( pool ), type ) ); ##__VA_ARGS__; *elementToCopy_ = *currentElement_; FREELIST_INSERT( elementToCopy_, ( destList ) ); currentElement_ = currentElement_->next; } }

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
#define ENDNUMBER 987654321
#define HELLONUMBER 1234554321
#define DISCONNECTNUMBER 999888777
#pragma pack(push, 1)
struct PacketHeader
{
    u16 dataSize;
    i32 magicNumber;
    u16 connectionSlot;
    u16 progressiveIndex;
    u8 flags;
    
    u16 acked;
    u32 ackedBits;
    ;
};

struct PacketTrailer
{
    i32 end;
};
#pragma pack(pop)

inline unsigned char* PackHeader_(unsigned char* buff, i32 magicNumber, u16 connectionSlot, u8 flags, u16 progressiveIndex)
{
    u16 dataSize = 0;
    unsigned char* result = buff;
    result += pack(result, "HlHCH", dataSize, (i32) magicNumber, connectionSlot, flags, progressiveIndex);
    
    return result;
}

inline unsigned char* UnpackHeader_(unsigned char* buff, PacketHeader* header)
{
    buff = unpack(buff, "HlHCH", &header->dataSize, &header->magicNumber, &header->connectionSlot, &header->flags, &header->progressiveIndex);
    return buff;
}

inline u16 PackTrailer_(unsigned char* original, unsigned char* current)
{
    current += pack(current, "l", (i32) ENDNUMBER);
    u16 totalSize = (u16) (current - original - sizeof(PacketHeader) - sizeof(PacketTrailer));
    pack(original, "H", totalSize);
    return totalSize;
}

struct NetworkPacketReceived
{
    b32 disconnected;
    u32 dataSize;
    unsigned char data[2048];
};

struct PendingConnection
{
    u32 challenge;
};


struct NetworkBufferedPacket
{
    u16 progressiveIndex;
    u32 dataSize;
    u8 data[2048];
    u8 flags;
    
    r32 timeInFlight;
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
    u32 bits;
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

struct NetworkConnection
{
    b32 connected;
    
    u32 salt;
    u16 counterpartConnectionSlot;
    u8 counterpartAddress[64];
    int counterpartAddrSize;
    
    
    u16 nextProgressiveIndex;
    NetworkBufferedPacket* firstNotSent;
    NetworkBufferedPacket sendQueueSentinel;
    NetworkBufferedPacket recvQueueSentinel;
    NetworkBufferedPacket* firstFreePacket;
    
    
    NetMutex mutex;
    NetworkBufferedAck* firstQueuedAck;
    NetworkBufferedAck* firstFreeAck;
    
    
    NetworkAck ackToInclude;
    //HashTable lastReceivedPackets;
    
    
    NetworkConnection* nextFree;
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
    
    u16 nextConnectionIndex;
    NetworkConnection* firstFreeConnection;
    
    u16 maxConnectionCount;
    NetworkConnection* connections;
    NetworkConnection clientConnection;
    
    u8 recvBuffer[2048 + sizeof(PacketHeader) + sizeof(PacketTrailer)];
    u8 sendBuffer[2048 + sizeof(PacketHeader) + sizeof(PacketTrailer)];
};

struct NetworkChannelParams
{
    b32 ordered;
};

enum NetworkFlags
{
    NetworkFlags_GuaranteedDelivery = (1 << 1),
};

#define NETWORK_QUEUE_PACKET(name) void name(NetworkInterface* network, u16 connectionSlot, u8 flags,void* data, u16 size)
typedef NETWORK_QUEUE_PACKET(network_platform_queue_packet);

#define NETWORK_FLUSH_SEND_QUEUE(name) void name(NetworkInterface* network, u16 connectionSlot, r32 elapsedTime)
typedef NETWORK_FLUSH_SEND_QUEUE(network_platform_flush_send_queue);

#define NETWORK_RECEIVE_DATA(name) void name(NetworkInterface* network)
typedef NETWORK_RECEIVE_DATA(network_platform_receive_data);

#define NETWORK_ACCEPT(name) u16 name(NetworkInterface* network, u16* acceptedSlots, u16 maxAccepted)
typedef NETWORK_ACCEPT(network_platform_accept);

#define NETWORK_GET_PACKET(name) NetworkPacketReceived name(NetworkInterface* network, u16 connectionSlot)
typedef NETWORK_GET_PACKET(network_platform_get_packet);

#define NETWORK_OPEN_CONNECTION(name) void name(NetworkInterface* network, char* IP, u16 port, u32 salt, u8 channelCount, NetworkChannelParams* channelParams)
typedef NETWORK_OPEN_CONNECTION(network_platform_open_connection);

#define NETWORK_CLOSE_CONNECTION(name) void name(NetworkInterface* network, u16 connectionSlot)
typedef NETWORK_CLOSE_CONNECTION(network_platform_close_connection);

#define NETWORK_INIT_SERVER(name) void name(NetworkInterface* network, u16 port, u8 channelCount, NetworkChannelParams* channelParams, NetworkConnection* connections, u16 maxConnectionCount)
typedef NETWORK_INIT_SERVER(network_platform_init_server);


#define NETWORK_RECYCLE_CONNECTION(name) void name(NetworkInterface* network, u16 connectionSlot)
typedef NETWORK_RECYCLE_CONNECTION(network_platform_recycle_connection);


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
    
    network_platform_recycle_connection* RecycleConnection;
};
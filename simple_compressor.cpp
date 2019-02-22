#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define Assert( expression ) if( !( expression ) ) { *( ( int* ) 0 ) = 0; }
#define ArrayCount( value ) ( sizeof( value ) / sizeof( ( value )[0] ) )

typedef char unsigned u8;

struct FileContent
{
    size_t fileSize;
    u8* content;
};

static FileContent ReadEntireFileAndNullTerminate( char* filename )
{
    FileContent result = {};
    FILE* file = fopen( filename, "rb" );
    if( file )
    {
        fseek( file, 0, SEEK_END );
        result.fileSize = ftell( file );
        fseek( file, 0, SEEK_SET );
        
        result.content = ( u8* ) malloc( result.fileSize );
        fread( result.content, result.fileSize, 1, file );
        
        fclose( file );
    }
    else
    {
        fprintf( stderr, "Unable to read file %s \n", filename );
    }
    
    return result;
}

static void Copy( size_t size, u8* in, u8* out )
{
    while( size-- )
    {
        *out++ = *in++;
    }
}

static size_t RLECompress( size_t inSize, u8* in, size_t maxOutSize, u8* outBase )
{
    u8* out = outBase;
    
#define MAX_LITERAL_COUNT 255
#define MAX_RUN_COUNT 255
    int literalCount = 0;
    u8 literals[MAX_LITERAL_COUNT];
    
    u8* inEnd = in + inSize;
    
    while( in < inEnd )
    {
        u8 startingValue = in[0];
        size_t run = 1;
        while( run < ( size_t ) ( inEnd - in ) && ( run < MAX_RUN_COUNT ) && ( in[run] == startingValue ) )
        {
            ++run;
        }
        
        if( run > 1 || ( literalCount == MAX_LITERAL_COUNT ) )
        {
            // NOTE(Leonardo): encode run
            u8 literalCount8 = ( u8 ) literalCount;
            Assert( literalCount8 == literalCount );
            
            *out++ = literalCount8;
            for( int literalIndex = 0; literalIndex < literalCount; ++literalIndex )
            {
                *out++ = literals[literalIndex];
            }
            
            literalCount = 0;
            
            u8 run8 = ( u8 ) run;
            Assert( run8 == run );
            *out++ = run8;
            *out++ = startingValue;
            in += run;
        }
        else
        {
            // NOTE(Leonardo): encode literals
            literals[literalCount++] = startingValue;
            ++in;
        }
    }
#undef MAX_LITERAL_COUNT
#undef MAX_RUN_COUNT
    
    size_t outSize = ( size_t )( out - outBase );
    Assert( outSize < maxOutSize );
    return outSize;
}

static void RLEDecompress( size_t inSize, u8* in, size_t outSize, u8* out )
{
    u8* inEnd = in + inSize;
    while( in < inEnd )
    {
        int literalCount = *in++;
        while( literalCount-- )
        {
            *out++ = *in++;
        }
        
        int repCount = *in++;
        u8 repValue = *in++;
        
        while( repCount-- )
        {
            *out++ = repValue;
        }
    }
    
    Assert( in == inEnd );
}

static size_t LZCompress( size_t inSize, u8* inBase, size_t maxOutSize, u8* outBase )
{
    u8* out = outBase;
    u8* in = inBase;
    
#define MAX_LITERAL_COUNT 255
#define MAX_RUN_COUNT 255
#define MAX_LOOKBACK_COUNT 255
    int literalCount = 0;
    u8 literals[MAX_LITERAL_COUNT];
    
    u8* inEnd = in + inSize;
    
    while( in < inEnd )
    {
        if( (in - inBase) == 3361 )
        {
            int a = 5;
        }
        size_t maxLookback = in - inBase;
        if( maxLookback > MAX_LOOKBACK_COUNT )
        {
            maxLookback = MAX_LOOKBACK_COUNT;
        }
        
        size_t bestRun = 0;
        size_t bestDistance = 0;
        for( u8* windowStart = in - maxLookback;
            windowStart < in;
            ++windowStart )
        {
            size_t windowSize = inEnd - windowStart;
            if( windowSize > MAX_RUN_COUNT )
            {
                windowSize = MAX_RUN_COUNT;
            }
            
            u8* windowEnd = windowStart + windowSize;
            u8* testIn = in;
            u8* windowIn = windowStart;
            size_t testRun = 0;
            
            while( ( windowIn < windowEnd ) && ( *testIn++  == *windowIn++ ) )
            {
                ++testRun;
            }
            
            if( bestRun < testRun )
            {
                bestRun = testRun;
                bestDistance = in - windowStart;
            }
            
        }
        
        // NOTE(Leonardo): think about when to output
        
        bool outputRun = false;
        if( literalCount )
        {
            outputRun = ( bestRun > 4 );
        }
        else
        {
            outputRun = ( bestRun > 2 );
        }
        
        if( outputRun || ( literalCount == MAX_LITERAL_COUNT ) )
        {
            // NOTE(Leonardo): flush
            u8 literalCount8 = ( u8 ) literalCount;
            Assert( literalCount8 == literalCount );
            
            if( literalCount8 )
            {
                *out++ = literalCount8;
                *out++ = 0;
                for( int literalIndex = 0; literalIndex < literalCount; ++literalIndex )
                {
                    *out++ = literals[literalIndex];
                }
                
                literalCount = 0;
            }
            
            if( outputRun )
            {
                u8 run8 = ( u8 ) bestRun;
                Assert( run8 == bestRun );
                *out++ = run8;
                
                u8 distance8 = ( u8 ) bestDistance;
                Assert( distance8 == bestDistance );
                *out++ = distance8;
                
                in += bestRun;
            }
        }
        else
        {
            // NOTE(Leonardo): buffer literals
            literals[literalCount++] = *in++;
        }
    }
#undef MAX_LITERAL_COUNT
#undef MAX_RUN_COUNT
#undef MAX_LOOKBACK_COUNT
    
    size_t outSize = ( size_t )( out - outBase );
    Assert( outSize < maxOutSize );
    return outSize;
}

static void LZDecompress( size_t inSize, u8* in, size_t outSize, u8* out )
{
    u8* inEnd = in + inSize;
    while( in < inEnd )
    {
        int count = *in++;
        u8 copyDistance = *in++;
        
        u8* source = (out - copyDistance );
        if( copyDistance == 0 )
        {
            source = in;
            in += count;
        }
        
        while( count-- )
        {
            *out++ = *source++;
        }
    }
    
    Assert( in == inEnd );
}


static size_t Compress( size_t inSize, u8* in, size_t maxOutSize, u8* out )
{
    size_t outSize = LZCompress( inSize, in, maxOutSize, out );
    return outSize;
}

static void Decompress( size_t inSize, u8* in, size_t maxOutSize, u8* out )
{
    LZDecompress( inSize, in, maxOutSize, out );
}

static size_t GetMaximumCompressedOutputSize( size_t inSize )
{
    size_t result = 256 + 2 * inSize;
    return result;
}

int main( int argc, char** argv )
{
    if( argc == 4 )
    {
        size_t finalOutputSize = 0;
        u8* finalOutputBuffer = 0;
        
        char* command = argv[1];
        char* inFilename = argv[2];
        char* outFilename = argv[3];
        
        FileContent inFile = ReadEntireFileAndNullTerminate( inFilename );
        if( strcmp( command, "compress" ) == 0 )
        {
            size_t outBufferSize = GetMaximumCompressedOutputSize( inFile.fileSize );
            u8* outBuffer = ( u8* ) malloc( outBufferSize + 4 );
            
            size_t compressedSize = Compress( inFile.fileSize, inFile.content, outBufferSize, outBuffer + 4);
            *( ( size_t* ) outBuffer ) = inFile.fileSize;
            
            finalOutputSize = compressedSize + 4;
            finalOutputBuffer = outBuffer;
        }
        else if( strcmp( command, "decompress" ) == 0 )
        {
            if( inFile.fileSize >= 4 )
            {
                size_t outBufferSize = *( int unsigned* ) inFile.content;
                u8* outBuffer = ( u8* ) malloc( outBufferSize );
                Decompress( inFile.fileSize - 4, inFile.content + 4, outBufferSize, outBuffer + 4);
                
                finalOutputSize = outBufferSize;
                finalOutputBuffer = outBuffer;
            }
            else
            {
                fprintf( stderr, "Invalid input file\n" );
            }
        }
        else if( strcmp( command, "test" ) == 0 )
        {
            size_t outBufferSize = GetMaximumCompressedOutputSize( inFile.fileSize );
            u8* outBuffer = ( u8* ) malloc( outBufferSize );
            u8* testBuffer = ( u8* ) malloc( inFile.fileSize );
            
            size_t compressedSize = Compress( inFile.fileSize, inFile.content, outBufferSize, outBuffer );
            Decompress( compressedSize, outBuffer, inFile.fileSize, testBuffer );
            
            if( memcmp( inFile.content, testBuffer, inFile.fileSize ) )
            {
                fprintf( stderr, "Test Failed!\n" );
            }
            else
            {
                fprintf( stderr, "Test succeded!\n" );
            }
        }
        else
        {
            fprintf( stderr, "Unrecognized command %s \n", argv[1] );
        }
        
        if( finalOutputBuffer )
        {
            FILE* outfile = fopen( outFilename, "wb" );
            if( outfile )
            {
                fwrite( finalOutputBuffer, 1, finalOutputSize, outfile );
            }
            else
            {
                fprintf( stderr, "unable to open output file%s\n", outFilename );
            }
        }
    }
    else
    {
        fprintf( stderr, "Usage: %s compress [raw filename] [compressed filename]\n", argv[0] );
    }
}
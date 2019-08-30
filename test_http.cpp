#include <stdio.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

static char* globalHost = "forgivenessthegame.com";
static char* globalPort = "443";

#define BUFF_SIZE 65000
struct AuthenticationToken
{
    char token[BUFF_SIZE];
};


struct SSLConnection
{
    char host[64];
    BIO* bio;
    SSL_CTX* ctx;
    
    char sendBuffer[BUFF_SIZE];
    char receiveBuffer[BUFF_SIZE];
    char messageBuffer[BUFF_SIZE];
    
    AuthenticationToken token;
};

inline void GenerateGetRequest(SSLConnection* connection, char* script, char* paramsString, bool keepAlive = true)
{
    sprintf(connection->sendBuffer, "GET https://%s/%s?%s HTTP/1.1\r\n"
            "Host: %s\r\n%s\r\n", connection->host, script, paramsString, connection->host, keepAlive ? "Connection: keep-alive\r\n" : "");
}

inline void GenerateAuthenticationRequest(SSLConnection* connection, char* email, char* password)
{
    char params[1024];
    sprintf(params, "email=%s&password=%s", email, password);
    GenerateGetRequest(connection, "api/auth/index.php", params);
}


inline void GenerateUpRequest(SSLConnection* connection, char* requestType, char* script, char* token, char* content, int contentSize, bool keepAlive = true)
{
    sprintf(connection->sendBuffer, "%s https://%s/%s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Authorization: Bearer %s\r\n"
            "Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n"
            "Accept: application/json\r\n"
            "%s"
            "Content-Length: %d\r\n"
            "\r\n%s", requestType, connection->host, script, connection->host, token, keepAlive ? "Connection: keep-alive\r\n" : "",contentSize, content);
}

inline void GeneratePutRequest(SSLConnection* connection, char* script, char* token, char* content, int contentSize, bool keepAlive = true)
{
    GenerateUpRequest(connection, "PUT", script, token, content, contentSize, keepAlive);
}

inline void GeneratePostRequest(SSLConnection* connection, char* script, char* token, char* content, int contentSize, bool keepAlive = true)
{
    GenerateUpRequest(connection, "POST", script, token, content, contentSize, keepAlive);
}






inline void Error(char* error)
{
    printf(error);
}

inline bool StrEqual(char* s1, char* s2)
{
    bool result = (!strcmp(s1, s2));
    return result;
}

bool InitializeSSL(SSLConnection* connection, char* host, char* port)
{
    bool result = true;
    
    char connectionString[128];
    sprintf(connectionString, "www.%s:%s", host, port);
    
    SSL_library_init();
    
    connection->ctx = SSL_CTX_new(SSLv23_client_method());
    if(!connection->ctx)
    {
        printf("Ctx is null\n");
        result = false;
    }
    
    connection->bio = BIO_new_ssl_connect(connection->ctx);
    BIO_set_conn_hostname(connection->bio, connectionString);
    
    if(BIO_do_connect(connection->bio) <= 0)
    {
        Error("Failed connection\n");
        result = false;
    }
    
    
    sprintf(connection->host, "%s", host);
    return result;
}

void CloseSSL(SSLConnection* connection)
{
    BIO_free_all(connection->bio);
    SSL_CTX_free(connection->ctx);
}

bool SSLFlushQueue(SSLConnection* connection)
{
    bool result = true;
    if(BIO_write(connection->bio, connection->sendBuffer, (int) strlen(connection->sendBuffer)) <= 0)
    {
        if(!BIO_should_retry(connection->bio))
        {
            printf("should retry!\n");
        }
        
        result = false;
        printf("error sending message!");
    }
    
    return result;
}

void WaitForReply(SSLConnection* connection)
{
    char* recv = connection->messageBuffer;
    for(;;)
    {
        int size = BIO_read(connection->bio, connection->receiveBuffer, sizeof(connection->receiveBuffer) - 1);
        if(size <= 0)
        {
            break;
        }
        
        connection->receiveBuffer[size] = 0;
        recv += sprintf(recv, "%s", connection->receiveBuffer);
        printf("raw message of length %d received: %s", size, connection->receiveBuffer);
        
        if(!strstr(connection->messageBuffer, "Transfer-Encoding: chunked") || strstr(connection->receiveBuffer, "0\r\n\r\n"))
        {
            break;
        }
    }
    
    printf("finished receiving message\n");
}


bool WriteTokenIntoBuffer(char* message, char* buffer, char* tag)
{
    bool result = false;
    
    char* token = strstr(message, tag);
    if(token)
    {
        result = true;
        char* data = strstr(token, ":");
        char* actualData = data + 2;
        char* dataEnd = strstr(actualData, "\"");
        
        int size = (int) (dataEnd - actualData);
        
        sprintf(buffer, "%.*s", size, actualData);
        
    }
    
    return result;
}

bool Authenticate(SSLConnection* connection, char* email, char* password)
{
    bool result = false;
    
    {
        GenerateAuthenticationRequest(connection, email, password);
        if(!SSLFlushQueue(connection))
        {
            Error("Failed write\n");
        }
        else
        {
            WaitForReply(connection);
            if(WriteTokenIntoBuffer(connection->messageBuffer, connection->token.token, "payload"))
            {
                result = true;
            }
        }
    }
    
    return result;
}


void RequestChangePassword(SSLConnection* connection, char* mail, char* password, char* newPassword)
{
    char content[1024];
    sprintf(content, "email=%s&password=%s&new_password=%s", mail, password, newPassword);
    int contentLength = (int) strlen(content);
    
    GeneratePutRequest(connection, "api/user/index.php", connection->token.token, content, contentLength);
    
    if(!SSLFlushQueue(connection))
    {
    }
    else
    {
        WaitForReply(connection);
    }
}


void IncrementPlaytime(SSLConnection* connection, char* mail)
{
    char content[1024];
    sprintf(content, "email=%s", mail);
    int contentLength = (int) strlen(content);
    
    GeneratePostRequest(connection, "api/playtime/index.php", connection->token.token, content, contentLength);
    
    if(!SSLFlushQueue(connection))
    {
    }
    else
    {
        WaitForReply(connection);
    }
}

void PostReview(SSLConnection* connection, char* mail, char* name, char* review, char* rating)
{
    char content[65000];
    sprintf(content, "email=%s&name=%s&review=%s&rating=%s", mail, name, review, rating);
    int contentLength = (int) strlen(content);
    
    GeneratePostRequest(connection, "api/review/index.php", connection->token.token, content, contentLength);
    
    if(!SSLFlushQueue(connection))
    {
    }
    else
    {
        WaitForReply(connection);
    }
}

#if 0        
char* email = "someguy@mail.com";
char* newPassword = "helloworld2";
#endif


int main(int argc, char** argv) 
{
    SSLConnection connection;
    if(!InitializeSSL(&connection, globalHost, globalPort))
    {
        printf("error initializing connection!");
    }
    else
    {
        if(argc < 2)
        {
            printf("first argument must be a command: changePassword, review, playtime\n");
        }
        else
        {
            char* command = argv[1];
            if(StrEqual(command, "changePassword"))
            {
                if(argc != 5)
                {
                    printf("wrong arguments!\n");
                }
                else
                {
                    char* email = argv[2];
                    char* password = argv[3];
                    char* newPassword = argv[4];
                    
                    if(Authenticate(&connection, email, password))
                    {
                        RequestChangePassword(&connection, email, password, newPassword);
                    }
                }
                
            }
            else if(StrEqual(command, "review"))
            {
                if(argc != 7)
                {
                    printf("wrong arguments!\n");
                }
                else
                {
                    char* email = argv[2];
                    char* password = argv[3];
                    char* name = argv[4];
                    char* review = argv[5];
                    char* rating = argv[6];
                    
                    if(Authenticate(&connection, email, password))
                    {
                        if(!strlen(name) || (!StrEqual(rating, "0") && !StrEqual(rating, "1")))
                        {
                            printf("wrong arguments!\n");
                        }
                        else
                        {
                            PostReview(&connection, email, name, review, rating);
                        }
                    }
                }
            }
            else if(StrEqual(command, "playtime"))
            {
                if(argc != 4)
                {
                    printf("wrong arguments!\n");
                }
                else
                {
                    char* email = argv[2];
                    char* password = argv[3];
                    if(Authenticate(&connection, email, password))
                    {
                        IncrementPlaytime(&connection, email);
                    }
                }
            }
            else
            {
                printf("invalid command %s, valid commands are changePassword, review, playtime\n", command);
            }
            
        }
    }
    
    CloseSSL(&connection);
    return 0;
}
#pragma once


struct ReceiveNetworkPacketWork
{
    NetworkInterface** network;
    network_platform_receive_data* ReceiveData;
};

struct ClientNetworkInterface
{
    u32 serverChallenge;
    ForgNetworkReceiver receiver;
    ForgNetworkApplicationData nextSendUnreliableApplicationData;
    ForgNetworkApplicationData nextSendReliableApplicationData;
    
    r32 serverFPS;
    r32 networkTimeElapsed;
    NetworkInterface* network;
};


enum PredictionType
{
    Prediction_None,
    Prediction_EquipmentRemoved,
    Prediction_EquipmentAdded,
    Prediction_ActionBegan,
};

struct ClientPrediction
{
    PredictionType type;
    
    r32 timeLeft;
    u32 slot;
    EntityAction action;
    
    u32 taxonomy;
    GenerationData gen;
    u64 identifier;
};

enum DataFileSentType
{
    DataFileSent_Nothing,
    DataFileSent_Everything,
    DataFileSent_OnlyTaxonomies,
    DataFileSent_OnlyAssets,
};

#define MAX_DATA_FILE_NAME_LENGTH 64
struct DataFileArrived
{
    char name[MAX_DATA_FILE_NAME_LENGTH];
    
    u32 chunkSize;
    u32 fileSize;
    u32 runningFileSize;
    u8* data;
    
    DataFileArrived* next;
};

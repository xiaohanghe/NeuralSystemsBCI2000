#ifndef NEURALYNX_THREAD_H
#define NEURALYNX_THREAD_H

#define MAX_OBJ  5000
#define MAX_LEN  100
#define RB_SIZE  50

#include "GenericADC.h"
#include "OSThread.h"
#include <queue>

typedef unsigned long long uint64_t;

using namespace std;


class NeuralynxThread : public OSThread
{
public:
    NeuralynxThread(short samplingRateFromADC);
    ~NeuralynxThread();


    int getObjectsAndTypes();//Get the objects that the Cheetah software is currently using
    void Connect(bool);//Connect to the amplifier
    void CheckAmp();//Check the amplifier's setup and compare to the Parameters set by the user
    void cleanup(bool disconnect); //Disconnects from amp if disconnect==true and cleans all memeory


    short getNextValue();
    bool bufferFilled, keepRunning;
    int numCSC, numEvC, calcSamplingFreq, samplingRateFromADC;

private:
    int maxCscSamples, recordBufferSize, maxEventStringLength;
    char cheetahObjects[MAX_OBJ][MAX_LEN];
    char cheetahTypes[MAX_OBJ][MAX_LEN];
    int cscIndex[MAX_OBJ];
    int evIndex[MAX_OBJ];


    int numEventsRead, numEventsWritten;
    int mEventTtlBinaryBase;

    int numRecordsReturned, numRecordsDropped;
    int mIndex;
    int mBufferSample, mBufferSampleReturned, mChannelReturned, indexFollower, eventFollower;
    queue<uint64_t> mIndexOfTimeStamps;

protected:

    char *channelNamesForChunk;
    int channelNamesSize;
    short *retBufSamples;
    uint64_t *retBufTimeStamps;
    int *retBufChannelNumbers;
    int *retBufValidSamples;
    int *retBufSamplingFreq;

    int *retBufEventIDs;
    int *retBufTTL;
    char **retBufEventString;

    typedef struct
    {
        char *name;
        int eventID;
        int ttl;
    } Event;

    typedef struct
    {
        int status; 	// 0=empty, 1=being filled, 2=being sent
        int chansFilled;
        uint64_t timeStamp;
        short *samples; /* numChannels x maxCscSamples */
        Event *events;
    } MultiChannelBlock;


    typedef struct {
        Event *events;
    } EventBlock;


    MultiChannelBlock mcbRingBuf[RB_SIZE];
    EventBlock evRingBuf[RB_SIZE];

    private:
        int Execute();

};




#endif //NEURALYNX_THREAD_H


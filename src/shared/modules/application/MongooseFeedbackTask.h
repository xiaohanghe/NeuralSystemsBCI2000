#ifndef MONGOOSETASK_H
#define MONGOOSETASK_H

#include <queue>
#include <vector>

#include "FeedbackTask.h"
#include "OSMutex.h"
#include "mongoose.h"
#include "BlockRandSeq.h"

class MongooseFeedbackTask : public FeedbackTask {
public:
    MongooseFeedbackTask();
    virtual ~MongooseFeedbackTask();

    /*
     * Loops infinitely and polls for incoming connections
     * Note: The underlying server can be reinitialized and swapped out at any time
     */
    friend void *MongooseServerThread(void *arg);

    /*
     * Handles all requests when the application is not ready (or not running)
     * Defines the GET methods for the game files
     *   and the POST /log method
     * Other methods are passed along to HandleMongooseRequest(...)
     */
    friend int MongooseServerHandler(struct mg_connection *conn);
    
    /*
     * Handles:
     *    GET /trial/status
     *   POST /trial/start
     *   POST /trial/stop
     * Additional functionality can be added by extending the appropriate delegate function
     */
    int HandleMongooseRequest(struct mg_connection *conn);

protected:
    /*
     * Checks MongooseFeedbackTask specific parameter settings
     * This should be called within Preflight
     */
    void CheckServerParameters(const SignalProperties& Input) const;

    /*
     * Creates the Mongoose server
     * This should be called within Initialization
     */
    void InitializeServer(const SignalProperties& Input, int numTrialTypes);
    
    /*
     * Resets some state in preparation for a run
     */
    void MongooseOnStartRun();
    
    /*
     * Resets some state after a run
     */
    void MongooseOnStopRun();
    
    /*
     * Tells the application to start a trial
     * Child classes can extend this
     */
    virtual void HandleTrialStartRequest(std::string data) {}
    
    /*
     * Tells the application to stop a trial
     * Child classes can extend this
     */
    virtual void HandleTrialStopRequest() {}
    
    /* This allows the game to poll for a status
     *   REFRESH means the run has ended and the game should refresh
     * Child classes can extend this to return other data, such as: 
     *   HIT when a TMS pulse should be triggered
     * Should return true if the connection is consumed
     */   
    virtual bool HandleTrialStatusRequest(struct mg_connection *conn) { return false; }
    
    /*
     * Provides synchronization for the Countdown game state
     * Any function that touches private variables of this class must first acquire this lock
     * Note: Reading does not (in most cases) require locking
     */
    OSMutex *state_lock;

    /*
     * Derived classes should set this value to True
     * IFF the class is ready to process requests
     */
    bool isRunning;

    /*
     * Should a trial start, stop, or continue?
     */
    enum TrialState {
        START_TRIAL,
        STOP_TRIAL,
        CONTINUE
    };

    /*
     * Holds the most recent trial-state command issued by the Countdown game
     * A state of CONTINUE means that this value has been processed
     *   and is awaiting a new command from the game.
     */
    TrialState lastClientPost;
    
    /*
     * When the Countdown game starts a trial, it should fetch the trial type from here
     */
    BlockRandSeq nextTrialType;

    /*
     * This value is updated when the Countdown game calls POST /trial/start
     */
    int currentTrialType;

    /*
     * Holds whether a set of trials has ended
     * The game is expected to poll for this value regularly
     * When it is sent to the game, the value is reset to false.
     */
    bool runEnded;

private:
    /*
     * Provides synchronization for the Mongoose server
     * This allows a new server instance to be swapped in at any time
     */
    OSMutex *server_lock;

    /*
     * Mongoose server
     * Serves up the files within the directory it is given
     *   and handles the various communication REST functions
     * A new server instance is created every time the configuration is set
     */
	struct mg_server *server;
    
    /*
     * How many different trial types are there?
     */
    int numTrialTypes;
};

#endif // MONGOOSETASK_H

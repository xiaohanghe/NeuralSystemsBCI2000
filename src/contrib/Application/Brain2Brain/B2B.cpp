#include "PCHIncludes.h"
#pragma hdrstop

#include "B2B.h"
#include "Localization.h"
#include "DFBuildScene2D.h"

#include "buffers.h"
#include <stdio.h>
#include <math.h>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#include <QImage>

#define CURSOR_POS_BITS "12"
const int cCursorPosBits = ::atoi( CURSOR_POS_BITS );

RegisterFilter( DynamicFeedbackTask, 3 );

DynamicFeedbackTask::DynamicFeedbackTask()
    : mpFeedbackScene( NULL ),
      mpMessage( NULL ),
      mpMessage2( NULL ),
      mCursorColor( RGBColor::White ),
      mRunCount( 0 ),
      mTrialCount( 0 ),
      mCurFeedbackDuration( 0 ),
      mMaxFeedbackDuration( 0 ),
      mCursorSpeedX( 1.0 ),
      mCursorSpeedY( 1.0 ),
      mCursorSpeedZ( 1.0 ),
      mScore(0.0),
      mrWindow( Window() ),
      mVisualFeedback(false),
      mIsVisualCatchTrial(false) {
      
    // Note: See MongooseTask.cpp for more parameters and states
    
    BEGIN_PARAMETER_DEFINITIONS
    "Application:Targets matrix Targets= "
        " 2 " // rows
        " [pos%20x pos%20y width%20x width%20y] " // columns
        " 50 90 20 20 "
        " 50 10 20 20 "
        " // Number of targets and their position (center) and dimensions in percentage coordinates",
    "Application:Targets int TargetColor= 0x0000FF % % % " // Blue
        " // target color (color)",

    "Application:Cursor float CursorWidth= 10 10 0.0 % "
        " // feedback cursor width in percent of screen width",
    "Application:Cursor int CursorColor= 0xff0000 % % % " // Red
        " // Cursor color (color)",
    "Application:Cursor floatlist CursorPos= 3 50 50 50 % % "
        " // cursor starting position",

    "Application:Sequencing float MaxFeedbackDuration= 3s % 0 % "
        " // abort a trial after this amount of feedback time has expired",

    "Application:3DEnvironment int WorkspaceBoundaryColor= 0xffffff 0 % % "
        " // workspace boundary color (0xff000000 for invisible) (color)",
    "Application:Feedback int VisualFeedback= 1 1 0 1 "
        "// provide visual stimulus (boolean)",
    "Application:Feedback intlist VisualCatchTrials= 4 1 3 4 2 % % % // "
        "// list of visual catch trials, leave empty for none",
    END_PARAMETER_DEFINITIONS

    BEGIN_STATE_DEFINITIONS
        "CursorPosX 12 0 0 0",
        "CursorPosY 12 0 0 0",
        "GameScore 16 0 0 0",
        "TrialType              2 0 0 0", // 0 for Airplane, 1 for Missile
        "CountdownHitReported   1 0 0 0", // Spacebar was pressed in the Countdown game
        "CountdownMissileScore  8 0 0 0", // Number of missiles shot
        "CountdownAirplaneScore 8 0 0 0", // Number of airplanes shot
    END_STATE_DEFINITIONS

    // Title screen message
    GUI::Rect rect = {0.5f, 0.4f, 0.5f, 0.6f};
    mpMessage = new TextField(mrWindow);
    mpMessage->SetTextColor(RGBColor::Lime)
              .SetTextHeight(0.8f)
              .SetColor(RGBColor::Gray)
              .SetAspectRatioMode(GUI::AspectRatioModes::AdjustWidth)
              .SetObjectRect(rect);

    // Score message in top right corner
    GUI::Rect rect3 = {0.89f, 0.01f, .99f, 0.09f};
    mpMessage2 = new TextField(mrWindow);
    mpMessage2->SetTextColor(RGBColor::Black)
               .SetTextHeight(0.45f)
               .SetColor(RGBColor::White)
               .SetAspectRatioMode(GUI::AspectRatioModes::AdjustNone)
               .SetObjectRect(rect3);
               
    state_lock = new OSMutex();
}

DynamicFeedbackTask::~DynamicFeedbackTask() {
    delete mpFeedbackScene;
}

void
DynamicFeedbackTask::OnPreflight(const SignalProperties& Input) const {
    if (Parameter("Targets")->NumValues() <= 0) {
        bcierr << "At least one target must be specified" << endl;
    }

    if (Parameter("CursorPos")->NumValues() != 3) {
        bcierr << "Parameter \"CursorPos\" must have 3 entries" << endl;
    }

    const char* colorParams[] = {
        "CursorColor",
        "TargetColor",
        "WorkspaceBoundaryColor"
    };
    for (size_t i = 0; i < sizeof(colorParams) / sizeof(*colorParams); ++i) {
        if (RGBColor(Parameter(colorParams[i])) == RGBColor(RGBColor::NullColor)) {
            bcierr << "Invalid RGB value in " << colorParams[ i ] << endl;
        }
    }

    if (Parameter("FeedbackDuration").InSampleBlocks() <= 0) {
        bcierr << "FeedbackDuration must be greater 0" << endl;
    }

    Parameter("SampleBlockSize");

    if (Parameter("VisualFeedback") == 1) {
        ParamRef visualCatch = Parameter("VisualCatchTrials");
        for (int i = 0; i < visualCatch->NumValues(); ++i) {
            if (visualCatch(i) < 1) {
                bcierr << "Invalid stimulus code "
                       << "(" << visualCatch(i) << ") "
                       << "at visualCatch(" << i << ")"
                       << endl;
            }
        }
    }
    
    CheckServerParameters();
}

void
DynamicFeedbackTask::OnInitialize(const SignalProperties& Input) {
    InitializeServer();

    // Cursor speed in pixels per signal block duration:
    float feedbackDuration = Parameter("FeedbackDuration").InSampleBlocks();

    // On average, we need to cross half the workspace during a trial.
    mCursorSpeedX = 100.0 / feedbackDuration / 2;
    mCursorSpeedY = 100.0 / feedbackDuration / 2;
    mCursorSpeedZ = 100.0 / feedbackDuration / 2;

    mMaxFeedbackDuration = static_cast<int>(Parameter("MaxFeedbackDuration").InSampleBlocks());

    mCursorColor = RGBColor(Parameter("CursorColor"));

    delete mpFeedbackScene;
    mpFeedbackScene = new DFBuildScene2D(mrWindow);
    mpFeedbackScene->Initialize();
    mpFeedbackScene->SetCursorColor(mCursorColor);

    mrWindow.Show();
    DisplayMessage("Timeout");
    DisplayScore("0");

    mVisualFeedback = Parameter("VisualFeedback") == 1;

    if (mVisualFeedback == true) {
        mVisualCatchTrials.clear();
        for (int j = 0; j < Parameter("VisualCatchTrials")->NumValues(); ++j) {
            mVisualCatchTrials.push_back(Parameter("VisualCatchTrials")(j));
        }
    }
}

void
DynamicFeedbackTask::OnStartRun() {
    // Reset state of the game
    state_lock->Acquire();
    isRunning = true;
    countdownMissileScore = 0;
    countdownAirplaneScore = 0;
    lastClientPost = CONTINUE;
    targetHit = false;
    runEnded = false;

    // Determine the trial types
    if (nextTrialType != NULL) {
        delete nextTrialType;
    }
    nextTrialType = new std::queue<TrialType>();
    // Note: This parameter is defined in FeedbackTask.cpp
    if (!std::string(Parameter("NumberOfTrials")).empty()) {
        // We know the number of trials,
        // so we can enforce an equal number of each trial type
        int numTrials = Parameter("NumberOfTrials");
        int typesPerTrial = numTrials / LAST;
        vector<TrialType> trialVector(numTrials, AIRPLANE);
        for (int i = AIRPLANE + 1; i < LAST; i++) {
            for (int j = 0; j < typesPerTrial; j++) {
                trialVector[i * typesPerTrial + j] = static_cast<TrialType>(i);
            }
        }

        // Shuffle and store the ordering
        std::random_shuffle(trialVector.begin(), trialVector.end());
        for (unsigned int i = 0; i < trialVector.size(); i++) {
            nextTrialType->push(trialVector[i]);
        }
    }
    state_lock->Release();
    
    // Reset various counters
    ++mRunCount;
    mTrialCount = 0;
    mTrialStatistics.Reset();
    mScore = 0;
    State("GameScore") = mScore;

    AppLog << "Run #" << mRunCount << " started" << endl;
    DisplayMessage(">> Get Ready! <<");
}

void
DynamicFeedbackTask::DoPreRun(const GenericSignal&, bool& doProgress) {
    // Wait for the start signal
    doProgress = false;

    if (lastClientPost == START_TRIAL) {
        doProgress = true;
    }
}

void
DynamicFeedbackTask::OnTrialBegin() {
    ++mTrialCount;
    AppLog.Screen << "Trial #" << mTrialCount
                  << ", target: " << State("TargetCode")
                  << endl;

    if (mVisualFeedback == true) {
        mIsVisualCatchTrial = false;
        for (size_t i = 0; i < mVisualCatchTrials.size(); i++) {
            mIsVisualCatchTrial = (mVisualCatchTrials.at(i) == mTrialCount);
        }

        if (mIsVisualCatchTrial == true) {
            AppLog.Screen << "<- visual catch trial" << endl;
        }
    }

    DisplayMessage("");
    RGBColor targetColor = RGBColor(Parameter("TargetColor"));
    for (int i = 0; i < mpFeedbackScene->NumTargets(); ++i) {
        mpFeedbackScene->SetTargetColor(targetColor, i);
        mpFeedbackScene->SetTargetVisible(State("TargetCode") == (i + 1), i);
    }

    // Reset trial-specific Countdown state
    state_lock->Acquire();
    targetHit = false;
    countdownSpacebarPressed = false;
    state_lock->Release();
}

void
DynamicFeedbackTask::OnFeedbackBegin() {
    mCurFeedbackDuration = 0;

    enum { x, y, z };
    ParamRef CursorPos = Parameter("CursorPos");
    MoveCursorTo(CursorPos(x), CursorPos(y), CursorPos(z));
    if (mVisualFeedback == true && mIsVisualCatchTrial == false) {
        mpFeedbackScene->SetCursorVisible(true);
    }
}

void
DynamicFeedbackTask::DoFeedback(const GenericSignal& ControlSignal, bool& doProgress) {
    doProgress = false;

    // Update cursor position
    float x = mpFeedbackScene->CursorXPosition(),
    y = mpFeedbackScene->CursorYPosition(),
    z = mpFeedbackScene->CursorZPosition();

    // Use the control signal to move up and down
    if (ControlSignal.Channels() > 0) {
        y += mCursorSpeedX * ControlSignal( 0, 0 );
    }

    // Restrict cursor movement to the inside of the bounding box:
    float r = mpFeedbackScene->CursorRadius();
    x = max(r, min(100 - r, x)),
    y = max(r, min(100 - r, y)),
    z = max(r, min(100 - r, z));
    mpFeedbackScene->SetCursorPosition(x, y, z);

    const float coordToState = ((1 << cCursorPosBits) - 1) / 100.0;
    State("CursorPosX") = static_cast<int>(x * coordToState);
    State("CursorPosY") = static_cast<int>(y * coordToState);

    if( mpFeedbackScene->TargetHit(State("TargetCode") - 1)) {
        State("ResultCode") = State("TargetCode");
        mpFeedbackScene->SetCursorColor(RGBColor::White);
        mpFeedbackScene->SetTargetColor(RGBColor::Red, State("ResultCode") - 1);

        // Pass this information onto the Countdown client
        state_lock->Acquire();
        targetHit = true;
        state_lock->Release();
        
        doProgress = true;
    }
    
    // Save whatever state is available to BCI2000
    state_lock->Acquire();
    State("TrialType") = currentTrialType;
    State("CountdownHitReported") = countdownSpacebarPressed;
    State("CountdownMissileScore") = countdownMissileScore;
    State("CountdownAirplaneScore") = countdownAirplaneScore;

    // Check for the stop signal
    if (lastClientPost == STOP_TRIAL) {
        doProgress = true;
    }
    state_lock->Release();
}

void
DynamicFeedbackTask::OnFeedbackEnd() {
    if (State("ResultCode") == 0) {
        AppLog.Screen << "-> aborted" << endl;
        mTrialStatistics.UpdateInvalid();

    } else {
        mTrialStatistics.Update(State("TargetCode"), State("ResultCode"));
        if (State("TargetCode") == State("ResultCode")) {
            AppLog.Screen << "-> hit\n " << "Your Score:" << mScore << endl;
            State("GameScore") = mScore;
        } else {
            mScore = mScore;
            AppLog.Screen << "-> miss\n " << "Your Score:" << mScore << endl;
            State("GameScore") = mScore;
        }
    }

    mpFeedbackScene->SetCursorVisible(false);

    // Persistent Score Display
    stringstream ss (stringstream::in | stringstream::out);
    int intScore = mScore >= 0 ? (int)(mScore + 0.5) : (int)(mScore - 0.5);
    ss << intScore;
    DisplayScore(ss.str());
}

void
DynamicFeedbackTask::OnTrialEnd(void) { };

void
DynamicFeedbackTask::DoITI(const GenericSignal&, bool& doProgress) {
    doProgress = false;

    // Wait for the start signal
    if (lastClientPost == START_TRIAL) {
        doProgress = true;
    }
}

void
DynamicFeedbackTask::OnStopRun() {
    AppLog << "Run " << mRunCount        << " finished: "
           << mTrialStatistics.Total()   << " trials, "
           << mTrialStatistics.Hits()    << " hits, "
           << mTrialStatistics.Invalid() << " invalid.\n";
    int validTrials = mTrialStatistics.Total() - mTrialStatistics.Invalid();
    if (validTrials > 0)
    AppLog << (200 * mTrialStatistics.Hits() + 1) / validTrials / 2  << "% correct, "
           << mTrialStatistics.Bits() << " bits transferred.\n, "
           << "Game Score:\n " << mScore
           << "====================="  << endl;

    // Indicate that the run has ended
    state_lock->Acquire();
    runEnded = true;
    isRunning = false;
    state_lock->Release();

    DisplayMessage("Timeout");
}


// Access to graphic objects
void
DynamicFeedbackTask::MoveCursorTo(float inX, float inY, float inZ) {
    mpFeedbackScene->SetCursorPosition(inX, inY, inZ);
}

void
DynamicFeedbackTask::DisplayMessage(const string& inMessage) {
    if (inMessage.empty()) {
        mpMessage->Hide();
    } else {
        mpMessage->SetText(" " + inMessage + " ");
        mpMessage->Show();
    }
}
void
DynamicFeedbackTask::DisplayScore(const string&inMessage) {
    if (inMessage.empty()) {
        mpMessage2->Hide();
    } else {
        mpMessage2->SetText("+" + inMessage + " ");
        mpMessage2->Show();
    }
}

/*
 * Splits a string by a delimiter
 */
static vector<string> split(const std::string input, char delimiter) {
    std::stringstream splitter(input);
    std::string item;
    std::vector<string> items;
    while (getline(splitter, item, delimiter)) {
        if (!item.empty()) {
            items.push_back(item);
        }
    }
    return items;
}

void DynamicFeedbackTask::HandleTrialStartRequest(struct mg_connection *conn) {
    // Fetch the next trial type
    if (nextTrialType->empty()) {
        currentTrialType = static_cast<TrialType>(rand() % LAST);
    } else {
        currentTrialType = nextTrialType->front();
        nextTrialType->pop();
    }

    // Pass along the trial type
    mg_send_status(conn, 200);
    mg_send_header(conn, "Content-Type", "text/plain");
    mg_printf_data(conn, "%d", currentTrialType);
    lastClientPost = START_TRIAL;
}

void DynamicFeedbackTask::HandleTrialStopRequest(struct mg_connection *conn) {
    mg_send_status(conn, 204);
    mg_send_data(conn, "", 0);
    lastClientPost = STOP_TRIAL;

    // Process the query string
    std::vector<string> queries = split(std::string(conn->query_string), '&');
    for (unsigned int i = 0; i < queries.size(); i++) {
        std::vector<string> parts = split(queries[i], '=');
        if (parts.size() != 2) {
            bciout << "Bad query: " << queries[i] << endl;
            continue;
        }

        // We expect integers
        if (parts[0].compare("spacebar") == 0) {
            countdownSpacebarPressed = stoi(parts[1]);
        } else if (parts[0].compare("missile") == 0) {
            countdownMissileScore = stoi(parts[1]);
        } else if (parts[0].compare("airplane") == 0) {
            countdownAirplaneScore = stoi(parts[1]);
        } else {
            bciout << "Unknown query: " << parts[0] << "=" << parts[1] << endl;
        }
    }
}

void HandleTrialStatusRequest(struct mg_connection *conn) {
    if (targetHit) {
        mg_send_status(conn, 200);
        mg_send_header(conn, "Content-Type", "text/plain");
        mg_printf_data(conn, "HIT");
        targetHit = false;
    } else if (runEnded) {
        mg_send_status(conn, 200);
        mg_send_header(conn, "Content-Type", "text/plain");
        mg_printf_data(conn, "REFRESH");
        runEnded = false;
    } else {
        mg_send_status(conn, 204);
        mg_send_data(conn, "", 0);
    }
}

int DynamicFeedbackTask::HandleMongooseRequest(struct mg_connection *conn) {
    std::string method(conn->request_method);
    std::string uri(conn->uri);

    state_lock->Acquire();
    if (method.compare("POST") == 0) {
        if (uri.compare("/trial/start") == 0) {
            HandleTrialStartRequest(conn);
            
            state_lock->Release();
            return MG_REQUEST_PROCESSED;

        } else if (uri.compare("/trial/stop") == 0) {
            HandleTrialStopRequest(conn);
            
            state_lock->Release();
            return MG_REQUEST_PROCESSED;
        }

    } else if (method.compare("GET") == 0) {
        if (uri.compare("/trial/status") == 0) {
            HandleTrialStatusRequest(conn);
            
            state_lock->Release();
            return MG_REQUEST_PROCESSED;
        }
    }
    
    state_lock->Release();
    return MG_REQUEST_NOT_PROCESSED;
}


////////////////////////////////////////////////////////////////////////////////
// $Id: B2B.cpp 4008 2012-05-15 12:42:40Z dsarma $
//
////////////////////////////////////////////////////////////////////////////////
#include "PCHIncludes.h"
#pragma hdrstop

#include "B2B.h"
#include "Localization.h"
//#include "FileUtils.h"
#include "DFBuildScene2D.h"
//#include "FeedbackScene3D.h"

#include "buffers.h"
#include <stdio.h>
#include <math.h>
#include <sstream>
#include <algorithm>

#ifndef __BORLANDC__
#include <QImage>
#endif // __BORLANDC__

#define CURSOR_POS_BITS "12"
const int cCursorPosBits = ::atoi( CURSOR_POS_BITS );

#define SCORE_BITS "16"
const int ScoreBits = ::atoi( SCORE_BITS );

#define TASK_DIFF_BITS "12"
const int TDiffBits = ::atoi( TASK_DIFF_BITS );

#define TARGET_XWIDTH_BITS "12"  
const int TargetXWidthBits = ::atoi( TARGET_XWIDTH_BITS );

#define TARGET_YWIDTH_BITS "12"  
const int TargetYWidthBits = ::atoi( TARGET_YWIDTH_BITS );

#define CURSOR_RADIUS_BITS "12"  
const int CursorRadiusBits = ::atoi( CURSOR_RADIUS_BITS );

#define PRIMARY_AXIS_BITS "12"  
const int PrimaryAxisBits = ::atoi( PRIMARY_AXIS_BITS );

#define DEFAULT_LINE_BUFFER 50

RegisterFilter( DynamicFeedbackTask, 3 );

using namespace std;

DynamicFeedbackTask::DynamicFeedbackTask()
: mpFeedbackScene( NULL ),
  mRenderingQuality( 0 ),
  mpMessage( NULL ),
  mpMessage2( NULL ),
  //mpBackground( NULL ),
  mCursorColorFront( RGBColor::White ),
  mCursorColorBack( RGBColor::White ),
  mRunCount( 0 ),
  mTrialCount( 0 ),
  mCurFeedbackDuration( 0 ),
  mMaxFeedbackDuration( 0 ),
  mCursorSpeedX( 1.0 ),
  mCursorSpeedY( 1.0 ),
  mCursorSpeedZ( 1.0 ),
  mScore(0.0),//new score variable?
  mScoreCount(0.0),//score counter
  mTaskDiff(1.0),
  mrWindow( Window() ),
  //mFeedbackModulationType ( 0 ),
  //mTactorBoard( NULL ),
  //mTactileFeedback( false ),
  mVisualFeedback( false),
  //mIsTactileCatchTrial( false ),
  mIsVisualCatchTrial( false )
  //mTactorVar( 0.0 ),
  //mTactorVarDelta( 0.0 ),
  //mMaxValue( 0 ),
  //mMinValue( 0 ),
  //mHeldGain( 0 ),
  //mHeldFrequency( 0 )

{
  BEGIN_PARAMETER_DEFINITIONS
	  "Application:Targets int TaskDifficulty= 5"
	  " // Difficulty is from to 1(hardest) to 5(easiest) ",
	  "Application:Targets matrix Targets= "
      " 5 " // rows
      " [pos%20x pos%20y pos%20z width%20x width%20y width%20z] " // columns
      "  50  90  50 8 8 8 "
      "  50  75  50 8 8 8 "
      "  50  25  50 8 8 8 "
      "  50  10  50 8 8 8 "
      "   0   0   4 8 8 8 "
	  /*"  35  35  50 8 8 8 "
      "  65  35  50 8 8 8 "
      "  35  65  50 8 8 8 "
      "  65  65  50 8 8 8 "
      "   0   0   4 8 8 8 "*/
      " // target positions and widths in percentage coordinates",
    "Application:Targets int TargetColor= 0x0000FF % % % " //0x808080
       " // target color (color)",
    "Application:Targets string TargetTexture= % % % % "
      " // path of target texture (inputfile)",
    "Application:Targets int TestAllTargets= 0 0 0 1 "
      " // test all targets for cursor collision? "
          "0: test only the visible current target, "
          "1: test all targets "
          "(enumeration)",
	
    "Application:Cursor float CursorWidth= 10 10 0.0 % "
      " // feedback cursor width in percent of screen width",
	"Application:Cursor int KeyboardControl= 1 1 0 1 "
      "// use Keyboard Control (boolean)",
	"Application:Cursor int cPrimaryAxis= 1"
	 " // set cursor movement axis: "
		"1 X: Keyboard (L/R); Y: Control Signal,"
		"    2 X: Control Signal; Y: Keyboard (U/D)",
	"Application:Cursor int CursorControlMode= 0 0 0 1 "
      " // Select cursor control mode "
          "0: centering gravity mode, "
          "1: velocity dependent mode "
          "(enumeration)",
	"Application:Cursor float CursorSpeedMult= 3"
       " // Cursor speed from to 1(slowest) to 10(fastest) ",
	"Application:Cursor float CursorGravity= 1"
       " // Centering cursor gravity from to 0 (weakest) to 10 (strongest) ",
    "Application:Cursor int CursorColorFront= 0xff0000 % % % "
       " // cursor color when it is at the front of the workspace (color)",
    "Application:Cursor int CursorColorBack= 0xFF0000 % % % " //0xffff00
       " // cursor color when it is in the back of the workspace (color)",
    "Application:Cursor string CursorTexture= % % % %"
      " // path of cursor texture (inputfile)",
    "Application:Cursor floatlist CursorPos= 3 50 50 50 % % "
      " // cursor starting position",

    "Application:Window int RenderingQuality= 0 0 0 1 "
      " // rendering quality: 0: low, 1: high (enumeration)",

    "Application:Sequencing float MaxFeedbackDuration= 3s % 0 % "
      " // abort a trial after this amount of feedback time has expired",

    "Application:3DEnvironment floatlist CameraPos= 3 50 50 150 % % "
      " // camera position vector in percent coordinates of 3D area",
    "Application:3DEnvironment floatlist CameraAim= 3 50 50 50 % % "
      " // camera aim point in percent coordinates",
    "Application:3DEnvironment int CameraProjection= 0 0 0 2 "
      " // projection type: 0: flat, 1: wide angle perspective, 2: narrow angle perspective (enumeration)",
    "Application:3DEnvironment floatlist LightSourcePos= 3 50 50 100 % % "
      " // light source position in percent coordinates",
    "Application:3DEnvironment int LightSourceColor= 0x808080 % % "
      " // light source RGB color (color)",
    "Application:3DEnvironment int WorkspaceBoundaryColor= 0xffffff 0 % % "
      " // workspace boundary color (0xff000000 for invisible) (color)",
    "Application:3DEnvironment string WorkspaceBoundaryTexture= images/grid.bmp % % % "
      " // path of workspace boundary texture (inputfile)",

    "Application:Feedback int VisualFeedback= 1 1 0 1 "
      "// provide visual stimulus (boolean)",
    //"Application:Feedback int TactileFeedback= 1 1 0 1 "
    //  "// provide tactile stimulus (boolean)",
    "Application:Feedback intlist VisualCatchTrials= 4 1 3 4 2 % % % // "
	  "// list of visual catch trials, leave empty for none",
 //   "Application:Feedback intlist TactileCatchTrials= 4 1 3 4 2 % % % // "
	//  "// list of tactile catch trials, leave empty for none",
 //   "Application:Stimuli string TactorBoardComPort= % % % % // "
 //     "COM Port for target tactor control board",
	//"Application:Stimuli int TactileFeedbackModulationType= 0 0 0 1 // "
	//  "Tactile feedback modulation type 0 intensity, 1 frequency (enumeration)",
	//"Application:Stimuli int MaxValue= 320 0 % % // "
	//  "// max value for stimulus feedback",
	//"Application:Stimuli int MinValue= 30 0 % % // "
	//  "// min value for stimulus feedback",
	//"Application:Stimuli int HeldGain= 90 0 % % // "
	//  "// gain during frequency modulation",
	//"Application:Stimuli int HeldFrequency= 150 0 % % // "
	//  "// frequency during gain modulatin",

	"Application:Connector string ConnectorAddress= % "
    "localhost:20320 % % // IP address/port to read from, e.g. localhost:20320",
  END_PARAMETER_DEFINITIONS

  BEGIN_STATE_DEFINITIONS
    "CursorPosX " CURSOR_POS_BITS " 0 0 0",
    "CursorPosY " CURSOR_POS_BITS " 0 0 0",
    "CursorPosZ " CURSOR_POS_BITS " 0 0 0",
	"GameScore " SCORE_BITS " 0 0 0", //score bits
	"TaskDiff " TASK_DIFF_BITS " 0 0 0", //task difficulty
	"TargetXWidth " TARGET_XWIDTH_BITS " 0 0 0", //target width in x
	"TargetYWidth " TARGET_YWIDTH_BITS " 0 0 0", //target width in y
	"CursorRadius " CURSOR_RADIUS_BITS " 0 0 0", //cursor radius
	"PrimaryAxis " PRIMARY_AXIS_BITS " 0 0 0", //cursor radius
  END_STATE_DEFINITIONS

  //Title Screen Messages
  GUI::Rect rect = { 0.5f, 0.4f, 0.5f, 0.6f };
  mpMessage = new TextField( mrWindow );
  mpMessage->SetTextColor( RGBColor::Lime ) //Lime
            .SetTextHeight( 0.8f )
            .SetColor( RGBColor::Gray ) //Grey
            .SetAspectRatioMode( GUI::AspectRatioModes::AdjustWidth )
            .SetObjectRect( rect );

 
  /*
  GUI::Rect rect2 = { 0.8f, 0.00f, 1.0f, 0.1f };
  //rect2->SetColor(RGBColor::Lime);
  TextField mpBackground = TextField( mrWindow );
  mpBackground->setTextColor(RGBColor::Black);
			.SetColor(RGBColor::Lime)
			.SetObjectRect(rect2)
			.SetAspectRatioMode(GUI::AspectRatioModes::AdjustNone);
  */

  //Score Message in top right corner
  GUI::Rect rect3 = { 0.89f, 0.01f, .99f, 0.09f };
  mpMessage2 = new TextField( mrWindow );
  mpMessage2->SetTextColor( RGBColor::Black )
            .SetTextHeight( 0.45f )
            .SetColor( RGBColor::White )
			.SetAspectRatioMode( GUI::AspectRatioModes::AdjustNone)
            .SetObjectRect( rect3 );
}

DynamicFeedbackTask::~DynamicFeedbackTask() {
  delete mpFeedbackScene;
}

void
DynamicFeedbackTask::OnPreflight( const SignalProperties& Input ) const {
  const char* vectorParams[] = {
    "CameraPos",
    "CameraAim",
    "LightSourcePos",
    "CursorPos",
  };
  for( size_t i = 0; i < sizeof( vectorParams ) / sizeof( *vectorParams ); ++i )
    if( Parameter( vectorParams[ i ] )->NumValues() != 3 )
      bcierr << "Parameter \"" << vectorParams[ i ] << "\" must have 3 entries" << endl;

  Parameter( "WorkspaceBoundaryColor" );
  const char* colorParams[] = {
    "CursorColorBack",
    "CursorColorFront",
    "TargetColor",
    "LightSourceColor",
    // WorkspaceBoundaryColor may be NullColor to indicate invisibility
  };
  for( size_t i = 0; i < sizeof( colorParams ) / sizeof( *colorParams ); ++i )
    if( RGBColor( Parameter( colorParams[ i ] ) )  == RGBColor( RGBColor::NullColor ) )
      bcierr << "Invalid RGB value in " << colorParams[ i ] << endl;

  bool showTextures = ( Parameter( "RenderingQuality" ) > 0 );
  const char* texParams[] = {
    "CursorTexture",
    "TargetTexture",
    "WorkspaceBoundaryTexture",
  };
  for( size_t i = 0; i < sizeof( texParams ) / sizeof( *texParams ); ++i ) {
    string filename = Parameter( texParams[ i ] );
    if( showTextures && !filename.empty() )
    {
//      filename = FileUtils::AbsolutePath( filename );
      bool err = !ifstream( filename.c_str() ).is_open();
      if( !err )
      {
#ifdef __BORLANDC__
        AUX_RGBImageRec* pImg = buffers::loadWindowsBitmap( filename );
        if( pImg != NULL )
        {
          ::free( pImg->data );
          ::free( pImg );
        }
        err = ( pImg == NULL );
#else // __BORLANDC__
        QImage img;
        err = !img.load( QString( filename.c_str() ) );
#endif // __BORLANDC__
      }
      if( err )
        bcierr << "Invalid texture file \"" << filename << "\""
               << " given in parameter " << texParams[ i ]
               << endl;
    }
  }

  if( Parameter( "NumberTargets" ) > Parameter( "Targets" )->NumRows() )
    bcierr << "The Targets parameter must contain at least NumberTargets "
           << "target definitions. "
           << "Currently, Targets contains "
           << Parameter( "Targets" )->NumRows()
           << " target definitions, and NumberTargets is "
           << Parameter( "NumberTargets" )
           << endl;
  
  if( (Parameter( "TaskDifficulty" ) < 1) || (Parameter( "TaskDifficulty" ) > 5) )
    bcierr << "Difficulty must be between 1 and 5" << endl;

  if( (Parameter( "cPrimaryAxis" ) < 1) || (Parameter( "cPrimaryAxis" ) > 2))
    bcierr << "Axis must be either 1 or 2" << endl;

  if( Parameter( "FeedbackDuration" ).InSampleBlocks() <= 0 )
    bcierr << "FeedbackDuration must be greater 0" << endl;

  if( (Parameter( "CursorSpeedMult" ) > 10) || (Parameter( "CursorSpeedMult" ) < 1) )
    bcierr << "Cursor speed must be between 1 and 10" << endl;

  if( (Parameter( "CursorGravity" ) > 10) ||  (Parameter( "CursorGravity" ) < 0))
    bcierr << "Cursor gravity must be between 1 and 10" << endl;

  OptionalParameter( "EnforceFixation" );

  //Check state KeyUp exists
  if (Parameter( "KeyboardControl" ) == 1){
  State("KeyUp");
  State("KeyDown");
  }
  Parameter( "SampleBlockSize" );

  if (Parameter( "VisualFeedback" ) == 1) {
	  ParamRef visualCatch = Parameter( "VisualCatchTrials" );
	  for( int i = 0; i < visualCatch->NumValues(); ++i )
		if( visualCatch( i ) < 1 )
		  bcierr << "Invalid stimulus code "
				 << "(" << visualCatch( i ) << ") "
				 << "at visualCatch(" << i << ")"
				 << endl;
  }
  
  string connectorAddress = string( Parameter( "ConnectorAddress" ) );
}

void
DynamicFeedbackTask::OnInitialize( const SignalProperties& /*Input*/ ) {
  mConnectorAddress = string( Parameter( "ConnectorAddress" ) );

  // Cursor speed in pixels per signal block duration:
  float feedbackDuration = Parameter( "FeedbackDuration" ).InSampleBlocks();
  // On average, we need to cross half the workspace during a trial.
  mCursorSpeedX = 100.0 / feedbackDuration / 2;
  mCursorSpeedY = 100.0 / feedbackDuration / 2;
  mCursorSpeedZ = 100.0 / feedbackDuration / 2;
  
  mMaxFeedbackDuration = static_cast<int>( Parameter( "MaxFeedbackDuration" ).InSampleBlocks() );
  mTaskDiff = static_cast<int>( Parameter( "TaskDifficulty" )); //Task difficulty
  
  mCursorColorFront = RGBColor( Parameter( "CursorColorFront" ) );
  mCursorColorBack = RGBColor( Parameter( "CursorColorBack" ) );
  mCursorAxis = static_cast<int>( Parameter( "cPrimaryAxis" )); //Primary control axis

  int renderingQuality = Parameter( "RenderingQuality" );
  if( renderingQuality != mRenderingQuality ) {
    mrWindow.Hide();
    mRenderingQuality = renderingQuality;
  }
  delete mpFeedbackScene;
  mpFeedbackScene = new DFBuildScene2D( mrWindow );
  mpFeedbackScene->Initialize();
  mpFeedbackScene->SetCursorColor( mCursorColorFront );

  mrWindow.Show();
  DisplayMessage( "Timeout" );
  DisplayScore("0");

  mVisualFeedback = ( Parameter( "VisualFeedback" ) == 1);

  if (mVisualFeedback == true) {
	  mVisualCatchTrials.clear();
      for( int j = 0; j < Parameter( "VisualCatchTrials" )->NumValues(); ++j )
	      mVisualCatchTrials.push_back( Parameter( "VisualCatchTrials" )( j ) );
  }
}

void
DynamicFeedbackTask::OnStartRun() {
  ++mRunCount;
  mTrialCount = 0;
  mTrialStatistics.Reset();
  mScore = 0;
  State("GameScore") = mScore;
  State("TaskDiff") = mTaskDiff;
  State("CursorRadius") = Parameter( "CursorWidth" );
  State("PrimaryAxis") = mCursorAxis;

  AppLog << "Run #" << mRunCount << " started" << endl;
  DisplayMessage( ">> Get Ready! <<" );

  mSocket.open( mConnectorAddress );
}

void
DynamicFeedbackTask::DoPreRun( const GenericSignal&, bool& doProgress ) {
  // Wait for the start signal
  doProgress = false;
  if (mSocket.can_read()) {
	char buffer[DEFAULT_LINE_BUFFER];
    mSocket.read(buffer, DEFAULT_LINE_BUFFER);
	
	std::string line(buffer);
    if (line.compare("t_start") != 0) {
      bciout << "DoPreRun: Unexpected input = \"" << line << "\"" << endl;
      return;
    }
    doProgress = true;
  }
}

void
DynamicFeedbackTask::OnTrialBegin() {
  ++mTrialCount;
  AppLog.Screen << "Trial #" << mTrialCount
                << ", target: " << State( "TargetCode" )
                << endl;

  if (mVisualFeedback == true) {
	  mIsVisualCatchTrial = false;
	  for (size_t i = 0; i < mVisualCatchTrials.size(); i++)
		  mIsVisualCatchTrial = (mVisualCatchTrials.at(i) == mTrialCount);

	  if (mIsVisualCatchTrial == true) {
		  AppLog.Screen << "<- visual catch trial" << endl;
	  }
  }


  enum { x, y, z, dx, dy, dz };
  ParamRef Targets = Parameter( "Targets" );
  State("TargetXWidth") = Targets( State("TargetCode") - 1, dx );
  //State("TargetXWidth") = Targets( State("TargetCode") - 1, dy ); //Original. State variable name typo?
  State("TargetYWidth") = Targets( State("TargetCode") - 1, dy );

  DisplayMessage( "" );
  RGBColor targetColor = RGBColor( Parameter( "TargetColor" ) );
  for( int i = 0; i < mpFeedbackScene->NumTargets(); ++i )
  {
    mpFeedbackScene->SetTargetColor( targetColor, i );
    mpFeedbackScene->SetTargetVisible( State( "TargetCode" ) == i + 1, i );
  }
}

void
DynamicFeedbackTask::OnFeedbackBegin() {
  mCurFeedbackDuration = 0;

  enum { x, y, z };
  ParamRef CursorPos = Parameter( "CursorPos" );
  MoveCursorTo( CursorPos( x ), CursorPos( y ), CursorPos( z ) );
  if (mVisualFeedback == true && mIsVisualCatchTrial == false)
    mpFeedbackScene->SetCursorVisible( true );
}

void
DynamicFeedbackTask::DoFeedback( const GenericSignal& ControlSignal, bool& doProgress ) {
	doProgress = false;
  
  // Update cursor position
  float x = mpFeedbackScene->CursorXPosition(),
        y = mpFeedbackScene->CursorYPosition(),
        z = mpFeedbackScene->CursorZPosition();
  
	// Restrict cursor movement to the inside of the bounding box:
	float r = mpFeedbackScene->CursorRadius();
	x = max( r, min( 100 - r, x ) ),
	y = max( r, min( 100 - r, y ) ),
	z = max( r, min( 100 - r, z ) );
	mpFeedbackScene->SetCursorPosition( x, y, z );

	const float coordToState = ( ( 1 << cCursorPosBits ) - 1 ) / 100.0;
	State( "CursorPosX" ) = static_cast<int>( x * coordToState );
	State( "CursorPosY" ) = static_cast<int>( y * coordToState );
	State( "CursorPosZ" ) = static_cast<int>( z * coordToState );

  // AppLog.Screen << "Cursor distance: " << mpFeedbackScene->CursorTargetDistance( State( "TargetCode" ) - 1 ) << endl;

  if( mpFeedbackScene->TargetHit( State( "TargetCode" ) - 1 ) ) {
    State( "ResultCode" ) = State( "TargetCode" );
    mpFeedbackScene->SetCursorColor( RGBColor::White );
    mpFeedbackScene->SetTargetColor( RGBColor::Red, State( "ResultCode" ) - 1 );

    // Send message of hit out to B2B Game
    mSocket.write("1", 1);
    doProgress = true;
  }

  if (mSocket.can_read()) {
	char buffer[DEFAULT_LINE_BUFFER];
    mSocket.read(buffer, DEFAULT_LINE_BUFFER);
	
	std::string line(buffer);
    if (line.compare("t_stop") != 0) {
      bciout << "DoFeedback: Unexpected input = \"" << line << "\"" << endl;
      return;
    }
    doProgress = true;
  }
}

void
DynamicFeedbackTask::OnFeedbackEnd() {
  if( State( "ResultCode" ) == 0 ) {
    AppLog.Screen << "-> aborted" << endl;
    mTrialStatistics.UpdateInvalid();

  } else {
    mTrialStatistics.Update( State( "TargetCode" ), State( "ResultCode" ) );
    if( State( "TargetCode" ) == State( "ResultCode" ) ) {
	    //mScore = mScore + 100;//NEED to figure out way to add based on target... try with multi-targ for now.
      //mpFeedbackScene->SetCursorColor( RGBColor::Yellow );
      //mpFeedbackScene->SetTargetColor( RGBColor::Yellow, State( "ResultCode" ) - 1 );
      AppLog.Screen << "-> hit\n " << "Your Score:" << mScore << endl;
	    State("GameScore") = mScore;
    } else {
      mScore = mScore;
	    AppLog.Screen << "-> miss\n " << "Your Score:" << mScore << endl;
	    State("GameScore") = mScore;
    }
  }

  //Persistent Score Display
  stringstream ss (stringstream::in | stringstream::out);
  int intScore = mScore >= 0 ? (int)(mScore + 0.5) : (int)(mScore - 0.5);
  ss << intScore;
  DisplayScore(ss.str());

  if (mVisualFeedback == true && mIsVisualCatchTrial == false) {
    // mpFeedbackScene->SetCursorVisible( false );
  }
}

void
DynamicFeedbackTask::OnTrialEnd() {
  for( int i = 0; i < mpFeedbackScene->NumTargets(); ++i ) {
    // mpFeedbackScene->SetTargetVisible( false, i );
  }
}

void
DynamicFeedbackTask::DoITI( const GenericSignal&, bool& doProgress ) {
  // Wait for the start signal
  doProgress = false;
  if (mSocket.can_read()) {
	char buffer[DEFAULT_LINE_BUFFER];
    mSocket.read(buffer, DEFAULT_LINE_BUFFER);
	
	std::string line(buffer);
    if (line.compare("t_stop") == 0) {
      bciout << "DoITI: Already waiting for t_start" << endl;
    }

    if (line.compare("t_start") != 0) {
      bciout << "DoITI: Unexpected input = \"" << line << "\"" << endl;
      return;
    }
    doProgress = true;
  }
}

void
DynamicFeedbackTask::OnStopRun() {
  AppLog   << "Run " << mRunCount << " finished: "
           << mTrialStatistics.Total() << " trials, "
           << mTrialStatistics.Hits() << " hits, "
           << mTrialStatistics.Invalid() << " invalid.\n";
  int validTrials = mTrialStatistics.Total() - mTrialStatistics.Invalid();
  if( validTrials > 0 )
    AppLog << ( 200 * mTrialStatistics.Hits() + 1 ) / validTrials / 2  << "% correct, "
           << mTrialStatistics.Bits() << " bits transferred.\n, "
		   << "Game Score:\n " << mScore ;
  AppLog   << "====================="  << endl;

  DisplayMessage( "Timeout" );
  mSocket.close();
}


// Access to graphic objects
void
DynamicFeedbackTask::MoveCursorTo( float inX, float inY, float inZ )
{
  // Adjust the cursor's color according to its z position:
  float z = inZ / 100;
  RGBColor color = z * mCursorColorFront + ( 1 - z ) * mCursorColorBack;
  mpFeedbackScene->SetCursorColor( color );
  mpFeedbackScene->SetCursorPosition( inX, inY, inZ );
}

void
DynamicFeedbackTask::DisplayMessage( const string& inMessage )
{
  if( inMessage.empty() )
  {
    mpMessage->Hide();
  }
  else
  {
    mpMessage->SetText( " " + inMessage + " " );
    mpMessage->Show();
  }
}
void
DynamicFeedbackTask::DisplayScore( const string& inMessage )
{
  if( inMessage.empty() )
  {
    mpMessage2->Hide();
  }
  else
  {
    mpMessage2->SetText( "+" + inMessage + " " );
    mpMessage2->Show();
  }
}
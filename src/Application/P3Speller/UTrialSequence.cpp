//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <stdio.h>

#include "UCoreComm.h"
#include "UParameter.h"
#include "UState.h"
#include "UTrialSequence.h"
#include "UTaskUtil.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)


// **************************************************************************
// Function:   TRIALSEQUENCE
// Purpose:    This is the constructor of the TRIALSEQUENCE class
// Parameters: plist - pointer to the paramter list
//             slist - pointer to the state list
// Returns:    N/A
// **************************************************************************
TRIALSEQUENCE::TRIALSEQUENCE(PARAMLIST *plist, STATELIST *slist)
{
int     i;
char    line[512];

 targets=new TARGETLIST();

 strcpy(line, "P3Speller string TargetDefinitionFile= p3targets.cfg 0 0 100 // Target definition file");
 plist->AddParameter2List(line,strlen(line));
 strcpy(line,"P3Speller int OnTime= 4 10 0 5000 // Duration of intensification in units of SampleBlocks");
 plist->AddParameter2List(line,strlen(line) );
 strcpy(line,"P3Speller int OffTime= 1 10 0 5000 // Interval between intensification in units of SampleBlocks");
 plist->AddParameter2List(line,strlen(line) );
 strcpy(line,"P3Speller int OnlineMode= 0 0 0 1 // Online mode (0=no, 1=yes)");
 plist->AddParameter2List(line,strlen(line) );
 strcpy(line,"P3Speller string TextColor= 0x00000000 0x00505050 0x00000000 0x00000000 // Text Color in hex (0x00BBGGRR)");
 plist->AddParameter2List(line,strlen(line));
 strcpy(line,"P3Speller string TextColorIntensified= 0x000000FF 0x00505050 0x00000000 0x00000000 // Text Color in hex (0x00BBGGRR)");
 plist->AddParameter2List(line,strlen(line));
 strcpy(line,"P3Speller string TextToSpell= P P A Z // Character or string to spell in offline mode");
 plist->AddParameter2List(line,strlen(line));

 vis=NULL;

 slist->AddState2List("StimulusCode 5 0 0 0\n");
 slist->AddState2List("StimulusType 3 0 0 0\n");
 slist->AddState2List("Flashing 1 0 0 0\n");
}


// **************************************************************************
// Function:   ~TRIALSEQUENCE
// Purpose:    This is the destructor of the TRIALSEQUENCE class
// Parameters: N/A
// Returns:    N/A
// **************************************************************************
TRIALSEQUENCE::~TRIALSEQUENCE()
{
 if (vis) delete vis;
 vis=NULL;
 if (targets) delete targets;
 targets=NULL;
}


// **************************************************************************
// Function:   Initialize
// Purpose:    Initialize is called after parameterization to initialize the trial sequence
// Parameters: plist        - pointer to the paramter list
//             new_svect    - current pointer to the state vector
//             new_corecomm - pointer to the communication object
//             new_userdisplay - pointer to the userdisplay (that contains the status bar, the cursor, and the currently active targets)
// Returns:    0 ... if there was a problem (e.g., a necessary parameter does not exist)
//             1 ... OK
// **************************************************************************
int TRIALSEQUENCE::Initialize( PARAMLIST *plist, STATEVECTOR *new_svect, CORECOMM *new_corecomm, USERDISPLAY *new_userdisplay)
{
int     ret;

 corecomm=new_corecomm;

 // load and create all potential targets
 ret=LoadPotentialTargets(plist->GetParamPtr("TargetDefinitionFile")->GetValue());
 if (ret == 0)
    {
    corecomm->SendStatus("416 P3 Speller: Could not open target definition file");
    return(0);
    }

 statevector=new_svect;
 userdisplay=new_userdisplay;

 // if (vis) delete vis;
 // vis= new GenericVisualization( plist, corecomm );
 // vis->SetSourceID(SOURCEID_SPELLERTRIALSEQ);
 // vis->SendCfg2Operator(SOURCEID_SPELLERTRIALSEQ, CFGID_WINDOWTITLE, "Speller Trial Sequence");

 ret=1;
 try
  {
  ontime=atoi(plist->GetParamPtr("OnTime")->GetValue());
  offtime=atoi(plist->GetParamPtr("OffTime")->GetValue());
  TextColor=(TColor)strtol(plist->GetParamPtr("TextColor")->GetValue(), NULL, 16);
  TextColorIntensified=(TColor)strtol(plist->GetParamPtr("TextColorIntensified")->GetValue(), NULL, 16);
  TextToSpell=AnsiString(plist->GetParamPtr("TextToSpell")->GetValue());
  if (atoi(plist->GetParamPtr("OnlineMode")->GetValue()) == 1)
     onlinemode=true;
  else
     onlinemode=false;
  }
 catch(...)
  {
  ret=0;
  ontime=10;
  offtime=3;
  TextColor=clYellow;
  TextColorIntensified=clRed;
  TextToSpell="G";
  chartospell="G";
  onlinemode=false;
  }

 // get the active targets as a subset of all the potential targets
 if (userdisplay->activetargets) delete userdisplay->activetargets;
 userdisplay->activetargets=GetActiveTargets();       // get the targets that are on the screen first

 // set the initial position/sizes of the current targets, status bar, cursor
 userdisplay->InitializeActiveTargetPosition();
 userdisplay->InitializeStatusBarPosition();
 // userdisplay->DisplayCursor();
 userdisplay->DisplayMessage("Waiting to start ...");

 oldrunning=0;

 // reset the trial's sequence
 ResetTrialSequence();

 return(ret);
}


int TRIALSEQUENCE::LoadPotentialTargets(const char *targetdeffilename)
{
char    buf[256], line[256];
FILE    *fp;
int     targetID, parentID, displaypos, ptr, targettype;
TARGET  *cur_target;

 // if we already have a list of potential targets, delete this list
 if (targets) delete targets;
 targets=new TARGETLIST();

 // read the target definition file
 fp=fopen(targetdeffilename, "rb");
 if (!fp) return(0);

 while (!feof(fp))
  {
  fgets(line, 255, fp);
  if (strlen(line) > 2)
     {
     ptr=0;
     // first column ... target code
     ptr=get_argument(ptr, buf, line, 255);
     targetID=atoi(buf);
     cur_target=new TARGET(targetID);
     // second column ... caption
     ptr=get_argument(ptr, buf, line, 255);
     cur_target->Caption=AnsiString(buf).Trim();
     // third column ... icon
     ptr=get_argument(ptr, buf, line, 255);
     cur_target->IconFile=AnsiString(buf).Trim();
     targettype=TARGETTYPE_NOTYPE;
     if ((targetID == TARGETID_BLANK) || (targetID == TARGETID_BACKUP) || (targetID == TARGETID_ROOT))
        targettype=TARGETTYPE_CONTROL;
     if ((targetID >= TARGETID_A) && (targetID <= TARGETID__))
        targettype=TARGETTYPE_CHARACTER;
     if ((targetID >= TARGETID_ABCDEFGHI) && (targetID <= TARGETID_YZ_))
        targettype=TARGETTYPE_CHARACTERS;
     cur_target->targettype=targettype;
     targets->Add(cur_target);
     }
  }
 fclose(fp);

 return(1);
}


// gets new targets, given a certain rootID
// i.e., the targetID of the last selected target
// parentID==TARGETID_ROOT on start
TARGETLIST *TRIALSEQUENCE::GetActiveTargets()
{
TARGETLIST      *new_list;
TARGET          *target, *new_target;;
int             targetID;

 new_list=new TARGETLIST;                                               // the list of active targets
 new_list->parentID=0;

 for (targetID=1; targetID<=36; targetID++)
  {
  target=targets->GetTargetPtr(targetID);
  if ((targetID >= 0) && (target))
     {
     // add to active targets
     new_target=target->CloneTarget();
     new_target->Color=clYellow;
     new_target->TextColor=TextColor;
     new_target->parentID=0;
     new_target->targetposition=targetID;
     new_target->targetID=targetID;
     new_list->Add(new_target);
     }
   }

 return(new_list);
}


// **************************************************************************
// Function:   get_argument
// Purpose:    parses a line
//             it returns the next token that is being delimited by a ";"
// Parameters: ptr - index into the line of where to start
//             buf - destination buffer for the token
//             line - the whole line
//             maxlen - maximum length of the line
// Returns:    the index into the line where the returned token ends
// **************************************************************************
int TRIALSEQUENCE::get_argument(int ptr, char *buf, const char *line, int maxlen) const
{
 // skip one preceding semicolon, if there is any
 if ((line[ptr] == ';') && (ptr < maxlen))
    ptr++;

 // go through the string, until we either hit a semicolon, or are at the end
 while ((line[ptr] != ';') && (line[ptr] != '\n') && (line[ptr] != '\r') && (ptr < maxlen))
  {
  *buf=line[ptr];
  ptr++;
  buf++;
  }

 *buf=0;
 return(ptr);
}


// **************************************************************************
// Function:   ResetTrialSequence
// Purpose:    Resets the trial's sequence to the beginning of the ITI
// Parameters: N/A
// Returns:    N/A
// **************************************************************************
void TRIALSEQUENCE::ResetTrialSequence()
{
char    line[256];

 cur_trialsequence=0;
 cur_on=false;

 selectedtarget=NULL;
 cur_stimuluscode=0;

 // initialize block randomized numbers
 InitializeBlockRandomizedNumber();

 statevector->SetStateValue("StimulusCode", 0);
 statevector->SetStateValue("StimulusType", 0);
}


// **************************************************************************
// Function:   GetRandomStimulusCode
// Purpose:    Resets the trial's sequence to the beginning of the ITI
// Parameters: N/A
// Returns:    N/A
// **************************************************************************
int TRIALSEQUENCE::GetRandomStimulusCode()
{
int     num;

 num=GetBlockRandomizedNumber(NUM_STIMULI);

return(num);
}


// **************************************************************************
// Function:   SuspendTrial
// Purpose:    Turn off display when trial gets suspended
//             also, turn off all the states so that they are not 'carried over' to the next run
// Parameters: N/A
// Returns:    N/A
// **************************************************************************
void TRIALSEQUENCE::SuspendTrial()
{
 userdisplay->HideActiveTargets();           // hide all active targets
 userdisplay->HideStatusBar();               // hide the status bar

 statevector->SetStateValue("StimulusCode", 0);
 statevector->SetStateValue("StimulusType", 0);
 statevector->SetStateValue("Flashing", 0);

 userdisplay->DisplayMessage("TIME OUT !!!");      // display the "TIME OUT" message
}


// this (dis)intensifies a row/column and returns
// 1 if the character to spell is within this row/column
short TRIALSEQUENCE::IntensifyTargets(int stimuluscode, bool intensify)
{
TARGET  *cur_target, *chartospellptr;
int     i, cur_row, cur_targetID, chartospellID;
short   thisisit;

 // find the ID of the character that we currently want to spell
 chartospellptr=userdisplay->activetargets->GetTargetPtr(chartospell);
 if (chartospellptr)
    chartospellID=chartospellptr->targetID;
 else
    chartospellID=0;
 thisisit=0;

 // do we want to flash a column (stimuluscode 1..6) ?
 if ((stimuluscode > 0) && (stimuluscode <= 6))
    {
    for (i=0; i<userdisplay->activetargets->GetNumTargets(); i++)
     {
     if (i%6+1 == stimuluscode)
        {
        cur_target=userdisplay->activetargets->GetTargetPtr(i+1);
        if (intensify)
           cur_target->SetTextColor(TextColorIntensified);
        else
           cur_target->SetTextColor(TextColor);
        if ((i+1 == chartospellID) && (intensify))
           thisisit=1;
        }
     }
    }

 // do we want to flash a row (stimuluscode 7..12) ?
 if ((stimuluscode > 6) && (stimuluscode <= 12))
    {
    for (i=0; i<6; i++)
     {
     cur_row=stimuluscode-6;
     cur_targetID=(cur_row-1)*6+i+1;
     cur_target=userdisplay->activetargets->GetTargetPtr(cur_targetID);
     if (intensify)
        cur_target->SetTextColor(TextColorIntensified);
     else
        cur_target->SetTextColor(TextColor);
     if ((cur_targetID == chartospellID) && (intensify))
        thisisit=1;
     }
    }

 return(thisisit);
}


// **************************************************************************
// Function:   GetReadyForTrial
// Purpose:    Show matrix, etc.
// Parameters: N/A
// Returns:    N/A
// **************************************************************************
void TRIALSEQUENCE::GetReadyForTrial()
{
 userdisplay->HideMessage();                 // hide any message that's there
 userdisplay->DisplayStatusBar();
 userdisplay->DisplayActiveTargets();           // display all active targets
}


// **************************************************************************
// Function:   SetUserDisplayTexts
// Purpose:    Set goal and result text on userdisplay
// Parameters: N/A
// Returns:    N/A
// **************************************************************************
void TRIALSEQUENCE::SetUserDisplayTexts()
{
 // if we are in offline mode, then ...
 if (!onlinemode)
    {
    chartospell=TextToSpell.SubString(char2spellidx, 1);
    userdisplay->statusbar->goaltext=TextToSpell+" ("+chartospell+")";
    }
 else // if we are in online mode
    userdisplay->statusbar->goaltext="";
}



// **************************************************************************
// Function:   Process
// Purpose:    Processes the control signal provided by the task
// Parameters: controlsignal - pointer to the vector of control signals
// Returns:    pointer to the selected target (if one was selected), or NULL
// **************************************************************************
int TRIALSEQUENCE::Process(const short *controlsignal)
{
unsigned short running, within;
int     ret;

 running=statevector->GetStateValue("Running");

 // when we suspend the system, show the "TIME OUT" message
 if ((running == 0) && (oldrunning == 1))
    {
    SuspendTrial();
    oldrunning=0;
    }

 // when we just started with this character,
 // then show the matrix and reset the trial sequence
 if ((running == 1) && (oldrunning == 0))
    {
    ResetTrialSequence();
    GetReadyForTrial();
    }

 // are we at the end of the intensification period ?
 // if yes, turn off the row/column
 if (cur_on && (cur_trialsequence == ontime))
    {
    cur_on=false;
    cur_trialsequence=0;
    IntensifyTargets(cur_stimuluscode, false);
    cur_stimuluscode=0;
    statevector->SetStateValue("StimulusType", 0);
    statevector->SetStateValue("Flashing", 0);
    ret=1;
    }

 // are we at the end of the "dis-intensification" period ?
 // if yes, intensify the next row/column
 if (!cur_on && (cur_trialsequence == offtime))
    {
    cur_on=true;
    cur_trialsequence=0;
    cur_stimuluscode=GetRandomStimulusCode();
    within=IntensifyTargets(cur_stimuluscode, true);
    // in the online mode, we do not know whether this is standard or oddball
    if (onlinemode)
       statevector->SetStateValue("StimulusType", 0);
    else
       statevector->SetStateValue("StimulusType", within);
    statevector->SetStateValue("Flashing", 1);
    }

 statevector->SetStateValue("StimulusCode", cur_stimuluscode);

 oldrunning=running;
 cur_trialsequence++;
 return(ret);
}


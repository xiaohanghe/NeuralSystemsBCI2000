#ifndef USysLogH
#define USysLogH

#include <ComCtrls.hpp>
#include <SyncObjs.hpp>

class SYSLOG
{
  public:
    enum LogEntryMode
    {
      logEntryNormal = 0,
      logEntryWarning,
      logEntryError,
      numLogEntryModes
    };
    SYSLOG();
    ~SYSLOG();
    void ShowSysLog();
    void HideSysLog();
    bool Visible() const;
    bool Close( bool force = false );
    void AddSysLogEntry( const char* text, LogEntryMode = logEntryNormal );

  private:
    TForm*            mpForm;
    TRichEdit*        mpLog;
    TCriticalSection* mpCritsec;
    bool              mDontClose;
};
#endif // USysLogH


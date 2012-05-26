////////////////////////////////////////////////////////////////////
// $Id$
// Author: juergen.mellinger@uni-tuebingen.de
// Description: A QtScript shell that may be used to start up and
//   control BCI2000.
//
// $BEGIN_BCI2000_LICENSE$
//
// This file is part of BCI2000, a platform for real-time bio-signal research.
// [ Copyright (C) 2000-2012: BCI2000 team and many external contributors ]
//
// BCI2000 is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// BCI2000 is distributed in the hope that it will be useful, but
//                         WITHOUT ANY WARRANTY
// - without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// $END_BCI2000_LICENSE$
////////////////////////////////////////////////////////////////////
#include <iostream>

#include "BCI2000Connection.h"
#include "FileUtils.h"
#include "EnvVariable.h"
#include <cstring>

using namespace std;

static const string sShebang = "#!";
#if _WIN32
static const char* sOperatorName = "Operator.exe";
#else
static const char* sOperatorName = "Operator";
#endif

int main( int argc, char** argv )
{
  bool command = false,
       interactive = false,
       help = false;
  int idx = 1;
  while( idx < argc && *argv[idx] == '-' )
  {
    if( !::strcmp( argv[idx], "-c" ) )
      command = true;
    if( !::strcmp( argv[idx], "-i" ) )
      interactive = true;
    if( !::strcmp( argv[idx], "-h" ) || !::strcmp( argv[idx], "--help" ) )
      help = true;
    ++idx;
  }
  if( idx >= argc )
    interactive = true;
  string script = "";
  if( !command && !interactive && !help )
    script = FileUtils::AbsolutePath( argv[idx++] );
    
  string additionalArgs,
         telnetAddress;
  EnvVariable::Get( "BCI2000TelnetAddress", telnetAddress );
  while( idx < argc && argv[idx] != sShebang )
  {
    if( !::stricmp( argv[idx], "--Telnet" ) && idx++ < argc )
    {
      telnetAddress = argv[idx];
    }
    else
    {
      string arg = argv[idx++];
      if( arg.find( '\"' ) == string::npos )
        arg = "\"" + arg + "\""; 
      additionalArgs += " ";
      additionalArgs += arg;
    }
  }
  struct : BCI2000Connection
  {
    bool OnInput( string& s )
    { return getline( cin, s ); }
    bool OnOutput( const string& s )
    { return cout << s << endl; }
  } bci;
  bci.OperatorPath( "" );
  bci.TelnetAddress( telnetAddress );
  bci.WindowVisible( false );
  if( !bci.Connect() )
  {
    if( !bci.Run( FileUtils::InstallationDirectory() + sOperatorName, additionalArgs )
        || !bci.Connect() )
    {
      cerr << bci.Result() << endl;
      return -1;
    }
  }
  bci.Execute( "cd \"" + FileUtils::WorkingDirectory() + "\"" );
  bci.Timeout( 3600 );
  int exitCode = 0;
  if( interactive )
  {
    const string prompt = FileUtils::ExtractBase( FileUtils::ExecutablePath() ) + ">";
    cout << prompt << flush;
    string line;
    while( bci.Connected() && getline( cin, line ) )
    {
      if( bci.Connected() )
      {
        exitCode = bci.Execute( line );
        if( !bci.Result().empty() )
          cerr << bci.Result() << '\n';
      }
      else
      {
        cout << "[Lost connection to Operator module]" << endl;
      }
      if( bci.Connected() )
        cout << prompt << flush;
    }
  }
  else
  {
    if( help )
      script = "help";
    else if( command )
      script = additionalArgs;
    else
      script = "execute script \"" + script + "\"" + additionalArgs;
    exitCode = bci.Execute( script );
    if( !bci.Result().empty() )
      cerr << bci.Result() << '\n';
  }
  return exitCode;
}

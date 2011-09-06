//////////////////////////////////////////////////////////////////////
// $Id$
// Author: juergen.mellinger@uni-tuebingen.de
// Description: A thread class similar to the VCL's TThread.
//   To implement your own thread, create a class that inherits from
//   OSThread, and put your own functionality into its
//   Execute() function.
//
// $BEGIN_BCI2000_LICENSE$
//
// This file is part of BCI2000, a platform for real-time bio-signal research.
// [ Copyright (C) 2000-2011: BCI2000 team and many external contributors ]
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
///////////////////////////////////////////////////////////////////////
#include "PCHIncludes.h"
#pragma hdrstop

#include "OSThread.h"
#include "OSEvent.h"
#include "BCIError.h"
#include "OSError.h"
#include "ClassName.h"
#include "ExceptionCatcher.h"

#if !_WIN32
# include <unistd.h>
#endif // !_WIN32

using namespace std;

#if _WIN32
DWORD OSThread::sMainThreadID = ::GetCurrentThreadId();
#else // _WIN32
pthread_t OSThread::sMainThread = ::pthread_self();
#endif // _WIN32

#if _WIN32

OSThread::OSThread()
: mHandle( NULL ),
  mThreadID( 0 ),
  mResult( 0 ),
  mpTerminationEvent( NULL ),
  mTerminating( false )
{
}

OSThread::~OSThread()
{
  if( mHandle != NULL )
  {
    ::TerminateThread( mHandle, 0 );
    ::CloseHandle( mHandle );
  }
}

void
OSThread::Start()
{
  mHandle = ::CreateThread( NULL, 0, OSThread::StartThread, this, 0, &mThreadID );
  if( mHandle == NULL )
    bcierr << OSError().Message() << endl;
}

bool
OSThread::IsTerminated() const
{
  return mHandle == NULL;
}

void
OSThread::Sleep( int inMs )
{
  ::Sleep( inMs );
}

bool
OSThread::IsMainThread()
{
  return ::GetCurrentThreadId() == sMainThreadID;
}

#else // _WIN32

OSThread::OSThread()
: mTerminated( true ),
  mResult( 0 ),
  mpTerminationEvent( NULL ),
  mTerminating( false )
{
}

OSThread::~OSThread()
{
  if( !mTerminated )
    ::pthread_cancel( mThread );
}

void
OSThread::Start()
{
  ::pthread_attr_t attributes;
  if( !::pthread_attr_init( &attributes ) )
  {
    // Create a thread in detached mode, i.e. thread resources are
    // auto-released when the thread terminates.
    ::pthread_attr_setdetachstate( &attributes, PTHREAD_CREATE_DETACHED );
    ::pthread_create( &mThread, &attributes, OSThread::StartThread, this );
    ::pthread_attr_destroy( &attributes );
  }
}

bool
OSThread::IsTerminated() const
{
  return mTerminated;
}

void
OSThread::Sleep( int inMs )
{
  ::usleep( inMs * 1000 );
}

bool
OSThread::IsMainThread()
{
  return ::pthread_equal( pthread_self(), sMainThread );
}

#endif // _WIN32

void
OSThread::Terminate( OSEvent* inpEvent )
{
  mpTerminationEvent = inpEvent;
  mTerminating = true;
  if( IsTerminated() && mpTerminationEvent )
    mpTerminationEvent->Set();
}

bool
OSThread::TerminateWait( int inTimeout )
{
  OSEvent event;
  Terminate( &event );
  return event.Wait( inTimeout );
}

int
OSThread::CallExecute()
{
  MemberCall<int(OSThread*)> call( &OSThread::Execute, this );
  bool finished = ExceptionCatcher()
    .SetMessage( "Canceling thread of type " + bci::ClassName( typeid( *this ) ) )
    .Run( call );
  return finished ? call.Result() : -1;
}


#if _WIN32

DWORD WINAPI
OSThread::StartThread( void* inInstance )
{
  OSThread* this_ = reinterpret_cast<OSThread*>( inInstance );
  this_->mTerminating = false;
  this_->mResult = this_->CallExecute();
  ::CloseHandle( this_->mHandle );
  this_->mHandle = NULL;
  if( this_->mpTerminationEvent )
    this_->mpTerminationEvent->Set();
  this_->mpTerminationEvent = NULL;
  return this_->mResult;
}

#else // _WIN32

void*
OSThread::StartThread( void* inInstance )
{
  OSThread* this_ = reinterpret_cast<OSThread*>( inInstance );
  this_->mTerminating = false;
  this_->mTerminated = false;
  // Set the cancel state to asynchronous, so pthread_cancel() will
  // immediately cancel thread execution.
  ::pthread_setcancelstate( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );
  this_->mResult = this_->CallExecute();
  this_->mTerminated = true;
  if( this_->mpTerminationEvent )
    this_->mpTerminationEvent->Set();
  this_->mpTerminationEvent = NULL;
  return &this_->mResult;
}

#endif // _WIN32

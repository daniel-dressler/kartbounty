#ifndef _WIN32
// For time interface
#define _POSIX_C_SOURCE = 1999309L
#endif

#include "SELib.h"
#include "SETimer.h"
#include <stdlib.h>

#ifdef _WIN32
#include <Windows.h>
struct _TimerDataStruct
{
	LARGE_INTEGER	m_liFrequency;
	LARGE_INTEGER	m_liStartTime;
};
#else
#include <time.h>
#endif

#define NANOSECONDS_PER_SECOND (1000000000.0)


Timer::Timer( Int32 bAutoStart ) : m_pData( 0 )
{
#ifdef _WIN32
	m_pData = calloc( 1, sizeof(_TimerDataStruct) );
#else
	m_pData = calloc( 1, sizeof(struct timespec) );
#endif
	ResetClock();
}

Timer::~Timer()
{
	free( m_pData );
}

void Timer::ResetClock()
{
	if( !m_pData )
		return;

#ifdef _WIN32
	_TimerDataStruct& data = *(_TimerDataStruct*)m_pData;
	QueryPerformanceFrequency( &data.m_liFrequency );
	QueryPerformanceCounter( &data.m_liStartTime );
#else
	//clock_gettime(CLOCK_BOOTTIME, (struct timespec *)m_pData);
#endif
}

Float64 Timer::CalcSeconds()
{
	if( !m_pData )
		return 0;

	Float64 fTime;

#ifdef _WIN32
	_TimerDataStruct& data = *(_TimerDataStruct*)m_pData;
	LARGE_INTEGER liCurrentTime;
	QueryPerformanceCounter( &liCurrentTime );
	fTime = (Float64)( liCurrentTime.QuadPart - data.m_liStartTime.QuadPart ) / data.m_liFrequency.QuadPart;
#else
	struct timespec ts;
	clock_gettime(CLOCK_BOOTTIME, &ts);
	fTime = ts.tv_sec - ((struct timespec *)m_pData)->tv_sec;
	fTime += (ts.tv_nsec - ((struct timespec *)m_pData)->tv_nsec) / NANOSECONDS_PER_SECOND;
#endif

	return fTime;
}

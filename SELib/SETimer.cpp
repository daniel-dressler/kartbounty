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



#endif

Timer::Timer( Int32 bAutoStart ) : m_pData( 0 )
{
#ifdef _WIN32
	m_pData = MALLOC( sizeof(_TimerDataStruct) );
	MEMSET( m_pData, 0, sizeof(_TimerDataStruct) );
#else


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



#endif

	return fTime;
}

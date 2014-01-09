#ifndef __SE_TIMER__
#define __SE_TIMER__

class Timer
{
private:
	void*	m_pData;

public:
	void ResetClock();
	Float64 CalcSeconds();

	Timer( Int32 bAutoStart = 1 );
	virtual ~Timer();
};

#endif

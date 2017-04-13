// Code based on:
// http://chabster.blogspot.com/2012/10/realtime-etw-consumer-howto.html

#pragma once
#include "TraceConsumer.hpp"

#undef OpenTrace

class TraceSession
{

public:
    TraceSession(const wchar_t* szSessionName, const wchar_t* szFileName);
    ~TraceSession();

    TraceSession(const TraceSession&) = delete;
    TraceSession& operator=(const TraceSession&) = delete;

public:
    bool Start();
    bool EnableProvider(const GUID& providerId, UCHAR level, ULONGLONG anyKeyword = 0, ULONGLONG allKeyword = 0);
    bool CaptureState(const GUID& providerId, UCHAR level, ULONGLONG anyKeyword = 0, ULONGLONG allKeyword = 0);
    bool OpenTrace(ITraceConsumer *pConsumer);
    bool Process();
    bool CloseTrace();
    bool DisableProvider(const GUID& providerId);
    bool Stop();

    bool AnythingLost(uint32_t &events, uint32_t &buffers);

    ULONG Status() const;
    LONGLONG PerfFreq() const;

private:
    LPTSTR _szSessionName, _szFileName;
    ULONG _status;
    EVENT_TRACE_PROPERTIES* _pSessionProperties;
    TRACEHANDLE _hSession;
    EVENT_TRACE_LOGFILEW _logFile;
    TRACEHANDLE _hTrace;
    uint32_t _eventsLost, _buffersLost;
    bool _started;
};

// Code based on:
// http://chabster.blogspot.com/2012/10/realtime-etw-consumer-howto.html

#include "TraceSession.hpp"
#include "MessageLog.h"

static VOID WINAPI EventRecordCallback(_In_ PEVENT_RECORD pEventRecord)
{
    reinterpret_cast<ITraceConsumer *>(pEventRecord->UserContext)->OnEventRecord(pEventRecord);
}

static ULONG WINAPI BufferRecordCallback(_In_ PEVENT_TRACE_LOGFILE Buffer)
{
    return reinterpret_cast<ITraceConsumer *>(Buffer->Context)->ContinueProcessing();
}

TraceSession::TraceSession(const wchar_t* szSessionName, const wchar_t* szFileName)
    : _szSessionName(_wcsdup(szSessionName))
    , _szFileName(szFileName == nullptr ? nullptr : _wcsdup(szFileName))
    , _status(0)
    , _pSessionProperties(0)
    , _hSession(0)
    , _hTrace(0)
    , _eventsLost(0)
    , _buffersLost(0)
    , _started(false)
{
}

TraceSession::~TraceSession(void)
{
    free(_szSessionName);
    free(_szFileName);
    free(_pSessionProperties);
}

bool TraceSession::Start()
{
    if (!_pSessionProperties)
    {
        static_assert(sizeof(EVENT_TRACE_PROPERTIES) == 120, "");
        size_t sessionNameBytes = (wcslen(_szSessionName) + 1) * sizeof(wchar_t);
        size_t fileNameBytes = _szFileName ? (wcslen(_szFileName) + 1) * sizeof(wchar_t) : 0;
        size_t buffSize = sizeof(EVENT_TRACE_PROPERTIES) + sessionNameBytes + fileNameBytes;
        _pSessionProperties = reinterpret_cast<EVENT_TRACE_PROPERTIES *>(malloc(buffSize));
        if (_pSessionProperties == nullptr) {
            return false;
        }

        ZeroMemory(_pSessionProperties, buffSize);
        _pSessionProperties->Wnode.BufferSize = ULONG(buffSize);
        _pSessionProperties->Wnode.ClientContext = 1;
        _pSessionProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
        _pSessionProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
        _pSessionProperties->BufferSize = 0;
        _pSessionProperties->MaximumBuffers = 200;

        if (_szFileName)
        {
            _pSessionProperties->LogFileNameOffset = ULONG(_pSessionProperties->LoggerNameOffset + sessionNameBytes);
            wchar_t* dest = reinterpret_cast<wchar_t*>(reinterpret_cast<char*>(_pSessionProperties) + _pSessionProperties->LogFileNameOffset);
            wcscpy_s(dest, fileNameBytes/sizeof(wchar_t), _szFileName);
        }
    }

    // Create the trace session.
    _status = StartTraceW(&_hSession, _szSessionName, _pSessionProperties);
		if (_status != ERROR_SUCCESS) {
			g_messageLog.Log(MessageLog::LOG_ERROR, "TraceSession", "StartTraceW failed", _status);
		}
    _started = (_status == ERROR_SUCCESS);

    return _started;
}

bool TraceSession::EnableProvider(const GUID& providerId, UCHAR level, ULONGLONG anyKeyword, ULONGLONG allKeyword)
{
    if (_started)
    {
        _status = EnableTraceEx2(_hSession, &providerId, EVENT_CONTROL_CODE_ENABLE_PROVIDER, level, anyKeyword, allKeyword, 0, NULL);
				if (_status != ERROR_SUCCESS) {
					g_messageLog.Log(MessageLog::LOG_ERROR, "TraceSession", "EnableTraceEx2 failed", _status);
				}
    }
    return (_status == ERROR_SUCCESS);
}

bool TraceSession::CaptureState(const GUID& providerId, UCHAR level, ULONGLONG anyKeyword, ULONGLONG allKeyword)
{
    if (_started)
    {
        _status = EnableTraceEx2(_hSession, &providerId, EVENT_CONTROL_CODE_CAPTURE_STATE, level, anyKeyword, allKeyword, 0, NULL);
    }
    return (_status == ERROR_SUCCESS);
}

bool TraceSession::OpenTrace(ITraceConsumer *pConsumer)
{
    if (!pConsumer)
        return false;

    ZeroMemory(&_logFile, sizeof(EVENT_TRACE_LOGFILE));
    if (_started)
    {
        _logFile.LoggerName = _szSessionName;
        _logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME;
    }
    else
    {
        _logFile.LogFileName = _szFileName;
    }
    _logFile.ProcessTraceMode |= PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
    _logFile.EventRecordCallback = &EventRecordCallback;
    _logFile.BufferCallback = &BufferRecordCallback;
    _logFile.Context = pConsumer;

    _hTrace = ::OpenTraceW(&_logFile);

		if (_hTrace == (TRACEHANDLE)INVALID_HANDLE_VALUE) {
			g_messageLog.Log(MessageLog::LOG_ERROR, "TraceSession", "OpenTraceW failed");
		}

    return (_hTrace != (TRACEHANDLE)INVALID_HANDLE_VALUE);
}

bool TraceSession::Process()
{
    _status = ProcessTrace(&_hTrace, 1, NULL, NULL);
    return (_status == ERROR_SUCCESS);
}

bool TraceSession::CloseTrace()
{
    _status = ::CloseTrace(_hTrace);
		if (_status != ERROR_SUCCESS) {
			g_messageLog.Log(MessageLog::LOG_ERROR, "TraceSession", "CloseTrace failed", _status);
		}
    return (_status == ERROR_SUCCESS);
}

bool TraceSession::DisableProvider(const GUID& providerId)
{
    if (_started)
    {
        _status = EnableTraceEx2(_hSession, &providerId, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, NULL);
    }
		if (_status != ERROR_SUCCESS) {
			g_messageLog.Log(MessageLog::LOG_ERROR, "TraceSession", "DisableProvider failed", _status);
		}
    return (_status == ERROR_SUCCESS);
}

bool TraceSession::Stop()
{
    if (_pSessionProperties)
    {
        _status = ControlTraceW(_hSession, _szSessionName, _pSessionProperties, EVENT_TRACE_CONTROL_STOP);
        delete _pSessionProperties;
        _pSessionProperties = NULL;
        _started = false;
    }

		if (_status != ERROR_SUCCESS) {
			g_messageLog.Log(MessageLog::LOG_ERROR, "TraceSession", "Stop ControlTraceW failed", _status);
		}

    return (_status == ERROR_SUCCESS);
}

bool TraceSession::AnythingLost(uint32_t &events, uint32_t &buffers)
{
    if (_started)
    {
        _status = ControlTraceW(_hSession, _szSessionName, _pSessionProperties, EVENT_TRACE_CONTROL_QUERY);
        if (_status != ERROR_SUCCESS && _status != ERROR_MORE_DATA) {
            return true;
        }
        _status = ERROR_SUCCESS;
        events = _pSessionProperties->EventsLost - _eventsLost;
        buffers = _pSessionProperties->RealTimeBuffersLost - _buffersLost;
        _eventsLost = _pSessionProperties->EventsLost;
        _buffersLost = _pSessionProperties->RealTimeBuffersLost;
    }
    else
    {
        events = 0;
        buffers = 0;
    }
    return events > 0 || buffers > 0;
}

ULONG TraceSession::Status() const
{
    return _status;
}

LONGLONG TraceSession::PerfFreq() const
{
    return _logFile.LogfileHeader.PerfFreq.QuadPart;
}

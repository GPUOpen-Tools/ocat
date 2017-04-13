/*
Copyright 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Code based on:
// http://chabster.blogspot.com/2012/10/realtime-etw-consumer-howto.html

#pragma once
#include "CommonIncludes.hpp"

struct ITraceConsumer {
    virtual void OnEventRecord(_In_ PEVENT_RECORD pEventRecord) = 0;
    virtual bool ContinueProcessing() = 0;
    
    // Time of the first event of the trace
    uint64_t mTraceStartTime = 0;
};

class TraceEventInfo
{
public:
    explicit TraceEventInfo(PEVENT_RECORD pEvent)
        : pInfo(nullptr)
        , pEvent(pEvent) {
        unsigned long bufferSize = 0;
        auto result = TdhGetEventInformation(pEvent, 0, nullptr, nullptr, &bufferSize);
        if (result == ERROR_INSUFFICIENT_BUFFER) {
            pInfo = reinterpret_cast<TRACE_EVENT_INFO*>(operator new(bufferSize));
            result = TdhGetEventInformation(pEvent, 0, nullptr, pInfo, &bufferSize);
        }
        if (result != ERROR_SUCCESS) {
            throw std::exception("Unexpected error from TdhGetEventInformation.", result);
        }
    }
    TraceEventInfo(const TraceEventInfo&) = delete;
    TraceEventInfo& operator=(const TraceEventInfo&) = delete;
    ~TraceEventInfo() {
        operator delete(pInfo);
        pInfo = nullptr;
    }

    void GetData(PCWSTR name, byte* outData, uint32_t dataSize) {
        PROPERTY_DATA_DESCRIPTOR descriptor;
        descriptor.ArrayIndex = 0;
        descriptor.PropertyName = reinterpret_cast<unsigned long long>(name);
        auto result = TdhGetProperty(pEvent, 0, nullptr, 1, &descriptor, dataSize, outData);
        if (result != ERROR_SUCCESS) {
            throw std::exception("Unexpected error from TdhGetProperty.", result);
        }
    }

    uint32_t GetDataSize(PCWSTR name) {
        PROPERTY_DATA_DESCRIPTOR descriptor;
        descriptor.ArrayIndex = 0;
        descriptor.PropertyName = reinterpret_cast<unsigned long long>(name);
        ULONG size = 0;
        auto result = TdhGetPropertySize(pEvent, 0, nullptr, 1, &descriptor, &size);
        if (result != ERROR_SUCCESS) {
            throw std::exception("Unexpected error from TdhGetPropertySize.", result);
        }
        return size;
    }

    template <typename T>
    T GetData(PCWSTR name) {
        T local;
        GetData(name, reinterpret_cast<byte*>(&local), sizeof(local));
        return local;
    }

    uint64_t GetPtr(PCWSTR name) {
        if (pEvent->EventHeader.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER) {
            return GetData<uint32_t>(name);
        }
        else if (pEvent->EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER) {
            return GetData<uint64_t>(name);
        }
        return 0;
    }

private:
    TRACE_EVENT_INFO* pInfo;
    EVENT_RECORD* pEvent;
};

class MultiTraceConsumer : public ITraceConsumer
{
public:
    void AddTraceConsumer(ITraceConsumer* pConsumer) {
        if (pConsumer != nullptr) {
            mTraceConsumers.push_back(pConsumer);
        }
    }

    virtual void OnEventRecord(_In_ PEVENT_RECORD pEventRecord);
    virtual bool ContinueProcessing();

private:
    std::vector<ITraceConsumer*> mTraceConsumers;
};

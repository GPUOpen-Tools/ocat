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

#include "ProcessTraceConsumer.hpp"

void ProcessTraceConsumer::OnEventRecord(PEVENT_RECORD pEventRecord)
{
    if (mTraceStartTime == 0)
    {
        mTraceStartTime = pEventRecord->EventHeader.TimeStamp.QuadPart;
    }

    auto& hdr = pEventRecord->EventHeader;

    if (hdr.ProviderId == NT_PROCESS_EVENT_GUID)
    {
        OnNTProcessEvent(pEventRecord);
    }
}

void ProcessTraceConsumer::OnNTProcessEvent(PEVENT_RECORD pEventRecord)
{
    TraceEventInfo eventInfo(pEventRecord);
    auto pid = eventInfo.GetData<uint32_t>(L"ProcessId");
    auto lock = scoped_lock(mProcessMutex);
    switch (pEventRecord->EventHeader.EventDescriptor.Opcode)
    {
    case EVENT_TRACE_TYPE_START:
    case EVENT_TRACE_TYPE_DC_START:
    {
        ProcessInfo process;
        auto nameSize = eventInfo.GetDataSize(L"ImageFileName");
        process.mModuleName.resize(nameSize, '\0');
        eventInfo.GetData(L"ImageFileName", (byte*)process.mModuleName.data(), nameSize);

        mNewProcessesFromETW.insert_or_assign(pid, std::move(process));
        break;
    }
    case EVENT_TRACE_TYPE_END:
    case EVENT_TRACE_TYPE_DC_END:
    {
        mDeadProcessIds.emplace_back(pid);
        break;
    }
    }
}

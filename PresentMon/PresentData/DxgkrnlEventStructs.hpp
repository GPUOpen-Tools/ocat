#pragma once

// First, structures that can be produced either by processing legacy events
// or modern manifest-based events.

struct DxgkEventBase
{
    EVENT_HEADER const* pEventHeader;
};

struct DxgkBltEventArgs : DxgkEventBase
{
    uint64_t Hwnd;
    bool Present;
};

struct DxgkFlipEventArgs : DxgkEventBase
{
    int32_t FlipInterval;
    bool MMIO;
};

// A QueueSubmit can be many types, but these are interesting for present.
enum class DxgKrnl_QueueSubmit_Type {
    MMIOFlip = 3,
    Software = 7,
};
struct DxgkQueueSubmitEventArgs : DxgkEventBase
{
    DxgKrnl_QueueSubmit_Type PacketType;
    uint32_t SubmitSequence;
    uint64_t Context;
    bool Present;
    bool SupportsDxgkPresentEvent;
};

struct DxgkQueueCompleteEventArgs : DxgkEventBase
{
    uint32_t SubmitSequence;
};

enum DxgKrnl_MMIOFlip_Flags {
    FlipImmediate = 0x2,
    FlipOnNextVSync = 0x4
};
struct DxgkMMIOFlipEventArgs : DxgkEventBase
{
    uint32_t FlipSubmitSequence;
    DxgKrnl_MMIOFlip_Flags Flags;
};

struct DxgkVSyncDPCEventArgs : DxgkEventBase
{
    uint32_t FlipSubmitSequence;
};

struct DxgkSubmitPresentHistoryEventArgs : DxgkEventBase
{
    uint64_t Token;
    uint64_t TokenData;
    PresentMode KnownPresentMode;
};

struct DxgkPropagatePresentHistoryEventArgs : DxgkEventBase
{
    uint64_t Token;
};

struct DxgkRetirePresentHistoryEventArgs : DxgkEventBase
{
    uint64_t Token;
    PresentMode KnownPresentMode;
};

// These are the structures used to process the legacy events.

//
// Ensure that all ETW structure won't be aligned. This is necessary to
// guarantee that a user mode application trying to get to this data
// can get the same structure layout using a different compiler.
//
#pragma pack(push)
#pragma pack(1)

//
// Dxgkrnl Blt Event
//

typedef struct _DXGKETW_PRESENTFLAGS
{
    struct
    {
        UINT    Blt : 1;    // 0x00000001
        UINT    ColorFill : 1;    // 0x00000002
        UINT    Flip : 1;    // 0x00000004
        UINT    FlipWithNoWait : 1;    // 0x00000008
        UINT    SrcColorKey : 1;    // 0x00000010
        UINT    DstColorKey : 1;    // 0x00000020
        UINT    LinearToSrgb : 1;    // 0x00000040
        UINT    Rotate : 1;    // 0x00000080
        UINT    Reserved : 24;    // 0xFFFFFF00
    } Bits;
} DXGKETW_PRESENTFLAGS;

typedef struct _DXGKETW_BLTEVENT {
    ULONGLONG                  hwnd;
    ULONGLONG                  pDmaBuffer;
    ULONGLONG                  PresentHistoryToken;
    ULONGLONG                  hSourceAllocation;
    ULONGLONG                  hDestAllocation;
    BOOL                       bSubmit;
    BOOL                       bRedirectedPresent;
    DXGKETW_PRESENTFLAGS       Flags;
    RECT                       SourceRect;
    RECT                       DestRect;
    UINT                       SubRectCount; // followed by variable number of ETWGUID_DXGKBLTRECT events
} DXGKETW_BLTEVENT;

//
// Dxgkrnl Flip Event
//

typedef struct _DXGKETW_FLIPEVENT {
    ULONGLONG                  pDmaBuffer;
    ULONG                      VidPnSourceId;
    ULONGLONG                  FlipToAllocation;
    UINT                       FlipInterval; // D3DDDI_FLIPINTERVAL_TYPE
    BOOLEAN                    FlipWithNoWait;
    BOOLEAN                    MMIOFlip;
} DXGKETW_FLIPEVENT;

//
// Dxgkrnl Present History Event.
//

typedef enum _D3DKMT_PRESENT_MODEL
{
    D3DKMT_PM_UNINITIALIZED = 0,
    D3DKMT_PM_REDIRECTED_GDI = 1,
    D3DKMT_PM_REDIRECTED_FLIP = 2,
    D3DKMT_PM_REDIRECTED_BLT = 3,
    D3DKMT_PM_REDIRECTED_VISTABLT = 4,
    D3DKMT_PM_SCREENCAPTUREFENCE = 5,
    D3DKMT_PM_REDIRECTED_GDI_SYSMEM = 6,
    D3DKMT_PM_REDIRECTED_COMPOSITION = 7,
} D3DKMT_PRESENT_MODEL;

typedef struct _DXGKETW_PRESENTHISTORYEVENT {
    ULONGLONG             hAdapter;
    ULONGLONG             Token;
    D3DKMT_PRESENT_MODEL  Model;     // available only for _STOP event type.
    UINT                  TokenSize; // available only for _STOP event type.
} DXGKETW_PRESENTHISTORYEVENT;

//
// Dxgkrnl Scheduler Submit QueuePacket Event
//

typedef enum _DXGKETW_QUEUE_PACKET_TYPE {
    DXGKETW_RENDER_COMMAND_BUFFER = 0,
    DXGKETW_DEFERRED_COMMAND_BUFFER = 1,
    DXGKETW_SYSTEM_COMMAND_BUFFER = 2,
    DXGKETW_MMIOFLIP_COMMAND_BUFFER = 3,
    DXGKETW_WAIT_COMMAND_BUFFER = 4,
    DXGKETW_SIGNAL_COMMAND_BUFFER = 5,
    DXGKETW_DEVICE_COMMAND_BUFFER = 6,
    DXGKETW_SOFTWARE_COMMAND_BUFFER = 7,
    DXGKETW_PAGING_COMMAND_BUFFER = 8,
} DXGKETW_QUEUE_PACKET_TYPE;

typedef struct _DXGKETW_QUEUESUBMITEVENT {
    ULONGLONG                  hContext;
    DXGKETW_QUEUE_PACKET_TYPE  PacketType;
    ULONG                      SubmitSequence;
    ULONGLONG                  DmaBufferSize;
    UINT                       AllocationListSize;
    UINT                       PatchLocationListSize;
    BOOL                       bPresent;
    ULONGLONG                  hDmaBuffer;
} DXGKETW_QUEUESUBMITEVENT;

//
// Dxgkrnl Scheduler Complete QueuePacket Event
//

typedef struct _DXGKETW_QUEUECOMPLETEEVENT {
    ULONGLONG                  hContext;
    ULONG                      PacketType;
    ULONG                      SubmitSequence;
    union {
        BOOL                   bPreempted;
        BOOL                   bTimeouted; // PacketType is WaitCommandBuffer.
    };
} DXGKETW_QUEUECOMPLETEEVENT;

//
// Dxgkrnl VSync Dpc event
//

typedef enum _DXGKETW_FLIPMODE_TYPE
{
    DXGKETW_FLIPMODE_NO_DEVICE = 0,
    DXGKETW_FLIPMODE_IMMEDIATE = 1,
    DXGKETW_FLIPMODE_VSYNC_HW_FLIP_QUEUE = 2,
    DXGKETW_FLIPMODE_VSYNC_SW_FLIP_QUEUE = 3,
    DXGKETW_FLIPMODE_VSYNC_BUILT_IN_WAIT = 4,
    DXGKETW_FLIPMODE_IMMEDIATE_SW_FLIP_QUEUE = 5,
} DXGKETW_FLIPMODE_TYPE;

typedef LARGE_INTEGER PHYSICAL_ADDRESS;

typedef struct _DXGKETW_SCHEDULER_VSYNC_DPC {
    ULONGLONG                 pDxgAdapter;
    UINT                      VidPnTargetId;
    PHYSICAL_ADDRESS          ScannedPhysicalAddress;
    UINT                      VidPnSourceId;
    UINT                      FrameNumber;
    LONGLONG                  FrameQPCTime;
    ULONGLONG                 hFlipDevice;
    DXGKETW_FLIPMODE_TYPE     FlipType;
    union
    {
        ULARGE_INTEGER        FlipFenceId;
        PHYSICAL_ADDRESS      FlipToAddress;
    };
} DXGKETW_SCHEDULER_VSYNC_DPC;

//
// Dxgkrnl Scheduler mmio flip event
//

// there is 2 version of structure due to pointer size difference
// between x86 (32bits) and x64/ia64 (64bits).
//

typedef struct _DXGKETW_SCHEDULER_MMIO_FLIP_32 {
    ULONGLONG        pDxgAdapter;
    UINT             VidPnSourceId;
    ULONG            FlipSubmitSequence; // ContextUserSubmissionId
    UINT             FlipToDriverAllocation;
    PHYSICAL_ADDRESS FlipToPhysicalAddress;
    UINT             FlipToSegmentId;
    UINT             FlipPresentId;
    UINT             FlipPhysicalAdapterMask;
    ULONG            Flags;
} DXGKETW_SCHEDULER_MMIO_FLIP_32;

typedef struct _DXGKETW_SCHEDULER_MMIO_FLIP_64 {
    ULONGLONG        pDxgAdapter;
    UINT             VidPnSourceId;
    ULONG            FlipSubmitSequence; // ContextUserSubmissionId
    ULONGLONG        FlipToDriverAllocation;
    PHYSICAL_ADDRESS FlipToPhysicalAddress;
    UINT             FlipToSegmentId;
    UINT             FlipPresentId;
    UINT             FlipPhysicalAdapterMask;
    ULONG            Flags;
} DXGKETW_SCHEDULER_MMIO_FLIP_64;

#pragma pack(pop)

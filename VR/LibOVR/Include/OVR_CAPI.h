/************************************************************************************
 \file      OVR_CAPI.h
 \brief     C Interface to the Oculus PC SDK tracking and rendering library.
 \copyright Copyright 2014 Oculus VR, LLC All Rights reserved.
 ************************************************************************************/

// We don't use version numbers within OVR_CAPI_h, as all versioned variations
// of this file are currently mutually exclusive.
#ifndef OVR_CAPI_h
#define OVR_CAPI_h

#include "OVR_CAPI_Keys.h"
#include "OVR_Version.h"
#include "OVR_ErrorCode.h"

#if !defined(_WIN32)
#include <sys/types.h>
#endif


#include <stdint.h>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to __declspec(align())
#pragma warning(disable : 4359) // The alignment specified for a type is less than the
// alignment of the type of one of its data members
#endif

//-----------------------------------------------------------------------------------
// ***** OVR_OS
//
#if !defined(OVR_OS_WIN32) && defined(_WIN32)
#define OVR_OS_WIN32
#endif

#if !defined(OVR_OS_MAC) && defined(__APPLE__)
#define OVR_OS_MAC
#endif

#if !defined(OVR_OS_LINUX) && defined(__linux__)
#define OVR_OS_LINUX
#endif

//-----------------------------------------------------------------------------------
// ***** OVR_CPP
//
#if !defined(OVR_CPP)
#if defined(__cplusplus)
#define OVR_CPP(x) x
#else
#define OVR_CPP(x) /* Not C++ */
#endif
#endif

//-----------------------------------------------------------------------------------
// ***** OVR_CDECL
//
/// LibOVR calling convention for 32-bit Windows builds.
//
#if !defined(OVR_CDECL)
#if defined(_WIN32)
#define OVR_CDECL __cdecl
#else
#define OVR_CDECL
#endif
#endif

//-----------------------------------------------------------------------------------
// ***** OVR_EXTERN_C
//
/// Defined as extern "C" when built from C++ code.
//
#if !defined(OVR_EXTERN_C)
#ifdef __cplusplus
#define OVR_EXTERN_C extern "C"
#else
#define OVR_EXTERN_C
#endif
#endif

//-----------------------------------------------------------------------------------
// ***** OVR_PUBLIC_FUNCTION / OVR_PRIVATE_FUNCTION
//
// OVR_PUBLIC_FUNCTION  - Functions that externally visible from a shared library.
//                        Corresponds to Microsoft __dllexport.
// OVR_PUBLIC_CLASS     - C++ structs and classes that are externally visible from a
//                        shared library. Corresponds to Microsoft __dllexport.
// OVR_PRIVATE_FUNCTION - Functions that are not visible outside of a shared library.
//                        They are private to the shared library.
// OVR_PRIVATE_CLASS    - C++ structs and classes that are not visible outside of a
//                        shared library. They are private to the shared library.
//
// OVR_DLL_BUILD        - Used to indicate that the current compilation unit is of a shared library.
// OVR_DLL_IMPORT       - Used to indicate that the current compilation unit is a
//                        user of the corresponding shared library.
// OVR_STATIC_BUILD     - used to indicate that the current compilation unit is not a
//                        shared library but rather statically linked code.
//
#if !defined(OVR_PUBLIC_FUNCTION)
#if defined(OVR_DLL_BUILD)
#if defined(_WIN32)
#define OVR_PUBLIC_FUNCTION(rval) OVR_EXTERN_C __declspec(dllexport) rval OVR_CDECL
#define OVR_PUBLIC_CLASS __declspec(dllexport)
#define OVR_PRIVATE_FUNCTION(rval) rval OVR_CDECL
#define OVR_PRIVATE_CLASS
#else
#define OVR_PUBLIC_FUNCTION(rval) \
  OVR_EXTERN_C __attribute__((visibility("default"))) rval OVR_CDECL /* Requires GCC 4.0+ */
#define OVR_PUBLIC_CLASS __attribute__((visibility("default"))) /* Requires GCC 4.0+ */
#define OVR_PRIVATE_FUNCTION(rval) __attribute__((visibility("hidden"))) rval OVR_CDECL
#define OVR_PRIVATE_CLASS __attribute__((visibility("hidden")))
#endif
#elif defined(OVR_DLL_IMPORT)
#if defined(_WIN32)
#define OVR_PUBLIC_FUNCTION(rval) OVR_EXTERN_C __declspec(dllimport) rval OVR_CDECL
#define OVR_PUBLIC_CLASS __declspec(dllimport)
#else
#define OVR_PUBLIC_FUNCTION(rval) OVR_EXTERN_C rval OVR_CDECL
#define OVR_PUBLIC_CLASS
#endif
#define OVR_PRIVATE_FUNCTION(rval) rval OVR_CDECL
#define OVR_PRIVATE_CLASS
#else // OVR_STATIC_BUILD
#define OVR_PUBLIC_FUNCTION(rval) OVR_EXTERN_C rval OVR_CDECL
#define OVR_PUBLIC_CLASS
#define OVR_PRIVATE_FUNCTION(rval) rval OVR_CDECL
#define OVR_PRIVATE_CLASS
#endif
#endif

//-----------------------------------------------------------------------------------
// ***** OVR_EXPORT
//
/// Provided for backward compatibility with older versions of this library.
//
#if !defined(OVR_EXPORT)
#ifdef OVR_OS_WIN32
#define OVR_EXPORT __declspec(dllexport)
#else
#define OVR_EXPORT
#endif
#endif

//-----------------------------------------------------------------------------------
// ***** OVR_ALIGNAS
//
#if !defined(OVR_ALIGNAS)
#if defined(__GNUC__) || defined(__clang__)
#define OVR_ALIGNAS(n) __attribute__((aligned(n)))
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
#define OVR_ALIGNAS(n) __declspec(align(n))
#elif defined(__CC_ARM)
#define OVR_ALIGNAS(n) __align(n)
#else
#error Need to define OVR_ALIGNAS
#endif
#endif

//-----------------------------------------------------------------------------------
// ***** OVR_CC_HAS_FEATURE
//
// This is a portable way to use compile-time feature identification available
// with some compilers in a clean way. Direct usage of __has_feature in preprocessing
// statements of non-supporting compilers results in a preprocessing error.
//
// Example usage:
//     #if OVR_CC_HAS_FEATURE(is_pod)
//         if(__is_pod(T)) // If the type is plain data then we can safely memcpy it.
//             memcpy(&destObject, &srcObject, sizeof(object));
//     #endif
//
#if !defined(OVR_CC_HAS_FEATURE)
#if defined(__clang__) // http://clang.llvm.org/docs/LanguageExtensions.html#id2
#define OVR_CC_HAS_FEATURE(x) __has_feature(x)
#else
#define OVR_CC_HAS_FEATURE(x) 0
#endif
#endif

// ------------------------------------------------------------------------
// ***** OVR_STATIC_ASSERT
//
// Portable support for C++11 static_assert().
// Acts as if the following were declared:
//     void OVR_STATIC_ASSERT(bool const_expression, const char* msg);
//
// Example usage:
//     OVR_STATIC_ASSERT(sizeof(int32_t) == 4, "int32_t expected to be 4 bytes.");

#if !defined(OVR_STATIC_ASSERT)
#if !(defined(__cplusplus) && (__cplusplus >= 201103L)) /* Other */ && \
    !(defined(__GXX_EXPERIMENTAL_CXX0X__)) /* GCC */ &&                \
    !(defined(__clang__) && defined(__cplusplus) &&                    \
      OVR_CC_HAS_FEATURE(cxx_static_assert)) /* clang */               \
    && !(defined(_MSC_VER) && (_MSC_VER >= 1600) && defined(__cplusplus)) /* VS2010+  */

#if !defined(OVR_SA_UNUSED)
#if defined(OVR_CC_GNU) || defined(OVR_CC_CLANG)
#define OVR_SA_UNUSED __attribute__((unused))
#else
#define OVR_SA_UNUSED
#endif
#define OVR_SA_PASTE(a, b) a##b
#define OVR_SA_HELP(a, b) OVR_SA_PASTE(a, b)
#endif

#if defined(__COUNTER__)
#define OVR_STATIC_ASSERT(expression, msg) \
  typedef char OVR_SA_HELP(staticAssert, __COUNTER__)[((expression) != 0) ? 1 : -1] OVR_SA_UNUSED
#else
#define OVR_STATIC_ASSERT(expression, msg) \
  typedef char OVR_SA_HELP(staticAssert, __LINE__)[((expression) != 0) ? 1 : -1] OVR_SA_UNUSED
#endif

#else
#define OVR_STATIC_ASSERT(expression, msg) static_assert(expression, msg)
#endif
#endif

//-----------------------------------------------------------------------------------
// ***** Padding
//
/// Defines explicitly unused space for a struct.
/// When used correcly, usage of this macro should not change the size of the struct.
/// Compile-time and runtime behavior with and without this defined should be identical.
///
#if !defined(OVR_UNUSED_STRUCT_PAD)
#define OVR_UNUSED_STRUCT_PAD(padName, size) char padName[size];
#endif

//-----------------------------------------------------------------------------------
// ***** Word Size
//
/// Specifies the size of a pointer on the given platform.
///
#if !defined(OVR_PTR_SIZE)
#if defined(__WORDSIZE)
#define OVR_PTR_SIZE ((__WORDSIZE) / 8)
#elif defined(_WIN64) || defined(__LP64__) || defined(_LP64) || defined(_M_IA64) || \
    defined(__ia64__) || defined(__arch64__) || defined(__64BIT__) || defined(__Ptr_Is_64)
#define OVR_PTR_SIZE 8
#elif defined(__CC_ARM) && (__sizeof_ptr == 8)
#define OVR_PTR_SIZE 8
#else
#define OVR_PTR_SIZE 4
#endif
#endif

//-----------------------------------------------------------------------------------
// ***** OVR_ON32 / OVR_ON64
//
#if OVR_PTR_SIZE == 8
#define OVR_ON32(x)
#define OVR_ON64(x) x
#else
#define OVR_ON32(x) x
#define OVR_ON64(x)
#endif

//-----------------------------------------------------------------------------------
// ***** ovrBool

typedef char ovrBool; ///< Boolean type
#define ovrFalse 0 ///< ovrBool value of false.
#define ovrTrue 1 ///< ovrBool value of true.

//-----------------------------------------------------------------------------------
// ***** Simple Math Structures

/// A RGBA color with normalized float components.
typedef struct OVR_ALIGNAS(4) ovrColorf_ {
  float r, g, b, a;
} ovrColorf;

/// A 2D vector with integer components.
typedef struct OVR_ALIGNAS(4) ovrVector2i_ {
  int x, y;
} ovrVector2i;

/// A 2D size with integer components.
typedef struct OVR_ALIGNAS(4) ovrSizei_ {
  int w, h;
} ovrSizei;

/// A 2D rectangle with a position and size.
/// All components are integers.
typedef struct OVR_ALIGNAS(4) ovrRecti_ {
  ovrVector2i Pos;
  ovrSizei Size;
} ovrRecti;

/// A quaternion rotation.
typedef struct OVR_ALIGNAS(4) ovrQuatf_ {
  float x, y, z, w;
} ovrQuatf;

/// A 2D vector with float components.
typedef struct OVR_ALIGNAS(4) ovrVector2f_ {
  float x, y;
} ovrVector2f;

/// A 3D vector with float components.
typedef struct OVR_ALIGNAS(4) ovrVector3f_ {
  float x, y, z;
} ovrVector3f;

/// A 4x4 matrix with float elements.
typedef struct OVR_ALIGNAS(4) ovrMatrix4f_ {
  float M[4][4];
} ovrMatrix4f;

/// Position and orientation together.
/// The coordinate system used is right-handed Cartesian.
typedef struct OVR_ALIGNAS(4) ovrPosef_ {
  ovrQuatf Orientation;
  ovrVector3f Position;
} ovrPosef;

/// A full pose (rigid body) configuration with first and second derivatives.
///
/// Body refers to any object for which ovrPoseStatef is providing data.
/// It can be the HMD, Touch controller, sensor or something else. The context
/// depends on the usage of the struct.
typedef struct OVR_ALIGNAS(8) ovrPoseStatef_ {
  ovrPosef ThePose; ///< Position and orientation.
  ovrVector3f AngularVelocity; ///< Angular velocity in radians per second.
  ovrVector3f LinearVelocity; ///< Velocity in meters per second.
  ovrVector3f AngularAcceleration; ///< Angular acceleration in radians per second per second.
  ovrVector3f LinearAcceleration; ///< Acceleration in meters per second per second.
  OVR_UNUSED_STRUCT_PAD(pad0, 4) ///< \internal struct pad.
  double TimeInSeconds; ///< Absolute time that this pose refers to. \see ovr_GetTimeInSeconds
} ovrPoseStatef;

/// Describes the up, down, left, and right angles of the field of view.
///
/// Field Of View (FOV) tangent of the angle units.
/// \note For a standard 90 degree vertical FOV, we would
/// have: { UpTan = tan(90 degrees / 2), DownTan = tan(90 degrees / 2) }.
typedef struct OVR_ALIGNAS(4) ovrFovPort_ {
  float UpTan; ///< Tangent of the angle between the viewing vector and top edge of the FOV.
  float DownTan; ///< Tangent of the angle between the viewing vector and bottom edge of the FOV.
  float LeftTan; ///< Tangent of the angle between the viewing vector and left edge of the FOV.
  float RightTan; ///< Tangent of the angle between the viewing vector and right edge of the FOV.
} ovrFovPort;

//-----------------------------------------------------------------------------------
// ***** HMD Types

/// Enumerates all HMD types that we support.
///
/// The currently released developer kits are ovrHmd_DK1 and ovrHmd_DK2.
/// The other enumerations are for internal use only.
typedef enum ovrHmdType_ {
  ovrHmd_None = 0,
  ovrHmd_DK1 = 3,
  ovrHmd_DKHD = 4,
  ovrHmd_DK2 = 6,
  ovrHmd_CB = 8,
  ovrHmd_Other = 9,
  ovrHmd_E3_2015 = 10,
  ovrHmd_ES06 = 11,
  ovrHmd_ES09 = 12,
  ovrHmd_ES11 = 13,
  ovrHmd_CV1 = 14,

  ovrHmd_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrHmdType;

/// HMD capability bits reported by device.
///
typedef enum ovrHmdCaps_ {
  // Read-only flags

  /// <B>(read only)</B> Specifies that the HMD is a virtual debug device.
  ovrHmdCap_DebugDevice = 0x0010,


  ovrHmdCap_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrHmdCaps;

/// Tracking capability bits reported by the device.
/// Used with ovr_GetTrackingCaps.
typedef enum ovrTrackingCaps_ {
  ovrTrackingCap_Orientation = 0x0010, ///< Supports orientation tracking (IMU).
  ovrTrackingCap_MagYawCorrection = 0x0020, ///< Supports yaw drift correction.
  ovrTrackingCap_Position = 0x0040, ///< Supports positional tracking.
  ovrTrackingCap_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrTrackingCaps;

/// Optional extensions
typedef enum ovrExtensions_ {
  ovrExtension_TextureLayout_Octilinear = 0, ///< Enable before first layer submission.
  ovrExtension_Count, ///< \internal Sanity checking
  ovrExtension_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrExtensions;

/// Specifies which eye is being used for rendering.
/// This type explicitly does not include a third "NoStereo" monoscopic option,
/// as such is not required for an HMD-centered API.
typedef enum ovrEyeType_ {
  ovrEye_Left = 0, ///< The left eye, from the viewer's perspective.
  ovrEye_Right = 1, ///< The right eye, from the viewer's perspective.
  ovrEye_Count = 2, ///< \internal Count of enumerated elements.
  ovrEye_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrEyeType;

/// Specifies the coordinate system ovrTrackingState returns tracking poses in.
/// Used with ovr_SetTrackingOriginType()
typedef enum ovrTrackingOrigin_ {
  /// \brief Tracking system origin reported at eye (HMD) height
  /// \details Prefer using this origin when your application requires
  /// matching user's current physical head pose to a virtual head pose
  /// without any regards to a the height of the floor. Cockpit-based,
  /// or 3rd-person experiences are ideal candidates.
  /// When used, all poses in ovrTrackingState are reported as an offset
  /// transform from the profile calibrated or recentered HMD pose.
  /// It is recommended that apps using this origin type call ovr_RecenterTrackingOrigin
  /// prior to starting the VR experience, but notify the user before doing so
  /// to make sure the user is in a comfortable pose, facing a comfortable
  /// direction.
  ovrTrackingOrigin_EyeLevel = 0,

  /// \brief Tracking system origin reported at floor height
  /// \details Prefer using this origin when your application requires the
  /// physical floor height to match the virtual floor height, such as
  /// standing experiences.
  /// When used, all poses in ovrTrackingState are reported as an offset
  /// transform from the profile calibrated floor pose. Calling ovr_RecenterTrackingOrigin
  /// will recenter the X & Z axes as well as yaw, but the Y-axis (i.e. height) will continue
  /// to be reported using the floor height as the origin for all poses.
  ovrTrackingOrigin_FloorLevel = 1,

  ovrTrackingOrigin_Count = 2, ///< \internal Count of enumerated elements.
  ovrTrackingOrigin_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrTrackingOrigin;

/// Identifies a graphics device in a platform-specific way.
/// For Windows this is a LUID type.
typedef struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrGraphicsLuid_ {
  // Public definition reserves space for graphics API-specific implementation
  char Reserved[8];
} ovrGraphicsLuid;

/// This is a complete descriptor of the HMD.
typedef struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrHmdDesc_ {
  ovrHmdType Type; ///< The type of HMD.
  OVR_ON64(OVR_UNUSED_STRUCT_PAD(pad0, 4)) ///< \internal struct paddding.
  char ProductName[64]; ///< UTF8-encoded product identification string (e.g. "Oculus Rift DK1").
  char Manufacturer[64]; ///< UTF8-encoded HMD manufacturer identification string.
  short VendorId; ///< HID (USB) vendor identifier of the device.
  short ProductId; ///< HID (USB) product identifier of the device.
  char SerialNumber[24]; ///< HMD serial number.
  short FirmwareMajor; ///< HMD firmware major version.
  short FirmwareMinor; ///< HMD firmware minor version.
  unsigned int AvailableHmdCaps; ///< Available ovrHmdCaps bits.
  unsigned int DefaultHmdCaps; ///< Default ovrHmdCaps bits.
  unsigned int AvailableTrackingCaps; ///< Available ovrTrackingCaps bits.
  unsigned int DefaultTrackingCaps; ///< Default ovrTrackingCaps bits.
  ovrFovPort DefaultEyeFov[ovrEye_Count]; ///< Defines the recommended FOVs for the HMD.
  ovrFovPort MaxEyeFov[ovrEye_Count]; ///< Defines the maximum FOVs for the HMD.
  ovrSizei Resolution; ///< Resolution of the full HMD screen (both eyes) in pixels.
  float DisplayRefreshRate; ///< Refresh rate of the display in cycles per second.
  OVR_ON64(OVR_UNUSED_STRUCT_PAD(pad1, 4)) ///< \internal struct paddding.
} ovrHmdDesc;

/// Used as an opaque pointer to an OVR session.
typedef struct ovrHmdStruct* ovrSession;

#ifdef OVR_OS_WIN32
typedef uint32_t ovrProcessId;
#else
typedef pid_t ovrProcessId;
#endif

/// Fallback definitions for when the vulkan header isn't being included
#if !defined(VK_VERSION_1_0)
// From <vulkan/vulkan.h>:
#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
#if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__)) || \
    defined(_M_X64) || defined(__ia64) || defined(_M_IA64) || defined(__aarch64__) ||       \
    defined(__powerpc64__)
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T* object;
#else
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;
#endif
VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_HANDLE(VkPhysicalDevice)
VK_DEFINE_HANDLE(VkDevice)
VK_DEFINE_HANDLE(VkQueue)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkImage)
#endif

/// Bit flags describing the current status of sensor tracking.
///  The values must be the same as in enum StatusBits
///
/// \see ovrTrackingState
///
typedef enum ovrStatusBits_ {
  ovrStatus_OrientationTracked = 0x0001, ///< Orientation is currently tracked (connected & in use).
  ovrStatus_PositionTracked = 0x0002, ///< Position is currently tracked (false if out of range).
  ovrStatus_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrStatusBits;

///  Specifies the description of a single sensor.
///
/// \see ovr_GetTrackerDesc
///
typedef struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrTrackerDesc_ {
  float FrustumHFovInRadians; ///< Sensor frustum horizontal field-of-view (if present).
  float FrustumVFovInRadians; ///< Sensor frustum vertical field-of-view (if present).
  float FrustumNearZInMeters; ///< Sensor frustum near Z (if present).
  float FrustumFarZInMeters; ///< Sensor frustum far Z (if present).
} ovrTrackerDesc;

///  Specifies sensor flags.
///
///  /see ovrTrackerPose
///
typedef enum ovrTrackerFlags_ {
  /// The sensor is present, else the sensor is absent or offline.
  ovrTracker_Connected = 0x0020,

  /// The sensor has a valid pose, else the pose is unavailable.
  /// This will only be set if ovrTracker_Connected is set.
  ovrTracker_PoseTracked = 0x0004
} ovrTrackerFlags;

///  Specifies the pose for a single sensor.
///
typedef struct OVR_ALIGNAS(8) _ovrTrackerPose {
  /// ovrTrackerFlags.
  unsigned int TrackerFlags;

  /// The sensor's pose. This pose includes sensor tilt (roll and pitch).
  /// For a leveled coordinate system use LeveledPose.
  ovrPosef Pose;

  /// The sensor's leveled pose, aligned with gravity. This value includes pos and yaw of the
  /// sensor, but not roll and pitch. It can be used as a reference point to render real-world
  /// objects in the correct location.
  ovrPosef LeveledPose;

  OVR_UNUSED_STRUCT_PAD(pad0, 4) ///< \internal struct pad.
} ovrTrackerPose;

/// Tracking state at a given absolute time (describes predicted HMD pose, etc.).
/// Returned by ovr_GetTrackingState.
///
/// \see ovr_GetTrackingState
///
typedef struct OVR_ALIGNAS(8) ovrTrackingState_ {
  /// Predicted head pose (and derivatives) at the requested absolute time.
  ovrPoseStatef HeadPose;

  /// HeadPose tracking status described by ovrStatusBits.
  unsigned int StatusFlags;

  /// The most recent calculated pose for each hand when hand controller tracking is present.
  /// HandPoses[ovrHand_Left] refers to the left hand and HandPoses[ovrHand_Right] to the right.
  /// These values can be combined with ovrInputState for complete hand controller information.
  ovrPoseStatef HandPoses[2];

  /// HandPoses status flags described by ovrStatusBits.
  /// Only ovrStatus_OrientationTracked and ovrStatus_PositionTracked are reported.
  unsigned int HandStatusFlags[2];

  /// The pose of the origin captured during calibration.
  /// Like all other poses here, this is expressed in the space set by ovr_RecenterTrackingOrigin,
  /// or ovr_SpecifyTrackingOrigin and so will change every time either of those functions are
  /// called. This pose can be used to calculate where the calibrated origin lands in the new
  /// recentered space. If an application never calls ovr_RecenterTrackingOrigin or
  /// ovr_SpecifyTrackingOrigin, expect this value to be the identity pose and as such will point
  /// respective origin based on ovrTrackingOrigin requested when calling ovr_GetTrackingState.
  ovrPosef CalibratedOrigin;

} ovrTrackingState;



/// Rendering information for each eye. Computed by ovr_GetRenderDesc() based on the
/// specified FOV. Note that the rendering viewport is not included
/// here as it can be specified separately and modified per frame by
/// passing different Viewport values in the layer structure.
///
/// \see ovr_GetRenderDesc
///
typedef struct OVR_ALIGNAS(4) ovrEyeRenderDesc_ {
  ovrEyeType Eye; ///< The eye index to which this instance corresponds.
  ovrFovPort Fov; ///< The field of view.
  ovrRecti DistortedViewport; ///< Distortion viewport.
  ovrVector2f PixelsPerTanAngleAtCenter; ///< How many display pixels will fit in tan(angle) = 1.
  ovrPosef HmdToEyePose; ///< Transform of eye from the HMD center, in meters.
} ovrEyeRenderDesc;

/// Projection information for ovrLayerEyeFovDepth.
///
/// Use the utility function ovrTimewarpProjectionDesc_FromProjection to
/// generate this structure from the application's projection matrix.
///
/// \see ovrLayerEyeFovDepth, ovrTimewarpProjectionDesc_FromProjection
///
typedef struct OVR_ALIGNAS(4) ovrTimewarpProjectionDesc_ {
  float Projection22; ///< Projection matrix element [2][2].
  float Projection23; ///< Projection matrix element [2][3].
  float Projection32; ///< Projection matrix element [3][2].
} ovrTimewarpProjectionDesc;


/// Contains the data necessary to properly calculate position info for various layer types.
/// - HmdToEyePose is the same value-pair provided in ovrEyeRenderDesc. Modifying this value is
///   suggested only if the app is forcing monoscopic rendering and requires that all layers
///   including quad layers show up in a monoscopic fashion.
/// - HmdSpaceToWorldScaleInMeters is used to scale player motion into in-application units.
///   In other words, it is how big an in-application unit is in the player's physical meters.
///   For example, if the application uses inches as its units then HmdSpaceToWorldScaleInMeters
///   would be 0.0254.
///   Note that if you are scaling the player in size, this must also scale. So if your application
///   units are inches, but you're shrinking the player to half their normal size, then
///   HmdSpaceToWorldScaleInMeters would be 0.0254*2.0.
///
/// \see ovrEyeRenderDesc, ovr_SubmitFrame
///
typedef struct OVR_ALIGNAS(4) ovrViewScaleDesc_ {
  ovrPosef HmdToEyePose[ovrEye_Count]; ///< Transform of each eye from the HMD center, in meters.
  float HmdSpaceToWorldScaleInMeters; ///< Ratio of viewer units to meter units.
} ovrViewScaleDesc;

//-----------------------------------------------------------------------------------
// ***** Platform-independent Rendering Configuration

/// The type of texture resource.
///
/// \see ovrTextureSwapChainDesc
///
typedef enum ovrTextureType_ {
  ovrTexture_2D, ///< 2D textures.
  ovrTexture_2D_External, ///< Application-provided 2D texture. Not supported on PC.
  ovrTexture_Cube, ///< Cube maps. ovrTextureSwapChainDesc::ArraySize must be 6 for this type.
  ovrTexture_Count,
  ovrTexture_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrTextureType;

/// The bindings required for texture swap chain.
///
/// All texture swap chains are automatically bindable as shader
/// input resources since the Oculus runtime needs this to read them.
///
/// \see ovrTextureSwapChainDesc
///
typedef enum ovrTextureBindFlags_ {
  ovrTextureBind_None,

  /// The application can write into the chain with pixel shader.
  ovrTextureBind_DX_RenderTarget = 0x0001,

  /// The application can write to the chain with compute shader.
  ovrTextureBind_DX_UnorderedAccess = 0x0002,

  /// The chain buffers can be bound as depth and/or stencil buffers.
  /// This flag cannot be combined with ovrTextureBind_DX_RenderTarget.
  ovrTextureBind_DX_DepthStencil = 0x0004,

  ovrTextureBind_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrTextureBindFlags;

/// The format of a texture.
///
/// \see ovrTextureSwapChainDesc
///
typedef enum ovrTextureFormat_ {
  OVR_FORMAT_UNKNOWN = 0,
  OVR_FORMAT_B5G6R5_UNORM = 1, ///< Not currently supported on PC. Requires a DirectX 11.1 device.
  OVR_FORMAT_B5G5R5A1_UNORM = 2, ///< Not currently supported on PC. Requires a DirectX 11.1 device.
  OVR_FORMAT_B4G4R4A4_UNORM = 3, ///< Not currently supported on PC. Requires a DirectX 11.1 device.
  OVR_FORMAT_R8G8B8A8_UNORM = 4,
  OVR_FORMAT_R8G8B8A8_UNORM_SRGB = 5,
  OVR_FORMAT_B8G8R8A8_UNORM = 6,
  OVR_FORMAT_B8G8R8A8_UNORM_SRGB = 7, ///< Not supported for OpenGL applications
  OVR_FORMAT_B8G8R8X8_UNORM = 8, ///< Not supported for OpenGL applications
  OVR_FORMAT_B8G8R8X8_UNORM_SRGB = 9, ///< Not supported for OpenGL applications
  OVR_FORMAT_R16G16B16A16_FLOAT = 10,
  OVR_FORMAT_R11G11B10_FLOAT = 25, ///< Introduced in v1.10

  // Depth formats
  OVR_FORMAT_D16_UNORM = 11,
  OVR_FORMAT_D24_UNORM_S8_UINT = 12,
  OVR_FORMAT_D32_FLOAT = 13,
  OVR_FORMAT_D32_FLOAT_S8X24_UINT = 14,

  // Added in 1.5 compressed formats can be used for static layers
  OVR_FORMAT_BC1_UNORM = 15,
  OVR_FORMAT_BC1_UNORM_SRGB = 16,
  OVR_FORMAT_BC2_UNORM = 17,
  OVR_FORMAT_BC2_UNORM_SRGB = 18,
  OVR_FORMAT_BC3_UNORM = 19,
  OVR_FORMAT_BC3_UNORM_SRGB = 20,
  OVR_FORMAT_BC6H_UF16 = 21,
  OVR_FORMAT_BC6H_SF16 = 22,
  OVR_FORMAT_BC7_UNORM = 23,
  OVR_FORMAT_BC7_UNORM_SRGB = 24,


  OVR_FORMAT_ENUMSIZE = 0x7fffffff ///< \internal Force type int32_t.
} ovrTextureFormat;

/// Misc flags overriding particular
///   behaviors of a texture swap chain
///
/// \see ovrTextureSwapChainDesc
///
typedef enum ovrTextureMiscFlags_ {
  ovrTextureMisc_None,

  /// Vulkan and DX only: The underlying texture is created with a TYPELESS equivalent
  /// of the format specified in the texture desc. The SDK will still access the
  /// texture using the format specified in the texture desc, but the app can
  /// create views with different formats if this is specified.
  ovrTextureMisc_DX_Typeless = 0x0001,

  /// DX only: Allow generation of the mip chain on the GPU via the GenerateMips
  /// call. This flag requires that RenderTarget binding also be specified.
  ovrTextureMisc_AllowGenerateMips = 0x0002,

  /// Texture swap chain contains protected content, and requires
  /// HDCP connection in order to display to HMD. Also prevents
  /// mirroring or other redirection of any frame containing this contents
  ovrTextureMisc_ProtectedContent = 0x0004,

  /// Automatically generate and use the mip chain in composition on each submission.
  /// Mips are regenerated from highest quality level, ignoring other pre-existing mip levels.
  /// Not supported for depth or compressed (BC) formats.
  ovrTextureMisc_AutoGenerateMips = 0x0008,

  ovrTextureMisc_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrTextureFlags;

/// Description used to create a texture swap chain.
///
/// \see ovr_CreateTextureSwapChainDX
/// \see ovr_CreateTextureSwapChainGL
///
typedef struct ovrTextureSwapChainDesc_ {
  ovrTextureType Type; ///< Must not be ovrTexture_Window
  ovrTextureFormat Format;
  int ArraySize; ///< Must be 6 for ovrTexture_Cube, 1 for other types.
  int Width;
  int Height;
  int MipLevels;
  int SampleCount; ///< Only supported with depth textures.
  ovrBool StaticImage; ///< Not buffered in a chain. For images that don't change
  unsigned int MiscFlags; ///< ovrTextureFlags
  unsigned int BindFlags; ///< ovrTextureBindFlags. Not used for GL.
} ovrTextureSwapChainDesc;

/// Bit flags used as part of ovrMirrorTextureDesc's MirrorOptions field.
///
/// \see ovr_CreateMirrorTextureWithOptionsDX
/// \see ovr_CreateMirrorTextureWithOptionsGL
/// \see ovr_CreateMirrorTextureWithOptionsVk
///
typedef enum ovrMirrorOptions_ {
  /// By default the mirror texture will be:
  /// * Pre-distortion (i.e. rectilinear)
  /// * Contain both eye textures
  /// * Exclude Guardian, Notifications, System Menu GUI
  ovrMirrorOption_Default = 0x0000,

  /// Retrieves the barrel distorted texture contents instead of the rectilinear one
  /// This is only recommended for debugging purposes, and not for final desktop presentation
  ovrMirrorOption_PostDistortion = 0x0001,

  /// Since ovrMirrorOption_Default renders both eyes into the mirror texture,
  /// these two flags are exclusive (i.e. cannot use them simultaneously)
  ovrMirrorOption_LeftEyeOnly = 0x0002,
  ovrMirrorOption_RightEyeOnly = 0x0004,

  /// Shows the boundary system aka Guardian on the mirror texture
  ovrMirrorOption_IncludeGuardian = 0x0008,

  /// Shows system notifications the user receives on the mirror texture
  ovrMirrorOption_IncludeNotifications = 0x0010,

  /// Shows the system menu (triggered by hitting the Home button) on the mirror texture
  ovrMirrorOption_IncludeSystemGui = 0x0020,


  ovrMirrorOption_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrMirrorOptions;

/// Description used to create a mirror texture.
///
/// \see ovr_CreateMirrorTextureWithOptionsDX
/// \see ovr_CreateMirrorTextureWithOptionsGL
/// \see ovr_CreateMirrorTextureWithOptionsVk
///
typedef struct ovrMirrorTextureDesc_ {
  ovrTextureFormat Format;
  int Width;
  int Height;
  unsigned int MiscFlags; ///< ovrTextureFlags
  unsigned int MirrorOptions; ///< ovrMirrorOptions
} ovrMirrorTextureDesc;

typedef struct ovrTextureSwapChainData* ovrTextureSwapChain;
typedef struct ovrMirrorTextureData* ovrMirrorTexture;

//-----------------------------------------------------------------------------------

/// Describes button input types.
/// Button inputs are combined; that is they will be reported as pressed if they are
/// pressed on either one of the two devices.
/// The ovrButton_Up/Down/Left/Right map to both XBox D-Pad and directional buttons.
/// The ovrButton_Enter and ovrButton_Return map to Start and Back controller buttons, respectively.
typedef enum ovrButton_ {
  /// A button on XBox controllers and right Touch controller. Select button on Oculus Remote.
  ovrButton_A = 0x00000001,

  /// B button on XBox controllers and right Touch controller. Back button on Oculus Remote.
  ovrButton_B = 0x00000002,

  /// Right thumbstick on XBox controllers and Touch controllers. Not present on Oculus Remote.
  ovrButton_RThumb = 0x00000004,

  /// Right shoulder button on XBox controllers. Not present on Touch controllers or Oculus Remote.
  ovrButton_RShoulder = 0x00000008,


  /// X button on XBox controllers and left Touch controller. Not present on Oculus Remote.
  ovrButton_X = 0x00000100,

  /// Y button on XBox controllers and left Touch controller. Not present on Oculus Remote.
  ovrButton_Y = 0x00000200,

  /// Left thumbstick on XBox controllers and Touch controllers. Not present on Oculus Remote.
  ovrButton_LThumb = 0x00000400,

  /// Left shoulder button on XBox controllers. Not present on Touch controllers or Oculus Remote.
  ovrButton_LShoulder = 0x00000800,

  /// Up button on XBox controllers and Oculus Remote. Not present on Touch controllers.
  ovrButton_Up = 0x00010000,

  /// Down button on XBox controllers and Oculus Remote. Not present on Touch controllers.
  ovrButton_Down = 0x00020000,

  /// Left button on XBox controllers and Oculus Remote. Not present on Touch controllers.
  ovrButton_Left = 0x00040000,

  /// Right button on XBox controllers and Oculus Remote. Not present on Touch controllers.
  ovrButton_Right = 0x00080000,

  /// Start on XBox 360 controller. Menu on XBox One controller and Left Touch controller.
  /// Should be referred to as the Menu button in user-facing documentation.
  ovrButton_Enter = 0x00100000,

  /// Back on Xbox 360 controller. View button on XBox One controller. Not present on Touch
  /// controllers or Oculus Remote.
  ovrButton_Back = 0x00200000,

  /// Volume button on Oculus Remote. Not present on XBox or Touch controllers.
  ovrButton_VolUp = 0x00400000,

  /// Volume button on Oculus Remote. Not present on XBox or Touch controllers.
  ovrButton_VolDown = 0x00800000,

  /// Home button on XBox controllers. Oculus button on Touch controllers and Oculus Remote.
  ovrButton_Home = 0x01000000,

  // Bit mask of all buttons that are for private usage by Oculus
  ovrButton_Private = ovrButton_VolUp | ovrButton_VolDown | ovrButton_Home,

  // Bit mask of all buttons on the right Touch controller
  ovrButton_RMask = ovrButton_A | ovrButton_B | ovrButton_RThumb | ovrButton_RShoulder,

  // Bit mask of all buttons on the left Touch controller
  ovrButton_LMask =
      ovrButton_X | ovrButton_Y | ovrButton_LThumb | ovrButton_LShoulder | ovrButton_Enter,

  ovrButton_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrButton;

/// Describes touch input types.
/// These values map to capacitive touch values reported ovrInputState::Touch.
/// Some of these values are mapped to button bits for consistency.
typedef enum ovrTouch_ {
  ovrTouch_A = ovrButton_A,
  ovrTouch_B = ovrButton_B,
  ovrTouch_RThumb = ovrButton_RThumb,
  ovrTouch_RThumbRest = 0x00000008,
  ovrTouch_RIndexTrigger = 0x00000010,

  // Bit mask of all the button touches on the right controller
  ovrTouch_RButtonMask =
      ovrTouch_A | ovrTouch_B | ovrTouch_RThumb | ovrTouch_RThumbRest | ovrTouch_RIndexTrigger,

  ovrTouch_X = ovrButton_X,
  ovrTouch_Y = ovrButton_Y,
  ovrTouch_LThumb = ovrButton_LThumb,
  ovrTouch_LThumbRest = 0x00000800,
  ovrTouch_LIndexTrigger = 0x00001000,

  // Bit mask of all the button touches on the left controller
  ovrTouch_LButtonMask =
      ovrTouch_X | ovrTouch_Y | ovrTouch_LThumb | ovrTouch_LThumbRest | ovrTouch_LIndexTrigger,

  // Finger pose state
  // Derived internally based on distance, proximity to sensors and filtering.
  ovrTouch_RIndexPointing = 0x00000020,
  ovrTouch_RThumbUp = 0x00000040,
  ovrTouch_LIndexPointing = 0x00002000,
  ovrTouch_LThumbUp = 0x00004000,

  // Bit mask of all right controller poses
  ovrTouch_RPoseMask = ovrTouch_RIndexPointing | ovrTouch_RThumbUp,

  // Bit mask of all left controller poses
  ovrTouch_LPoseMask = ovrTouch_LIndexPointing | ovrTouch_LThumbUp,

  ovrTouch_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrTouch;

/// Describes the Touch Haptics engine.
/// Currently, those values will NOT change during a session.
typedef struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrTouchHapticsDesc_ {
  // Haptics engine frequency/sample-rate, sample time in seconds equals 1.0/sampleRateHz
  int SampleRateHz;
  // Size of each Haptics sample, sample value range is [0, 2^(Bytes*8)-1]
  int SampleSizeInBytes;

  // Queue size that would guarantee Haptics engine would not starve for data
  // Make sure size doesn't drop below it for best results
  int QueueMinSizeToAvoidStarvation;

  // Minimum, Maximum and Optimal number of samples that can be sent to Haptics through
  // ovr_SubmitControllerVibration
  int SubmitMinSamples;
  int SubmitMaxSamples;
  int SubmitOptimalSamples;
} ovrTouchHapticsDesc;

/// Specifies which controller is connected; multiple can be connected at once.
typedef enum ovrControllerType_ {
  ovrControllerType_None = 0x0000,
  ovrControllerType_LTouch = 0x0001,
  ovrControllerType_RTouch = 0x0002,
  ovrControllerType_Touch = (ovrControllerType_LTouch | ovrControllerType_RTouch),
  ovrControllerType_Remote = 0x0004,

  ovrControllerType_XBox = 0x0010,

  ovrControllerType_Object0 = 0x0100,
  ovrControllerType_Object1 = 0x0200,
  ovrControllerType_Object2 = 0x0400,
  ovrControllerType_Object3 = 0x0800,

  ovrControllerType_Active = 0xffffffff, ///< Operate on or query whichever controller is active.

  ovrControllerType_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrControllerType;

/// Haptics buffer submit mode
typedef enum ovrHapticsBufferSubmitMode_ {
  /// Enqueue buffer for later playback
  ovrHapticsBufferSubmit_Enqueue
} ovrHapticsBufferSubmitMode;

/// Maximum number of samples in ovrHapticsBuffer
#define OVR_HAPTICS_BUFFER_SAMPLES_MAX 256

/// Haptics buffer descriptor, contains amplitude samples used for Touch vibration
typedef struct ovrHapticsBuffer_ {
  /// Samples stored in opaque format
  const void* Samples;
  /// Number of samples (up to OVR_HAPTICS_BUFFER_SAMPLES_MAX)
  int SamplesCount;
  /// How samples are submitted to the hardware
  ovrHapticsBufferSubmitMode SubmitMode;
} ovrHapticsBuffer;

/// State of the Haptics playback for Touch vibration
typedef struct ovrHapticsPlaybackState_ {
  // Remaining space available to queue more samples
  int RemainingQueueSpace;

  // Number of samples currently queued
  int SamplesQueued;
} ovrHapticsPlaybackState;

/// Position tracked devices
typedef enum ovrTrackedDeviceType_ {
  ovrTrackedDevice_None = 0x0000,
  ovrTrackedDevice_HMD = 0x0001,
  ovrTrackedDevice_LTouch = 0x0002,
  ovrTrackedDevice_RTouch = 0x0004,
  ovrTrackedDevice_Touch = (ovrTrackedDevice_LTouch | ovrTrackedDevice_RTouch),

  ovrTrackedDevice_Object0 = 0x0010,
  ovrTrackedDevice_Object1 = 0x0020,
  ovrTrackedDevice_Object2 = 0x0040,
  ovrTrackedDevice_Object3 = 0x0080,

  ovrTrackedDevice_All = 0xFFFF,
} ovrTrackedDeviceType;

/// Boundary types that specified while using the boundary system
typedef enum ovrBoundaryType_ {
  /// Outer boundary - closely represents user setup walls
  ovrBoundary_Outer = 0x0001,

  /// Play area - safe rectangular area inside outer boundary which can optionally be used to
  /// restrict user interactions and motion.
  ovrBoundary_PlayArea = 0x0100,
} ovrBoundaryType;

/// Boundary system look and feel
typedef struct ovrBoundaryLookAndFeel_ {
  /// Boundary color (alpha channel is ignored)
  ovrColorf Color;
} ovrBoundaryLookAndFeel;

/// Provides boundary test information
typedef struct ovrBoundaryTestResult_ {
  /// True if the boundary system is being triggered. Note that due to fade in/out effects this may
  /// not exactly match visibility.
  ovrBool IsTriggering;

  /// Distance to the closest play area or outer boundary surface.
  float ClosestDistance;

  /// Closest point on the boundary surface.
  ovrVector3f ClosestPoint;

  /// Unit surface normal of the closest boundary surface.
  ovrVector3f ClosestPointNormal;
} ovrBoundaryTestResult;

/// Provides names for the left and right hand array indexes.
///
/// \see ovrInputState, ovrTrackingState
///
typedef enum ovrHandType_ {
  ovrHand_Left = 0,
  ovrHand_Right = 1,
  ovrHand_Count = 2,
  ovrHand_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrHandType;

/// ovrInputState describes the complete controller input state, including Oculus Touch,
/// and XBox gamepad. If multiple inputs are connected and used at the same time,
/// their inputs are combined.
typedef struct ovrInputState_ {
  /// System type when the controller state was last updated.
  double TimeInSeconds;

  /// Values for buttons described by ovrButton.
  unsigned int Buttons;

  /// Touch values for buttons and sensors as described by ovrTouch.
  unsigned int Touches;

  /// Left and right finger trigger values (ovrHand_Left and ovrHand_Right), in range 0.0 to 1.0f.
  /// Returns 0 if the value would otherwise be less than 0.1176, for ovrControllerType_XBox.
  /// This has been formally named simply "Trigger". We retain the name IndexTrigger for backwards
  /// code compatibility.
  /// User-facing documentation should refer to it as the Trigger.
  float IndexTrigger[ovrHand_Count];

  /// Left and right hand trigger values (ovrHand_Left and ovrHand_Right), in the range 0.0 to 1.0f.
  /// This has been formally named "Grip Button". We retain the name HandTrigger for backwards code
  /// compatibility.
  /// User-facing documentation should refer to it as the Grip Button or simply Grip.
  float HandTrigger[ovrHand_Count];

  /// Horizontal and vertical thumbstick axis values (ovrHand_Left and ovrHand_Right), in the range
  /// of -1.0f to 1.0f.
  /// Returns a deadzone (value 0) per each axis if the value on that axis would otherwise have been
  /// between -.2746 to +.2746, for ovrControllerType_XBox
  ovrVector2f Thumbstick[ovrHand_Count];

  /// The type of the controller this state is for.
  ovrControllerType ControllerType;

  /// Left and right finger trigger values (ovrHand_Left and ovrHand_Right), in range 0.0 to 1.0f.
  /// Does not apply a deadzone.  Only touch applies a filter.
  /// This has been formally named simply "Trigger". We retain the name IndexTrigger for backwards
  /// code compatibility.
  /// User-facing documentation should refer to it as the Trigger.
  float IndexTriggerNoDeadzone[ovrHand_Count];

  /// Left and right hand trigger values (ovrHand_Left and ovrHand_Right), in the range 0.0 to 1.0f.
  /// Does not apply a deadzone. Only touch applies a filter.
  /// This has been formally named "Grip Button". We retain the name HandTrigger for backwards code
  /// compatibility.
  /// User-facing documentation should refer to it as the Grip Button or simply Grip.
  float HandTriggerNoDeadzone[ovrHand_Count];

  /// Horizontal and vertical thumbstick axis values (ovrHand_Left and ovrHand_Right), in the range
  /// -1.0f to 1.0f
  /// Does not apply a deadzone or filter.
  ovrVector2f ThumbstickNoDeadzone[ovrHand_Count];

  /// Left and right finger trigger values (ovrHand_Left and ovrHand_Right), in range 0.0 to 1.0f.
  /// No deadzone or filter
  /// This has been formally named "Grip Button". We retain the name HandTrigger for backwards code
  /// compatibility.
  /// User-facing documentation should refer to it as the Grip Button or simply Grip.
  float IndexTriggerRaw[ovrHand_Count];

  /// Left and right hand trigger values (ovrHand_Left and ovrHand_Right), in the range 0.0 to 1.0f.
  /// No deadzone or filter
  /// This has been formally named "Grip Button". We retain the name HandTrigger for backwards code
  /// compatibility.
  /// User-facing documentation should refer to it as the Grip Button or simply Grip.
  float HandTriggerRaw[ovrHand_Count];

  /// Horizontal and vertical thumbstick axis values (ovrHand_Left and ovrHand_Right), in the range
  /// -1.0f to 1.0f
  /// No deadzone or filter
  ovrVector2f ThumbstickRaw[ovrHand_Count];
} ovrInputState;

typedef struct ovrCameraIntrinsics_ {
  /// Time in seconds from last change to the parameters
  double LastChangedTime;

  /// Angles of all 4 sides of viewport
  ovrFovPort FOVPort;

  /// Near plane of the virtual camera used to match the external camera
  float VirtualNearPlaneDistanceMeters;

  /// Far plane of the virtual camera used to match the external camera
  float VirtualFarPlaneDistanceMeters;

  /// Height in pixels of image sensor
  ovrSizei ImageSensorPixelResolution;

  /// The lens distortion matrix of camera
  ovrMatrix4f LensDistortionMatrix;

  /// How often, in seconds, the exposure is taken
  double ExposurePeriodSeconds;

  /// length of the exposure time
  double ExposureDurationSeconds;

} ovrCameraIntrinsics;

typedef enum ovrCameraStatusFlags_ {
  /// Initial state of camera
  ovrCameraStatus_None = 0x0,

  /// Bit set when the camera is connected to the system
  ovrCameraStatus_Connected = 0x1,

  /// Bit set when the camera is undergoing calibration
  ovrCameraStatus_Calibrating = 0x2,

  /// Bit set when the camera has tried & failed calibration
  ovrCameraStatus_CalibrationFailed = 0x4,

  /// Bit set when the camera has tried & passed calibration
  ovrCameraStatus_Calibrated = 0x8,
  ovrCameraStatus_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrCameraStatusFlags;

typedef struct ovrCameraExtrinsics_ {
  /// Time in seconds from last change to the parameters.
  /// For instance, if the pose changes, or a camera exposure happens, this struct will be updated.
  double LastChangedTimeSeconds;

  /// Current Status of the camera, a mix of bits from ovrCameraStatusFlags
  unsigned int CameraStatusFlags;

  /// Which Tracked device, if any, is the camera rigidly attached to
  /// If set to ovrTrackedDevice_None, then the camera is not attached to a tracked object.
  /// If the external camera moves while unattached (i.e. set to ovrTrackedDevice_None), its Pose
  /// won't be updated
  ovrTrackedDeviceType AttachedToDevice;

  /// The relative Pose of the External Camera.
  /// If AttachedToDevice is ovrTrackedDevice_None, then this is a absolute pose in tracking space
  ovrPosef RelativePose;

  /// The time, in seconds, when the last successful exposure was taken
  double LastExposureTimeSeconds;

  /// Estimated exposure latency to get from the exposure time to the system
  double ExposureLatencySeconds;

  /// Additional latency to get from the exposure time of the real camera to match the render time
  /// of the virtual camera
  double AdditionalLatencySeconds;

} ovrCameraExtrinsics;

#define OVR_EXTERNAL_CAMERA_NAME_SIZE 32
typedef struct ovrExternalCamera_ {
  char Name[OVR_EXTERNAL_CAMERA_NAME_SIZE]; // camera identifier: vid + pid + serial number etc.
  ovrCameraIntrinsics Intrinsics;
  ovrCameraExtrinsics Extrinsics;
} ovrExternalCamera;

//-----------------------------------------------------------------------------------
// ***** Initialize structures

/// Initialization flags.
///
/// \see ovrInitParams, ovr_Initialize
///
typedef enum ovrInitFlags_ {
  /// When a debug library is requested, a slower debugging version of the library will
  /// run which can be used to help solve problems in the library and debug application code.
  ovrInit_Debug = 0x00000001,


  /// When a version is requested, the LibOVR runtime respects the RequestedMinorVersion
  /// field and verifies that the RequestedMinorVersion is supported. Normally when you
  /// specify this flag you simply use OVR_MINOR_VERSION for ovrInitParams::RequestedMinorVersion,
  /// though you could use a lower version than OVR_MINOR_VERSION to specify previous
  /// version behavior.
  ovrInit_RequestVersion = 0x00000004,


  /// This client will not be visible in the HMD.
  /// Typically set by diagnostic or debugging utilities.
  ovrInit_Invisible = 0x00000010,

  /// This client will alternate between VR and 2D rendering.
  /// Typically set by game engine editors and VR-enabled web browsers.
  ovrInit_MixedRendering = 0x00000020,

  /// This client is aware of ovrSessionStatus focus states (e.g. ovrSessionStatus::HasInputFocus),
  /// and responds to them appropriately (e.g. pauses and stops drawing hands when lacking focus).
  ovrInit_FocusAware = 0x00000040,





  /// These bits are writable by user code.
  ovrinit_WritableBits = 0x00ffffff,

  ovrInit_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrInitFlags;

/// Logging levels
///
/// \see ovrInitParams, ovrLogCallback
///
typedef enum ovrLogLevel_ {
  ovrLogLevel_Debug = 0, ///< Debug-level log event.
  ovrLogLevel_Info = 1, ///< Info-level log event.
  ovrLogLevel_Error = 2, ///< Error-level log event.

  ovrLogLevel_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrLogLevel;

/// Signature of the logging callback function pointer type.
///
/// \param[in] userData is an arbitrary value specified by the user of ovrInitParams.
/// \param[in] level is one of the ovrLogLevel constants.
/// \param[in] message is a UTF8-encoded null-terminated string.
/// \see ovrInitParams, ovrLogLevel, ovr_Initialize
///
typedef void(OVR_CDECL* ovrLogCallback)(uintptr_t userData, int level, const char* message);

/// Parameters for ovr_Initialize.
///
/// \see ovr_Initialize
///
typedef struct OVR_ALIGNAS(8) ovrInitParams_ {
  /// Flags from ovrInitFlags to override default behavior.
  /// Use 0 for the defaults.
  uint32_t Flags;

  /// Requests a specific minor version of the LibOVR runtime.
  /// Flags must include ovrInit_RequestVersion or this will be ignored and OVR_MINOR_VERSION
  /// will be used. If you are directly calling the LibOVRRT version of ovr_Initialize
  /// in the LibOVRRT DLL then this must be valid and include ovrInit_RequestVersion.
  uint32_t RequestedMinorVersion;

  /// User-supplied log callback function, which may be called at any time
  /// asynchronously from multiple threads until ovr_Shutdown completes.
  /// Use NULL to specify no log callback.
  ovrLogCallback LogCallback;

  /// User-supplied data which is passed as-is to LogCallback. Typically this
  /// is used to store an application-specific pointer which is read in the
  /// callback function.
  uintptr_t UserData;

  /// Relative number of milliseconds to wait for a connection to the server
  /// before failing. Use 0 for the default timeout.
  uint32_t ConnectionTimeoutMS;

  OVR_ON64(OVR_UNUSED_STRUCT_PAD(pad0, 4)) ///< \internal

} ovrInitParams;

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(OVR_EXPORTING_CAPI)

// -----------------------------------------------------------------------------------
// ***** API Interfaces

/// Initializes LibOVR
///
/// Initialize LibOVR for application usage. This includes finding and loading the LibOVRRT
/// shared library. No LibOVR API functions, other than ovr_GetLastErrorInfo and ovr_Detect, can
/// be called unless ovr_Initialize succeeds. A successful call to ovr_Initialize must be eventually
/// followed by a call to ovr_Shutdown. ovr_Initialize calls are idempotent.
/// Calling ovr_Initialize twice does not require two matching calls to ovr_Shutdown.
/// If already initialized, the return value is ovr_Success.
///
/// LibOVRRT shared library search order:
///      -# Current working directory (often the same as the application directory).
///      -# Module directory (usually the same as the application directory,
///         but not if the module is a separate shared library).
///      -# Application directory
///      -# Development directory (only if OVR_ENABLE_DEVELOPER_SEARCH is enabled,
///         which is off by default).
///      -# Standard OS shared library search location(s) (OS-specific).
///
/// \param params Specifies custom initialization options. May be NULL to indicate default options
///        when using the CAPI shim. If you are directly calling the LibOVRRT version of
///        ovr_Initialize in the LibOVRRT DLL then this must be valid and
///        include ovrInit_RequestVersion.
/// \return Returns an ovrResult indicating success or failure. In the case of failure, use
///         ovr_GetLastErrorInfo to get more information. Example failed results include:
///     - ovrError_Initialize: Generic initialization error.
///     - ovrError_LibLoad: Couldn't load LibOVRRT.
///     - ovrError_LibVersion: LibOVRRT version incompatibility.
///     - ovrError_ServiceConnection: Couldn't connect to the OVR Service.
///     - ovrError_ServiceVersion: OVR Service version incompatibility.
///     - ovrError_IncompatibleOS: The operating system version is incompatible.
///     - ovrError_DisplayInit: Unable to initialize the HMD display.
///     - ovrError_ServerStart:  Unable to start the server. Is it already running?
///     - ovrError_Reinitialization: Attempted to re-initialize with a different version.
///
/// <b>Example code</b>
///     \code{.cpp}
///         ovrInitParams initParams = { ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL, 0, 0 };
///         ovrResult result = ovr_Initialize(&initParams);
///         if(OVR_FAILURE(result)) {
///             ovrErrorInfo errorInfo;
///             ovr_GetLastErrorInfo(&errorInfo);
///             DebugLog("ovr_Initialize failed: %s", errorInfo.ErrorString);
///             return false;
///         }
///         [...]
///     \endcode
///
/// \see ovr_Shutdown
///
OVR_PUBLIC_FUNCTION(ovrResult) ovr_Initialize(const ovrInitParams* params);

/// Shuts down LibOVR
///
/// A successful call to ovr_Initialize must be eventually matched by a call to ovr_Shutdown.
/// After calling ovr_Shutdown, no LibOVR functions can be called except ovr_GetLastErrorInfo
/// or another ovr_Initialize. ovr_Shutdown invalidates all pointers, references, and created
/// objects
/// previously returned by LibOVR functions. The LibOVRRT shared library can be unloaded by
/// ovr_Shutdown.
///
/// \see ovr_Initialize
///
OVR_PUBLIC_FUNCTION(void) ovr_Shutdown();

/// Returns information about the most recent failed return value by the
/// current thread for this library.
///
/// This function itself can never generate an error.
/// The last error is never cleared by LibOVR, but will be overwritten by new errors.
/// Do not use this call to determine if there was an error in the last API
/// call as successful API calls don't clear the last ovrErrorInfo.
/// To avoid any inconsistency, ovr_GetLastErrorInfo should be called immediately
/// after an API function that returned a failed ovrResult, with no other API
/// functions called in the interim.
///
/// \param[out] errorInfo The last ovrErrorInfo for the current thread.
///
/// \see ovrErrorInfo
///
OVR_PUBLIC_FUNCTION(void) ovr_GetLastErrorInfo(ovrErrorInfo* errorInfo);

/// Returns the version string representing the LibOVRRT version.
///
/// The returned string pointer is valid until the next call to ovr_Shutdown.
///
/// Note that the returned version string doesn't necessarily match the current
/// OVR_MAJOR_VERSION, etc., as the returned string refers to the LibOVRRT shared
/// library version and not the locally compiled interface version.
///
/// The format of this string is subject to change in future versions and its contents
/// should not be interpreted.
///
/// \return Returns a UTF8-encoded null-terminated version string.
///
OVR_PUBLIC_FUNCTION(const char*) ovr_GetVersionString();

/// Writes a message string to the LibOVR tracing mechanism (if enabled).
///
/// This message will be passed back to the application via the ovrLogCallback if
/// it was registered.
///
/// \param[in] level One of the ovrLogLevel constants.
/// \param[in] message A UTF8-encoded null-terminated string.
/// \return returns the strlen of the message or a negative value if the message is too large.
///
/// \see ovrLogLevel, ovrLogCallback
///
OVR_PUBLIC_FUNCTION(int) ovr_TraceMessage(int level, const char* message);

/// Identify client application info.
///
/// The string is one or more newline-delimited lines of optional info
/// indicating engine name, engine version, engine plugin name, engine plugin
/// version, engine editor. The order of the lines is not relevant. Individual
/// lines are optional. A newline is not necessary at the end of the last line.
/// Call after ovr_Initialize and before the first call to ovr_Create.
/// Each value is limited to 20 characters. Key names such as 'EngineName:'
/// 'EngineVersion:' do not count towards this limit.
///
/// \param[in] identity Specifies one or more newline-delimited lines of optional info:
///             EngineName: %s\n
///             EngineVersion: %s\n
///             EnginePluginName: %s\n
///             EnginePluginVersion: %s\n
///             EngineEditor: <boolean> ('true' or 'false')\n
///
/// <b>Example code</b>
///     \code{.cpp}
///     ovr_IdentifyClient("EngineName: Unity\n"
///                        "EngineVersion: 5.3.3\n"
///                        "EnginePluginName: OVRPlugin\n"
///                        "EnginePluginVersion: 1.2.0\n"
///                        "EngineEditor: true");
///     \endcode
///
OVR_PUBLIC_FUNCTION(ovrResult) ovr_IdentifyClient(const char* identity);

//-------------------------------------------------------------------------------------
/// @name HMD Management
///
/// Handles the enumeration, creation, destruction, and properties of an HMD (head-mounted display).
///@{

/// Returns information about the current HMD.
///
/// ovr_Initialize must be called prior to calling this function,
/// otherwise ovrHmdDesc::Type will be set to ovrHmd_None without
/// checking for the HMD presence.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create() or NULL.
///
/// \return Returns an ovrHmdDesc. If invoked with NULL session argument, ovrHmdDesc::Type
///         set to ovrHmd_None indicates that the HMD is not connected.
///
OVR_PUBLIC_FUNCTION(ovrHmdDesc) ovr_GetHmdDesc(ovrSession session);

/// Returns the number of attached trackers.
///
/// The number of trackers may change at any time, so this function should be called before use
/// as opposed to once on startup.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
///
/// \return Returns unsigned int count.
///
OVR_PUBLIC_FUNCTION(unsigned int) ovr_GetTrackerCount(ovrSession session);

/// Returns a given attached tracker description.
///
/// ovr_Initialize must have first been called in order for this to succeed, otherwise the returned
/// trackerDescArray will be zero-initialized. The data returned by this function can change at
/// runtime.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
///
/// \param[in] trackerDescIndex Specifies a tracker index. The valid indexes are in the
///            range of 0 to the tracker count returned by ovr_GetTrackerCount.
///
/// \return Returns ovrTrackerDesc. An empty ovrTrackerDesc will be returned if
///           trackerDescIndex is out of range.
///
/// \see ovrTrackerDesc, ovr_GetTrackerCount
///
OVR_PUBLIC_FUNCTION(ovrTrackerDesc)
ovr_GetTrackerDesc(ovrSession session, unsigned int trackerDescIndex);

/// Creates a handle to a VR session.
///
/// Upon success the returned ovrSession must be eventually freed with ovr_Destroy when it is no
/// longer needed.
/// A second call to ovr_Create will result in an error return value if the previous session has not
/// been destroyed.
///
/// \param[out] pSession Provides a pointer to an ovrSession which will be written to upon success.
/// \param[out] pLuid Provides a system specific graphics adapter identifier that locates which
/// graphics adapter has the HMD attached. This must match the adapter used by the application
/// or no rendering output will be possible. This is important for stability on multi-adapter
/// systems. An
/// application that simply chooses the default adapter will not run reliably on multi-adapter
/// systems.
/// \return Returns an ovrResult indicating success or failure. Upon failure
///         the returned ovrSession will be NULL.
///
/// <b>Example code</b>
///     \code{.cpp}
///         ovrSession session;
///         ovrGraphicsLuid luid;
///         ovrResult result = ovr_Create(&session, &luid);
///         if(OVR_FAILURE(result))
///            ...
///     \endcode
///
/// \see ovr_Destroy
///
OVR_PUBLIC_FUNCTION(ovrResult) ovr_Create(ovrSession* pSession, ovrGraphicsLuid* pLuid);

/// Destroys the session.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \see ovr_Create
///
OVR_PUBLIC_FUNCTION(void) ovr_Destroy(ovrSession session);

#endif // !defined(OVR_EXPORTING_CAPI)

/// Specifies status information for the current session.
///
/// \see ovr_GetSessionStatus
///
typedef struct ovrSessionStatus_ {
  /// True if the process has VR focus and thus is visible in the HMD.
  ovrBool IsVisible;

  /// True if an HMD is present.
  ovrBool HmdPresent;

  /// True if the HMD is on the user's head.
  ovrBool HmdMounted;

  /// True if the session is in a display-lost state. See ovr_SubmitFrame.
  ovrBool DisplayLost;

  /// True if the application should initiate shutdown.
  ovrBool ShouldQuit;

  /// True if UX has requested re-centering. Must call ovr_ClearShouldRecenterFlag,
  /// ovr_RecenterTrackingOrigin or ovr_SpecifyTrackingOrigin.
  ovrBool ShouldRecenter;

  /// True if the application is the foreground application and receives input (e.g. Touch
  /// controller state). If this is false then the application is in the background (but possibly
  /// still visible) should hide any input representations such as hands.
  ovrBool HasInputFocus;

  /// True if a system overlay is present, such as a dashboard. In this case the application
  /// (if visible) should pause while still drawing, avoid drawing near-field graphics so they
  /// don't visually fight with the system overlay, and consume fewer CPU and GPU resources.
  ovrBool OverlayPresent;

} ovrSessionStatus;

#if !defined(OVR_EXPORTING_CAPI)

/// Returns status information for the application.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[out] sessionStatus Provides an ovrSessionStatus that is filled in.
///
/// \return Returns an ovrResult indicating success or failure. In the case of
///         failure, use ovr_GetLastErrorInfo to get more information.
///         Return values include but aren't limited to:
///     - ovrSuccess: Completed successfully.
///     - ovrError_ServiceConnection: The service connection was lost and the application
///       must destroy the session.
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetSessionStatus(ovrSession session, ovrSessionStatus* sessionStatus);


/// Query extension support status.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] extension Extension to query.
/// \param[out] outExtensionSupported Set to extension support status. ovrTrue if supported.
///
/// \return Returns an ovrResult indicating success or failure. In the case of
///         failure use ovr_GetLastErrorInfo to get more information.
///
/// \see ovrExtensions
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_IsExtensionSupported(
    ovrSession session,
    ovrExtensions extension,
    ovrBool* outExtensionSupported);

/// Enable extension. Extensions must be enabled after ovr_Create is called.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] extension Extension to enable.
///
/// \return Returns an ovrResult indicating success or failure. Extension is only
///         enabled if successful. In the case of failure use ovr_GetLastErrorInfo
///         to get more information.
///
/// \see ovrExtensions
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_EnableExtension(ovrSession session, ovrExtensions extension);

//@}

//-------------------------------------------------------------------------------------
/// @name Tracking
///
/// Tracking functions handle the position, orientation, and movement of the HMD in space.
///
/// All tracking interface functions are thread-safe, allowing tracking state to be sampled
/// from different threads.
///
///@{


/// Sets the tracking origin type
///
/// When the tracking origin is changed, all of the calls that either provide
/// or accept ovrPosef will use the new tracking origin provided.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] origin Specifies an ovrTrackingOrigin to be used for all ovrPosef
///
/// \return Returns an ovrResult indicating success or failure. In the case of failure, use
///         ovr_GetLastErrorInfo to get more information.
///
/// \see ovrTrackingOrigin, ovr_GetTrackingOriginType
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_SetTrackingOriginType(ovrSession session, ovrTrackingOrigin origin);

/// Gets the tracking origin state
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
///
/// \return Returns the ovrTrackingOrigin that was either set by default, or previous set by the
/// application.
///
/// \see ovrTrackingOrigin, ovr_SetTrackingOriginType
OVR_PUBLIC_FUNCTION(ovrTrackingOrigin) ovr_GetTrackingOriginType(ovrSession session);

/// Re-centers the sensor position and orientation.
///
/// This resets the (x,y,z) positional components and the yaw orientation component of the
/// tracking space for the HMD and controllers using the HMD's current tracking pose.
/// If the caller requires some tweaks on top of the HMD's current tracking pose, consider using
/// ovr_SpecifyTrackingOrigin instead.
///
/// The roll and pitch orientation components are always determined by gravity and cannot
/// be redefined. All future tracking will report values relative to this new reference position.
/// If you are using ovrTrackerPoses then you will need to call ovr_GetTrackerPose after
/// this, because the sensor position(s) will change as a result of this.
///
/// The headset cannot be facing vertically upward or downward but rather must be roughly
/// level otherwise this function will fail with ovrError_InvalidHeadsetOrientation.
///
/// For more info, see the notes on each ovrTrackingOrigin enumeration to understand how
/// recenter will vary slightly in its behavior based on the current ovrTrackingOrigin setting.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
///
/// \return Returns an ovrResult indicating success or failure. In the case of failure, use
///         ovr_GetLastErrorInfo to get more information. Return values include but aren't limited
///         to:
///     - ovrSuccess: Completed successfully.
///     - ovrError_InvalidHeadsetOrientation: The headset was facing an invalid direction when
///       attempting recentering, such as facing vertically.
///
/// \see ovrTrackingOrigin, ovr_GetTrackerPose, ovr_SpecifyTrackingOrigin
///
OVR_PUBLIC_FUNCTION(ovrResult) ovr_RecenterTrackingOrigin(ovrSession session);

/// Allows manually tweaking the sensor position and orientation.
///
/// This function is similar to ovr_RecenterTrackingOrigin in that it modifies the
/// (x,y,z) positional components and the yaw orientation component of the tracking space for
/// the HMD and controllers.
///
/// While ovr_RecenterTrackingOrigin resets the tracking origin in reference to the HMD's
/// current pose, ovr_SpecifyTrackingOrigin allows the caller to explicitly specify a transform
/// for the tracking origin. This transform is expected to be an offset to the most recent
/// recentered origin, so calling this function repeatedly with the same originPose will keep
/// nudging the recentered origin in that direction.
///
/// There are several use cases for this function. For example, if the application decides to
/// limit the yaw, or translation of the recentered pose instead of directly using the HMD pose
/// the application can query the current tracking state via ovr_GetTrackingState, and apply
/// some limitations to the HMD pose because feeding this pose back into this function.
/// Similarly, this can be used to "adjust the seating position" incrementally in apps that
/// feature seated experiences such as cockpit-based games.
///
/// This function can emulate ovr_RecenterTrackingOrigin as such:
///     ovrTrackingState ts = ovr_GetTrackingState(session, 0.0, ovrFalse);
///     ovr_SpecifyTrackingOrigin(session, ts.HeadPose.ThePose);
///
/// The roll and pitch orientation components are determined by gravity and cannot be redefined.
/// If you are using ovrTrackerPoses then you will need to call ovr_GetTrackerPose after
/// this, because the sensor position(s) will change as a result of this.
///
/// For more info, see the notes on each ovrTrackingOrigin enumeration to understand how
/// recenter will vary slightly in its behavior based on the current ovrTrackingOrigin setting.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] originPose Specifies a pose that will be used to transform the current tracking
/// origin.
///
/// \return Returns an ovrResult indicating success or failure. In the case of failure, use
///         ovr_GetLastErrorInfo to get more information. Return values include but aren't limited
///         to:
///     - ovrSuccess: Completed successfully.
///     - ovrError_InvalidParameter: The heading direction in originPose was invalid,
///         such as facing vertically. This can happen if the caller is directly feeding the pose
///         of a position-tracked device such as an HMD or controller into this function.
///
/// \see ovrTrackingOrigin, ovr_GetTrackerPose, ovr_RecenterTrackingOrigin
///
OVR_PUBLIC_FUNCTION(ovrResult) ovr_SpecifyTrackingOrigin(ovrSession session, ovrPosef originPose);

/// Clears the ShouldRecenter status bit in ovrSessionStatus.
///
/// Clears the ShouldRecenter status bit in ovrSessionStatus, allowing further recenter requests to
/// be detected. Since this is automatically done by ovr_RecenterTrackingOrigin and
/// ovr_SpecifyTrackingOrigin, this function only needs to be called when application is doing
/// its own re-centering logic.
OVR_PUBLIC_FUNCTION(void) ovr_ClearShouldRecenterFlag(ovrSession session);

/// Returns tracking state reading based on the specified absolute system time.
///
/// Pass an absTime value of 0.0 to request the most recent sensor reading. In this case
/// both PredictedPose and SamplePose will have the same value.
///
/// This may also be used for more refined timing of front buffer rendering logic, and so on.
/// This may be called by multiple threads.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] absTime Specifies the absolute future time to predict the return
///            ovrTrackingState value. Use 0 to request the most recent tracking state.
/// \param[in] latencyMarker Specifies that this call is the point in time where
///            the "App-to-Mid-Photon" latency timer starts from. If a given ovrLayer
///            provides "SensorSampleTime", that will override the value stored here.
/// \return Returns the ovrTrackingState that is predicted for the given absTime.
///
/// \see ovrTrackingState, ovr_GetEyePoses, ovr_GetTimeInSeconds
///
OVR_PUBLIC_FUNCTION(ovrTrackingState)
ovr_GetTrackingState(ovrSession session, double absTime, ovrBool latencyMarker);

/// Returns an array of poses, where each pose matches a device type provided by the deviceTypes
/// array parameter.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] deviceTypes Array of device types to query for their poses.
/// \param[in] deviceCount Number of queried poses. This number must match the length of the
/// outDevicePoses and deviceTypes array.
/// \param[in] absTime Specifies the absolute future time to predict the return
///             ovrTrackingState value. Use 0 to request the most recent tracking state.
/// \param[out] outDevicePoses Array of poses, one for each device type in deviceTypes arrays.
///
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error and
///         true upon success.
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetDevicePoses(
    ovrSession session,
    ovrTrackedDeviceType* deviceTypes,
    int deviceCount,
    double absTime,
    ovrPoseStatef* outDevicePoses);


/// Returns the ovrTrackerPose for the given attached tracker.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] trackerPoseIndex Index of the tracker being requested.
///
/// \return Returns the requested ovrTrackerPose. An empty ovrTrackerPose will be returned if
/// trackerPoseIndex is out of range.
///
/// \see ovr_GetTrackerCount
///
OVR_PUBLIC_FUNCTION(ovrTrackerPose)
ovr_GetTrackerPose(ovrSession session, unsigned int trackerPoseIndex);

/// Returns the most recent input state for controllers, without positional tracking info.
///
/// \param[out] inputState Input state that will be filled in.
/// \param[in] ovrControllerType Specifies which controller the input will be returned for.
/// \return Returns ovrSuccess if the new state was successfully obtained.
///
/// \see ovrControllerType
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetInputState(ovrSession session, ovrControllerType controllerType, ovrInputState* inputState);

/// Returns controller types connected to the system OR'ed together.
///
/// \return A bitmask of ovrControllerTypes connected to the system.
///
/// \see ovrControllerType
///
OVR_PUBLIC_FUNCTION(unsigned int) ovr_GetConnectedControllerTypes(ovrSession session);

/// Gets information about Haptics engine for the specified Touch controller.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] controllerType The controller to retrieve the information from.
///
/// \return Returns an ovrTouchHapticsDesc.
///
OVR_PUBLIC_FUNCTION(ovrTouchHapticsDesc)
ovr_GetTouchHapticsDesc(ovrSession session, ovrControllerType controllerType);

/// Sets constant vibration (with specified frequency and amplitude) to a controller.
/// Note: ovr_SetControllerVibration cannot be used interchangeably with
/// ovr_SubmitControllerVibration.
///
/// This method should be called periodically, vibration lasts for a maximum of 2.5 seconds.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] controllerType The controller to set the vibration to.
/// \param[in] frequency Vibration frequency. Supported values are: 0.0 (disabled), 0.5 and 1.0. Non
/// valid values will be clamped.
/// \param[in] amplitude Vibration amplitude in the [0.0, 1.0] range.
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error and true
///         upon success. Return values include but aren't limited to:
///     - ovrSuccess: The call succeeded and a result was returned.
///     - ovrSuccess_DeviceUnavailable: The call succeeded but the device referred to by
///     controllerType is not available.
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_SetControllerVibration(
    ovrSession session,
    ovrControllerType controllerType,
    float frequency,
    float amplitude);

/// Submits a Haptics buffer (used for vibration) to Touch (only) controllers.
/// Note: ovr_SubmitControllerVibration cannot be used interchangeably with
/// ovr_SetControllerVibration.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] controllerType Controller where the Haptics buffer will be played.
/// \param[in] buffer Haptics buffer containing amplitude samples to be played.
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error and true
///         upon success. Return values include but aren't limited to:
///     - ovrSuccess: The call succeeded and a result was returned.
///     - ovrSuccess_DeviceUnavailable: The call succeeded but the device referred to by
///     controllerType is not available.
///
/// \see ovrHapticsBuffer
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_SubmitControllerVibration(
    ovrSession session,
    ovrControllerType controllerType,
    const ovrHapticsBuffer* buffer);

/// Gets the Haptics engine playback state of a specific Touch controller.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] controllerType Controller where the Haptics buffer wil be played.
/// \param[in] outState State of the haptics engine.
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error and true
///         upon success. Return values include but aren't limited to:
///     - ovrSuccess: The call succeeded and a result was returned.
///     - ovrSuccess_DeviceUnavailable: The call succeeded but the device referred to by
///     controllerType is not available.
///
/// \see ovrHapticsPlaybackState
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetControllerVibrationState(
    ovrSession session,
    ovrControllerType controllerType,
    ovrHapticsPlaybackState* outState);

/// Tests collision/proximity of position tracked devices (e.g. HMD and/or Touch) against the
/// Boundary System.
/// Note: this method is similar to ovr_BoundaryTestPoint but can be more precise as it may take
/// into account device acceleration/momentum.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] deviceBitmask Bitmask of one or more tracked devices to test.
/// \param[in] boundaryType Must be either ovrBoundary_Outer or ovrBoundary_PlayArea.
/// \param[out] outTestResult Result of collision/proximity test, contains information such as
/// distance and closest point.
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error and true
///         upon success. Return values include but aren't limited to:
///     - ovrSuccess: The call succeeded and a result was returned.
///     - ovrSuccess_BoundaryInvalid: The call succeeded but the result is not a valid boundary due
///     to not being set up.
///     - ovrSuccess_DeviceUnavailable: The call succeeded but the device referred to by
///     deviceBitmask is not available.
///
/// \see ovrBoundaryTestResult
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_TestBoundary(
    ovrSession session,
    ovrTrackedDeviceType deviceBitmask,
    ovrBoundaryType boundaryType,
    ovrBoundaryTestResult* outTestResult);

/// Tests collision/proximity of a 3D point against the Boundary System.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] point 3D point to test.
/// \param[in] singleBoundaryType Must be either ovrBoundary_Outer or ovrBoundary_PlayArea to test
/// against
/// \param[out] outTestResult Result of collision/proximity test, contains information such as
/// distance and closest point.
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error and true
///         upon success. Return values include but aren't limited to:
///     - ovrSuccess: The call succeeded and a result was returned.
///     - ovrSuccess_BoundaryInvalid: The call succeeded but the result is not a valid boundary due
///     to not being set up.
///
/// \see ovrBoundaryTestResult
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_TestBoundaryPoint(
    ovrSession session,
    const ovrVector3f* point,
    ovrBoundaryType singleBoundaryType,
    ovrBoundaryTestResult* outTestResult);

/// Sets the look and feel of the Boundary System.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] lookAndFeel Look and feel parameters.
/// \return Returns ovrSuccess upon success.
/// \see ovrBoundaryLookAndFeel
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_SetBoundaryLookAndFeel(ovrSession session, const ovrBoundaryLookAndFeel* lookAndFeel);

/// Resets the look and feel of the Boundary System to its default state.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \return Returns ovrSuccess upon success.
/// \see ovrBoundaryLookAndFeel
///
OVR_PUBLIC_FUNCTION(ovrResult) ovr_ResetBoundaryLookAndFeel(ovrSession session);

/// Gets the geometry of the Boundary System's "play area" or "outer boundary" as 3D floor points.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] boundaryType Must be either ovrBoundary_Outer or ovrBoundary_PlayArea.
/// \param[out] outFloorPoints Array of 3D points (in clockwise order) defining the boundary at
/// floor height (can be NULL to retrieve only the number of points).
/// \param[out] outFloorPointsCount Number of 3D points returned in the array.
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error and true
///         upon success. Return values include but aren't limited to:
///     - ovrSuccess: The call succeeded and a result was returned.
///     - ovrSuccess_BoundaryInvalid: The call succeeded but the result is not a valid boundary due
///     to not being set up.
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetBoundaryGeometry(
    ovrSession session,
    ovrBoundaryType boundaryType,
    ovrVector3f* outFloorPoints,
    int* outFloorPointsCount);

/// Gets the dimension of the Boundary System's "play area" or "outer boundary".
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] boundaryType Must be either ovrBoundary_Outer or ovrBoundary_PlayArea.
/// \param[out] outDimensions Dimensions of the axis aligned bounding box that encloses the area in
/// meters (width, height and length).
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error and true
///         upon success. Return values include but aren't limited to:
///     - ovrSuccess: The call succeeded and a result was returned.
///     - ovrSuccess_BoundaryInvalid: The call succeeded but the result is not a valid boundary due
///     to not being set up.
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetBoundaryDimensions(
    ovrSession session,
    ovrBoundaryType boundaryType,
    ovrVector3f* outDimensions);

/// Returns if the boundary is currently visible.
/// Note: visibility is false if the user has turned off boundaries, otherwise, it's true if
/// the app has requested boundaries to be visible or if any tracked device is currently
/// triggering it. This may not exactly match rendering due to fade-in and fade-out effects.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[out] outIsVisible ovrTrue, if the boundary is visible.
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error and true
///         upon success. Return values include but aren't limited to:
///     - ovrSuccess: Result was successful and a result was returned.
///     - ovrSuccess_BoundaryInvalid: The call succeeded but the result is not a valid boundary due
///     to not being set up.
///
OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetBoundaryVisible(ovrSession session, ovrBool* outIsVisible);

/// Requests boundary to be visible.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] visible forces the outer boundary to be visible. An application can't force it
///            to be invisible, but can cancel its request by passing false.
/// \return Returns ovrSuccess upon success.
///
OVR_PUBLIC_FUNCTION(ovrResult) ovr_RequestBoundaryVisible(ovrSession session, ovrBool visible);

///@}

#endif // !defined(OVR_EXPORTING_CAPI)

//-------------------------------------------------------------------------------------
// @name Layers
//
///@{

///  Specifies the maximum number of layers supported by ovr_SubmitFrame.
///
///  /see ovr_SubmitFrame
///
enum { ovrMaxLayerCount = 16 };

/// Describes layer types that can be passed to ovr_SubmitFrame.
/// Each layer type has an associated struct, such as ovrLayerEyeFov.
///
/// \see ovrLayerHeader
///
typedef enum ovrLayerType_ {
  /// Layer is disabled.
  ovrLayerType_Disabled = 0,

  /// Described by ovrLayerEyeFov.
  ovrLayerType_EyeFov = 1,

  /// Described by ovrLayerEyeFovDepth.
  ovrLayerType_EyeFovDepth = 2,

  /// Described by ovrLayerQuad. Previously called ovrLayerType_QuadInWorld.
  ovrLayerType_Quad = 3,

  // enum 4 used to be ovrLayerType_QuadHeadLocked. Instead, use ovrLayerType_Quad with
  // ovrLayerFlag_HeadLocked.

  /// Described by ovrLayerEyeMatrix.
  ovrLayerType_EyeMatrix = 5,


  /// Described by ovrLayerEyeFovMultires.
  ovrLayerType_EyeFovMultires = 7,

  /// Described by ovrLayerCylinder.
  ovrLayerType_Cylinder = 8,

  /// Described by ovrLayerCube
  ovrLayerType_Cube = 10,


  ovrLayerType_EnumSize = 0x7fffffff ///< Force type int32_t.

} ovrLayerType;

/// Identifies flags used by ovrLayerHeader and which are passed to ovr_SubmitFrame.
///
/// \see ovrLayerHeader
///
typedef enum ovrLayerFlags_ {
  /// ovrLayerFlag_HighQuality enables 4x anisotropic sampling during the composition of the layer.
  /// The benefits are mostly visible at the periphery for high-frequency & high-contrast visuals.
  /// For best results consider combining this flag with an ovrTextureSwapChain that has mipmaps and
  /// instead of using arbitrary sized textures, prefer texture sizes that are powers-of-two.
  /// Actual rendered viewport and doesn't necessarily have to fill the whole texture.
  ovrLayerFlag_HighQuality = 0x01,

  /// ovrLayerFlag_TextureOriginAtBottomLeft: the opposite is TopLeft.
  /// Generally this is false for D3D, true for OpenGL.
  ovrLayerFlag_TextureOriginAtBottomLeft = 0x02,

  /// Mark this surface as "headlocked", which means it is specified
  /// relative to the HMD and moves with it, rather than being specified
  /// relative to sensor/torso space and remaining still while the head moves.
  /// What used to be ovrLayerType_QuadHeadLocked is now ovrLayerType_Quad plus this flag.
  /// However the flag can be applied to any layer type to achieve a similar effect.
  ovrLayerFlag_HeadLocked = 0x04,


} ovrLayerFlags;

/// Defines properties shared by all ovrLayer structs, such as ovrLayerEyeFov.
///
/// ovrLayerHeader is used as a base member in these larger structs.
/// This struct cannot be used by itself except for the case that Type is ovrLayerType_Disabled.
///
/// \see ovrLayerType, ovrLayerFlags
///
typedef struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrLayerHeader_ {
  ovrLayerType Type; ///< Described by ovrLayerType.
  unsigned Flags; ///< Described by ovrLayerFlags.
} ovrLayerHeader;

/// Describes a layer that specifies a monoscopic or stereoscopic view.
/// This is the kind of layer that's typically used as layer 0 to ovr_SubmitFrame,
/// as it is the kind of layer used to render a 3D stereoscopic view.
///
/// Three options exist with respect to mono/stereo texture usage:
///    - ColorTexture[0] and ColorTexture[1] contain the left and right stereo renderings,
///      respectively.
///      Viewport[0] and Viewport[1] refer to ColorTexture[0] and ColorTexture[1], respectively.
///    - ColorTexture[0] contains both the left and right renderings, ColorTexture[1] is NULL,
///      and Viewport[0] and Viewport[1] refer to sub-rects with ColorTexture[0].
///    - ColorTexture[0] contains a single monoscopic rendering, and Viewport[0] and
///      Viewport[1] both refer to that rendering.
///
/// \see ovrTextureSwapChain, ovr_SubmitFrame
///
typedef struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrLayerEyeFov_ {
  /// Header.Type must be ovrLayerType_EyeFov.
  ovrLayerHeader Header;

  /// ovrTextureSwapChains for the left and right eye respectively.
  /// The second one of which can be NULL for cases described above.
  ovrTextureSwapChain ColorTexture[ovrEye_Count];

  /// Specifies the ColorTexture sub-rect UV coordinates.
  /// Both Viewport[0] and Viewport[1] must be valid.
  ovrRecti Viewport[ovrEye_Count];

  /// The viewport field of view.
  ovrFovPort Fov[ovrEye_Count];

  /// Specifies the position and orientation of each eye view, with position specified in meters.
  /// RenderPose will typically be the value returned from ovr_CalcEyePoses,
  /// but can be different in special cases if a different head pose is used for rendering.
  ovrPosef RenderPose[ovrEye_Count];

  /// Specifies the timestamp when the source ovrPosef (used in calculating RenderPose)
  /// was sampled from the SDK. Typically retrieved by calling ovr_GetTimeInSeconds
  /// around the instant the application calls ovr_GetTrackingState
  /// The main purpose for this is to accurately track app tracking latency.
  double SensorSampleTime;

} ovrLayerEyeFov;

/// Describes a layer that specifies a monoscopic or stereoscopic view,
/// with depth textures in addition to color textures. This is typically used to support
/// positional time warp. This struct is the same as ovrLayerEyeFov, but with the addition
/// of DepthTexture and ProjectionDesc.
///
/// ProjectionDesc can be created using ovrTimewarpProjectionDesc_FromProjection.
///
/// Three options exist with respect to mono/stereo texture usage:
///    - ColorTexture[0] and ColorTexture[1] contain the left and right stereo renderings,
///      respectively.
///      Viewport[0] and Viewport[1] refer to ColorTexture[0] and ColorTexture[1], respectively.
///    - ColorTexture[0] contains both the left and right renderings, ColorTexture[1] is NULL,
///      and Viewport[0] and Viewport[1] refer to sub-rects with ColorTexture[0].
///    - ColorTexture[0] contains a single monoscopic rendering, and Viewport[0] and
///      Viewport[1] both refer to that rendering.
///
/// \see ovrTextureSwapChain, ovr_SubmitFrame
///
typedef struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrLayerEyeFovDepth_ {
  /// Header.Type must be ovrLayerType_EyeFovDepth.
  ovrLayerHeader Header;

  /// ovrTextureSwapChains for the left and right eye respectively.
  /// The second one of which can be NULL for cases described above.
  ovrTextureSwapChain ColorTexture[ovrEye_Count];

  /// Specifies the ColorTexture sub-rect UV coordinates.
  /// Both Viewport[0] and Viewport[1] must be valid.
  ovrRecti Viewport[ovrEye_Count];

  /// The viewport field of view.
  ovrFovPort Fov[ovrEye_Count];

  /// Specifies the position and orientation of each eye view, with position specified in meters.
  /// RenderPose will typically be the value returned from ovr_CalcEyePoses,
  /// but can be different in special cases if a different head pose is used for rendering.
  ovrPosef RenderPose[ovrEye_Count];

  /// Specifies the timestamp when the source ovrPosef (used in calculating RenderPose)
  /// was sampled from the SDK. Typically retrieved by calling ovr_GetTimeInSeconds
  /// around the instant the application calls ovr_GetTrackingState
  /// The main purpose for this is to accurately track app tracking latency.
  double SensorSampleTime;

  /// Depth texture for positional timewarp.
  /// Must map 1:1 to the ColorTexture.
  ovrTextureSwapChain DepthTexture[ovrEye_Count];

  /// Specifies how to convert DepthTexture information into meters.
  /// \see ovrTimewarpProjectionDesc_FromProjection
  ovrTimewarpProjectionDesc ProjectionDesc;

} ovrLayerEyeFovDepth;

/// Describes eye texture layouts. Used with ovrLayerEyeFovMultires.
///
typedef enum ovrTextureLayout_ {
  ovrTextureLayout_Rectilinear = 0, ///< Regular eyeFov layer.
  ovrTextureLayout_Octilinear = 1, ///< Octilinear extension must be enabled.
  ovrTextureLayout_EnumSize = 0x7fffffff ///< Force type int32_t.
} ovrTextureLayout;

/// Multiresolution descriptor for Octilinear.
///
/// \see ovrLayerEyeFovMultres
///
typedef struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrTextureLayoutOctilinear_ {
  // W warping
  float WarpLeft;
  float WarpRight;
  float WarpUp;
  float WarpDown;

  // Size of W quadrants.
  //
  // SizeLeft + SizeRight <= Viewport.Size.w
  // SizeUp   + sizeDown  <= Viewport.Size.h
  //
  // Clip space (0,0) is located at Viewport.Pos + (SizeLeft,SizeUp) where
  // Viewport is given in the layer description.
  //
  // Viewport Top left
  // +-----------------------------------------------------+
  // |                        ^                       |    |
  // |                        |                       |    |
  // |           0          SizeUp         1          |    |
  // |                        |                       |<--Portion of viewport
  // |                        |                       |   determined by sizes
  // |                        |                       |    |
  // |<--------SizeLeft-------+-------SizeRight------>|    |
  // |                        |                       |    |
  // |                        |                       |    |
  // |           2         SizeDown        3          |    |
  // |                        |                       |    |
  // |                        |                       |    |
  // |                        v                       |    |
  // +------------------------------------------------+    |
  // |                                                     |
  // +-----------------------------------------------------+
  //                                                       Viewport bottom right
  //
  // For example, when rendering quadrant 0 its scissor rectangle will be
  //
  //  Top    = 0
  //  Left   = 0
  //  Right  = SizeLeft
  //  Bottom = SizeUp
  //
  // and the scissor rectangle for quadrant 1 will be:
  //
  //  Top    = 0
  //  Left   = SizeLeft
  //  Right  = SizeLeft + SizeRight
  //  Bottom = SizeUp
  //
  float SizeLeft;
  float SizeRight;
  float SizeUp;
  float SizeDown;

} ovrTextureLayoutOctilinear;

/// Combines texture layout descriptors.
///
typedef union OVR_ALIGNAS(OVR_PTR_SIZE) ovrTextureLayoutDesc_Union_ {
  ovrTextureLayoutOctilinear Octilinear[ovrEye_Count];
} ovrTextureLayoutDesc_Union;

/// Describes a layer that specifies a monoscopic or stereoscopic view with
/// support for optional multiresolution textures. This struct is the same as
/// ovrLayerEyeFov plus texture layout parameters.
///
/// Three options exist with respect to mono/stereo texture usage:
///    - ColorTexture[0] and ColorTexture[1] contain the left and right stereo renderings,
///      respectively.
///      Viewport[0] and Viewport[1] refer to ColorTexture[0] and ColorTexture[1], respectively.
///    - ColorTexture[0] contains both the left and right renderings, ColorTexture[1] is NULL,
///      and Viewport[0] and Viewport[1] refer to sub-rects with ColorTexture[0].
///    - ColorTexture[0] contains a single monoscopic rendering, and Viewport[0] and
///      Viewport[1] both refer to that rendering.
///
/// \see ovrTextureSwapChain, ovr_SubmitFrame
///
typedef struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrLayerEyeFovMultires_ {
  /// Header.Type must be ovrLayerType_EyeFovMultires.
  ovrLayerHeader Header;

  /// ovrTextureSwapChains for the left and right eye respectively.
  /// The second one of which can be NULL for cases described above.
  ovrTextureSwapChain ColorTexture[ovrEye_Count];

  /// Specifies the ColorTexture sub-rect UV coordinates.
  /// Both Viewport[0] and Viewport[1] must be valid.
  ovrRecti Viewport[ovrEye_Count];

  /// The viewport field of view.
  ovrFovPort Fov[ovrEye_Count];

  /// Specifies the position and orientation of each eye view, with position specified in meters.
  /// RenderPose will typically be the value returned from ovr_CalcEyePoses,
  /// but can be different in special cases if a different head pose is used for rendering.
  ovrPosef RenderPose[ovrEye_Count];

  /// Specifies the timestamp when the source ovrPosef (used in calculating RenderPose)
  /// was sampled from the SDK. Typically retrieved by calling ovr_GetTimeInSeconds
  /// around the instant the application calls ovr_GetTrackingState
  /// The main purpose for this is to accurately track app tracking latency.
  double SensorSampleTime;

  /// Specifies layout type of textures.
  ovrTextureLayout TextureLayout;

  /// Specifies texture layout parameters.
  ovrTextureLayoutDesc_Union TextureLayoutDesc;

} ovrLayerEyeFovMultires;

/// Describes a layer that specifies a monoscopic or stereoscopic view.
/// This uses a direct 3x4 matrix to map from view space to the UV coordinates.
/// It is essentially the same thing as ovrLayerEyeFov but using a much
/// lower level. This is mainly to provide compatibility with specific apps.
/// Unless the application really requires this flexibility, it is usually better
/// to use ovrLayerEyeFov.
///
/// Three options exist with respect to mono/stereo texture usage:
///    - ColorTexture[0] and ColorTexture[1] contain the left and right stereo renderings,
///      respectively.
///      Viewport[0] and Viewport[1] refer to ColorTexture[0] and ColorTexture[1], respectively.
///    - ColorTexture[0] contains both the left and right renderings, ColorTexture[1] is NULL,
///      and Viewport[0] and Viewport[1] refer to sub-rects with ColorTexture[0].
///    - ColorTexture[0] contains a single monoscopic rendering, and Viewport[0] and
///      Viewport[1] both refer to that rendering.
///
/// \see ovrTextureSwapChain, ovr_SubmitFrame
///
typedef struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrLayerEyeMatrix_ {
  /// Header.Type must be ovrLayerType_EyeMatrix.
  ovrLayerHeader Header;

  /// ovrTextureSwapChains for the left and right eye respectively.
  /// The second one of which can be NULL for cases described above.
  ovrTextureSwapChain ColorTexture[ovrEye_Count];

  /// Specifies the ColorTexture sub-rect UV coordinates.
  /// Both Viewport[0] and Viewport[1] must be valid.
  ovrRecti Viewport[ovrEye_Count];

  /// Specifies the position and orientation of each eye view, with position specified in meters.
  /// RenderPose will typically be the value returned from ovr_CalcEyePoses,
  /// but can be different in special cases if a different head pose is used for rendering.
  ovrPosef RenderPose[ovrEye_Count];

  /// Specifies the mapping from a view-space vector
  /// to a UV coordinate on the textures given above.
  /// P = (x,y,z,1)*Matrix
  /// TexU  = P.x/P.z
  /// TexV  = P.y/P.z
  ovrMatrix4f Matrix[ovrEye_Count];

  /// Specifies the timestamp when the source ovrPosef (used in calculating RenderPose)
  /// was sampled from the SDK. Typically retrieved by calling ovr_GetTimeInSeconds
  /// around the instant the application calls ovr_GetTrackingState
  /// The main purpose for this is to accurately track app tracking latency.
  double SensorSampleTime;

} ovrLayerEyeMatrix;

/// Describes a layer of Quad type, which is a single quad in world or viewer space.
/// It is used for ovrLayerType_Quad. This type of layer represents a single
/// object placed in the world and not a stereo view of the world itself.
///
/// A typical use of ovrLayerType_Quad is to draw a television screen in a room
/// that for some reason is more convenient to draw as a layer than as part of the main
/// view in layer 0. For example, it could implement a 3D popup GUI that is drawn at a
/// higher resolution than layer 0 to improve fidelity of the GUI.
///
/// Quad layers are visible from both sides; they are not back-face culled.
///
/// \see ovrTextureSwapChain, ovr_SubmitFrame
///
typedef struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrLayerQuad_ {
  /// Header.Type must be ovrLayerType_Quad.
  ovrLayerHeader Header;

  /// Contains a single image, never with any stereo view.
  ovrTextureSwapChain ColorTexture;

  /// Specifies the ColorTexture sub-rect UV coordinates.
  ovrRecti Viewport;

  /// Specifies the orientation and position of the center point of a Quad layer type.
  /// The supplied direction is the vector perpendicular to the quad.
  /// The position is in real-world meters (not the application's virtual world,
  /// the physical world the user is in) and is relative to the "zero" position
  /// set by ovr_RecenterTrackingOrigin unless the ovrLayerFlag_HeadLocked flag is used.
  ovrPosef QuadPoseCenter;

  /// Width and height (respectively) of the quad in meters.
  ovrVector2f QuadSize;

} ovrLayerQuad;

/// Describes a layer of type ovrLayerType_Cylinder which is a single cylinder
/// relative to the recentered origin. This type of layer represents a single
/// object placed in the world and not a stereo view of the world itself.
///
///                -Z                                       +Y
///         U=0  +--+--+  U=1
///          +---+  |  +---+            +-----------------+  - V=0
///       +--+ \    |    / +--+         |                 |  |
///     +-+     \       /     +-+       |                 |  |
///    ++        \  A  /        ++      |                 |  |
///   ++          \---/          ++     |                 |  |
///   |            \ /            |     |              +X |  |
///   +-------------C------R------+ +X  +--------C--------+  | <--- Height
///       (+Y is out of screen)         |                 |  |
///                                     |                 |  |
///   R = Radius                        |                 |  |
///   A = Angle (0,2*Pi)                |                 |  |
///   C = CylinderPoseCenter            |                 |  |
///   U/V = UV Coordinates              +-----------------+  - V=1
///
/// An identity CylinderPoseCenter places the center of the cylinder
/// at the recentered origin unless the headlocked flag is set.
///
/// Does not utilize HmdSpaceToWorldScaleInMeters. If necessary, adjust
/// translation and radius.
///
/// \note Only the interior surface of the cylinder is visible. Use cylinder
/// layers when the user cannot leave the extents of the cylinder. Artifacts may
/// appear when viewing the cylinder's exterior surface. Additionally, while the
/// interface supports an Angle that ranges from [0,2*Pi] the angle should
/// remain less than 1.9*PI to avoid artifacts where the cylinder edges
/// converge.
///
/// \see ovrTextureSwapChain, ovr_SubmitFrame
///
typedef struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrLayerCylinder_ {
  /// Header.Type must be ovrLayerType_Cylinder.
  ovrLayerHeader Header;

  /// Contains a single image, never with any stereo view.
  ovrTextureSwapChain ColorTexture;

  /// Specifies the ColorTexture sub-rect UV coordinates.
  ovrRecti Viewport;

  /// Specifies the orientation and position of the center point of a cylinder layer type.
  /// The position is in real-world meters not the application's virtual world,
  /// but the physical world the user is in. It is relative to the "zero" position
  /// set by ovr_RecenterTrackingOrigin unless the ovrLayerFlag_HeadLocked flag is used.
  ovrPosef CylinderPoseCenter;

  /// Radius of the cylinder in meters.
  float CylinderRadius;

  /// Angle in radians. Range is from 0 to 2*Pi exclusive covering the entire
  /// cylinder (see diagram and note above).
  float CylinderAngle;

  /// Custom aspect ratio presumably set based on 'Viewport'. Used to
  /// calculate the height of the cylinder based on the arc-length (CylinderAngle)
  /// and radius (CylinderRadius) given above. The height of the cylinder is
  /// given by: height = (CylinderRadius * CylinderAngle) / CylinderAspectRatio.
  /// Aspect ratio is width / height.
  float CylinderAspectRatio;

} ovrLayerCylinder;

/// Describes a layer of type ovrLayerType_Cube which is a single timewarped
/// cubemap at infinity. When looking down the recentered origin's -Z axis, +X
/// face is left and +Y face is up. Similarly, if headlocked the +X face is
/// left, +Y face is up and -Z face is forward. Note that the coordinate system
/// is left-handed.
///
/// ovrLayerFlag_TextureOriginAtBottomLeft flag is not supported by ovrLayerCube.
///
/// \see ovrTextureSwapChain, ovr_SubmitFrame
///
typedef struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrLayerCube_ {
  /// Header.Type must be ovrLayerType_Cube.
  ovrLayerHeader Header;

  /// Orientation of the cube.
  ovrQuatf Orientation;

  /// Contains a single cubemap swapchain (not a stereo pair of swapchains).
  ovrTextureSwapChain CubeMapTexture;
} ovrLayerCube;



/// Union that combines ovrLayer types in a way that allows them
/// to be used in a polymorphic way.
typedef union ovrLayer_Union_ {
  ovrLayerHeader Header;
  ovrLayerEyeFov EyeFov;
  ovrLayerEyeFovDepth EyeFovDepth;
  ovrLayerQuad Quad;
  ovrLayerEyeFovMultires Multires;
  ovrLayerCylinder Cylinder;
  ovrLayerCube Cube;
} ovrLayer_Union;

//@}

#if !defined(OVR_EXPORTING_CAPI)

/// @name SDK Distortion Rendering
///
/// All of rendering functions including the configure and frame functions
/// are not thread safe. It is OK to use ConfigureRendering on one thread and handle
/// frames on another thread, but explicit synchronization must be done since
/// functions that depend on configured state are not reentrant.
///
/// These functions support rendering of distortion by the SDK.
///
//@{

/// TextureSwapChain creation is rendering API-specific.
/// ovr_CreateTextureSwapChainDX and ovr_CreateTextureSwapChainGL can be found in the
/// rendering API-specific headers, such as OVR_CAPI_D3D.h and OVR_CAPI_GL.h

/// Gets the number of buffers in an ovrTextureSwapChain.
///
/// \param[in]  session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in]  chain Specifies the ovrTextureSwapChain for which the length should be retrieved.
/// \param[out] out_Length Returns the number of buffers in the specified chain.
///
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error.
///
/// \see ovr_CreateTextureSwapChainDX, ovr_CreateTextureSwapChainGL
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetTextureSwapChainLength(ovrSession session, ovrTextureSwapChain chain, int* out_Length);

/// Gets the current index in an ovrTextureSwapChain.
///
/// \param[in]  session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in]  chain Specifies the ovrTextureSwapChain for which the index should be retrieved.
/// \param[out] out_Index Returns the current (free) index in specified chain.
///
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error.
///
/// \see ovr_CreateTextureSwapChainDX, ovr_CreateTextureSwapChainGL
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetTextureSwapChainCurrentIndex(ovrSession session, ovrTextureSwapChain chain, int* out_Index);

/// Gets the description of the buffers in an ovrTextureSwapChain
///
/// \param[in]  session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in]  chain Specifies the ovrTextureSwapChain for which the description
///                   should be retrieved.
/// \param[out] out_Desc Returns the description of the specified chain.
///
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error.
///
/// \see ovr_CreateTextureSwapChainDX, ovr_CreateTextureSwapChainGL
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetTextureSwapChainDesc(
    ovrSession session,
    ovrTextureSwapChain chain,
    ovrTextureSwapChainDesc* out_Desc);

/// Commits any pending changes to an ovrTextureSwapChain, and advances its current index
///
/// \param[in]  session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in]  chain Specifies the ovrTextureSwapChain to commit.
///
/// \note When Commit is called, the texture at the current index is considered ready for use by the
/// runtime, and further writes to it should be avoided. The swap chain's current index is advanced,
/// providing there's room in the chain. The next time the SDK dereferences this texture swap chain,
/// it will synchronize with the app's graphics context and pick up the submitted index, opening up
/// room in the swap chain for further commits.
///
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error.
///         Failures include but aren't limited to:
///     - ovrError_TextureSwapChainFull: ovr_CommitTextureSwapChain was called too many times on a
///         texture swapchain without calling submit to use the chain.
///
/// \see ovr_CreateTextureSwapChainDX, ovr_CreateTextureSwapChainGL
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_CommitTextureSwapChain(ovrSession session, ovrTextureSwapChain chain);

/// Destroys an ovrTextureSwapChain and frees all the resources associated with it.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] chain Specifies the ovrTextureSwapChain to destroy. If it is NULL then
///            this function has no effect.
///
/// \see ovr_CreateTextureSwapChainDX, ovr_CreateTextureSwapChainGL
///
OVR_PUBLIC_FUNCTION(void)
ovr_DestroyTextureSwapChain(ovrSession session, ovrTextureSwapChain chain);

/// MirrorTexture creation is rendering API-specific.
/// ovr_CreateMirrorTextureWithOptionsDX and ovr_CreateMirrorTextureWithOptionsGL can be found in
/// rendering API-specific headers, such as OVR_CAPI_D3D.h and OVR_CAPI_GL.h

/// Destroys a mirror texture previously created by one of the mirror texture creation functions.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] mirrorTexture Specifies the ovrTexture to destroy. If it is NULL then
///            this function has no effect.
///
/// \see ovr_CreateMirrorTextureWithOptionsDX, ovr_CreateMirrorTextureWithOptionsGL
///
OVR_PUBLIC_FUNCTION(void)
ovr_DestroyMirrorTexture(ovrSession session, ovrMirrorTexture mirrorTexture);

/// Calculates the recommended viewport size for rendering a given eye within the HMD
/// with a given FOV cone.
///
/// Higher FOV will generally require larger textures to maintain quality.
/// Apps packing multiple eye views together on the same texture should ensure there are
/// at least 8 pixels of padding between them to prevent texture filtering and chromatic
/// aberration causing images to leak between the two eye views.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] eye Specifies which eye (left or right) to calculate for.
/// \param[in] fov Specifies the ovrFovPort to use.
/// \param[in] pixelsPerDisplayPixel Specifies the ratio of the number of render target pixels
///            to display pixels at the center of distortion. 1.0 is the default value. Lower
///            values can improve performance, higher values give improved quality.
///
/// <b>Example code</b>
///     \code{.cpp}
///         ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
///         ovrSizei eyeSizeLeft  = ovr_GetFovTextureSize(session, ovrEye_Left,
///         hmdDesc.DefaultEyeFov[ovrEye_Left],  1.0f);
///         ovrSizei eyeSizeRight = ovr_GetFovTextureSize(session, ovrEye_Right,
///         hmdDesc.DefaultEyeFov[ovrEye_Right], 1.0f);
///     \endcode
///
/// \return Returns the texture width and height size.
///
OVR_PUBLIC_FUNCTION(ovrSizei)
ovr_GetFovTextureSize(
    ovrSession session,
    ovrEyeType eye,
    ovrFovPort fov,
    float pixelsPerDisplayPixel);

/// Computes the distortion viewport, view adjust, and other rendering parameters for
/// the specified eye.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] eyeType Specifies which eye (left or right) for which to perform calculations.
/// \param[in] fov Specifies the ovrFovPort to use.
///
/// \return Returns the computed ovrEyeRenderDesc for the given eyeType and field of view.
///
/// \see ovrEyeRenderDesc
///
OVR_PUBLIC_FUNCTION(ovrEyeRenderDesc)
ovr_GetRenderDesc(ovrSession session, ovrEyeType eyeType, ovrFovPort fov);

/// Waits until surfaces are available and it is time to begin rendering the frame.  Must be
/// called before ovr_BeginFrame, but not necessarily from the same thread.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
///
/// \param[in] frameIndex Specifies the targeted application frame index.
///
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error and true
///         upon success. Return values include but aren't limited to:
///     - ovrSuccess: command completed successfully.
///     - ovrSuccess_NotVisible: rendering of a previous frame completed successfully but was not
///       displayed on the HMD, usually because another application currently has ownership of the
///       HMD. Applications receiving this result should stop rendering new content and call
///       ovr_GetSessionStatus to detect visibility.
///     - ovrError_DisplayLost: The session has become invalid (such as due to a device removal)
///       and the shared resources need to be released (ovr_DestroyTextureSwapChain), the session
///       needs to destroyed (ovr_Destroy) and recreated (ovr_Create), and new resources need to be
///       created (ovr_CreateTextureSwapChainXXX). The application's existing private graphics
///       resources do not need to be recreated unless the new ovr_Create call returns a different
///       GraphicsLuid.
///
/// \see ovr_BeginFrame, ovr_EndFrame, ovr_GetSessionStatus
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_WaitToBeginFrame(ovrSession session, long long frameIndex);

/// Called from render thread before application begins rendering.  Must be called after
/// ovr_WaitToBeginFrame and before ovr_EndFrame, but not necessarily from the same threads.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
///
/// \param[in] frameIndex Specifies the targeted application frame index.  It must match what was
///        passed to ovr_WaitToBeginFrame.
///
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error and true
///         upon success. Return values include but aren't limited to:
///     - ovrSuccess: command completed successfully.
///     - ovrError_DisplayLost: The session has become invalid (such as due to a device removal)
///       and the shared resources need to be released (ovr_DestroyTextureSwapChain), the session
///       needs to destroyed (ovr_Destroy) and recreated (ovr_Create), and new resources need to be
///       created (ovr_CreateTextureSwapChainXXX). The application's existing private graphics
///       resources do not need to be recreated unless the new ovr_Create call returns a different
///       GraphicsLuid.
///
/// \see ovr_WaitToBeginFrame, ovr_EndFrame
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_BeginFrame(ovrSession session, long long frameIndex);

/// Called from render thread after application has finished rendering.  Must be called after
/// ovr_BeginFrame, but not necessarily from the same thread.  Submits layers for distortion and
/// display, which will happen asynchronously.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
///
/// \param[in] frameIndex Specifies the targeted application frame index.  It must match what was
///        passed to ovr_BeginFrame.
///
/// \param[in] viewScaleDesc Provides additional information needed only if layerPtrList contains
///        an ovrLayerType_Quad. If NULL, a default version is used based on the current
///        configuration and a 1.0 world scale.
///
/// \param[in] layerPtrList Specifies a list of ovrLayer pointers, which can include NULL entries to
///        indicate that any previously shown layer at that index is to not be displayed.
///        Each layer header must be a part of a layer structure such as ovrLayerEyeFov or
///        ovrLayerQuad, with Header.Type identifying its type. A NULL layerPtrList entry in the
///        array indicates the absence of the given layer.
///
/// \param[in] layerCount Indicates the number of valid elements in layerPtrList. The maximum
///        supported layerCount is not currently specified, but may be specified in a future
///        version.
///
/// - Layers are drawn in the order they are specified in the array, regardless of the layer type.
///
/// - Layers are not remembered between successive calls to ovr_SubmitFrame. A layer must be
///   specified in every call to ovr_SubmitFrame or it won't be displayed.
///
/// - If a layerPtrList entry that was specified in a previous call to ovr_SubmitFrame is
///   passed as NULL or is of type ovrLayerType_Disabled, that layer is no longer displayed.
///
/// - A layerPtrList entry can be of any layer type and multiple entries of the same layer type
///   are allowed. No layerPtrList entry may be duplicated (i.e. the same pointer as an earlier
///   entry).
///
/// <b>Example code</b>
///     \code{.cpp}
///         ovrLayerEyeFov  layer0;
///         ovrLayerQuad    layer1;
///           ...
///         ovrLayerHeader* layers[2] = { &layer0.Header, &layer1.Header };
///         ovrResult result = ovr_EndFrame(session, frameIndex, nullptr, layers, 2);
///     \endcode
///
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error and true
///         upon success. Return values include but aren't limited to:
///     - ovrSuccess: rendering completed successfully.
///     - ovrError_DisplayLost: The session has become invalid (such as due to a device removal)
///       and the shared resources need to be released (ovr_DestroyTextureSwapChain), the session
///       needs to destroyed (ovr_Destroy) and recreated (ovr_Create), and new resources need to be
///       created (ovr_CreateTextureSwapChainXXX). The application's existing private graphics
///       resources do not need to be recreated unless the new ovr_Create call returns a different
///       GraphicsLuid.
///     - ovrError_TextureSwapChainInvalid: The ovrTextureSwapChain is in an incomplete or
///       inconsistent state. Ensure ovr_CommitTextureSwapChain was called at least once first.
///
/// \see ovr_WaitToBeginFrame, ovr_BeginFrame, ovrViewScaleDesc, ovrLayerHeader
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_EndFrame(
    ovrSession session,
    long long frameIndex,
    const ovrViewScaleDesc* viewScaleDesc,
    ovrLayerHeader const* const* layerPtrList,
    unsigned int layerCount);

/// Submits layers for distortion and display.
///
/// Deprecated.  Use ovr_WaitToBeginFrame, ovr_BeginFrame, and ovr_EndFrame instead.
///
/// ovr_SubmitFrame triggers distortion and processing which might happen asynchronously.
/// The function will return when there is room in the submission queue and surfaces
/// are available. Distortion might or might not have completed.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
///
/// \param[in] frameIndex Specifies the targeted application frame index, or 0 to refer to one frame
///        after the last time ovr_SubmitFrame was called.
///
/// \param[in] viewScaleDesc Provides additional information needed only if layerPtrList contains
///        an ovrLayerType_Quad. If NULL, a default version is used based on the current
///        configuration and a 1.0 world scale.
///
/// \param[in] layerPtrList Specifies a list of ovrLayer pointers, which can include NULL entries to
///        indicate that any previously shown layer at that index is to not be displayed.
///        Each layer header must be a part of a layer structure such as ovrLayerEyeFov or
///        ovrLayerQuad, with Header.Type identifying its type. A NULL layerPtrList entry in the
///        array indicates the absence of the given layer.
///
/// \param[in] layerCount Indicates the number of valid elements in layerPtrList. The maximum
///        supported layerCount is not currently specified, but may be specified in a future
///        version.
///
/// - Layers are drawn in the order they are specified in the array, regardless of the layer type.
///
/// - Layers are not remembered between successive calls to ovr_SubmitFrame. A layer must be
///   specified in every call to ovr_SubmitFrame or it won't be displayed.
///
/// - If a layerPtrList entry that was specified in a previous call to ovr_SubmitFrame is
///   passed as NULL or is of type ovrLayerType_Disabled, that layer is no longer displayed.
///
/// - A layerPtrList entry can be of any layer type and multiple entries of the same layer type
///   are allowed. No layerPtrList entry may be duplicated (i.e. the same pointer as an earlier
///   entry).
///
/// <b>Example code</b>
///     \code{.cpp}
///         ovrLayerEyeFov  layer0;
///         ovrLayerQuad    layer1;
///           ...
///         ovrLayerHeader* layers[2] = { &layer0.Header, &layer1.Header };
///         ovrResult result = ovr_SubmitFrame(session, frameIndex, nullptr, layers, 2);
///     \endcode
///
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error and true
///         upon success. Return values include but aren't limited to:
///     - ovrSuccess: rendering completed successfully.
///     - ovrSuccess_NotVisible: rendering completed successfully but was not displayed on the HMD,
///       usually because another application currently has ownership of the HMD. Applications
///       receiving this result should stop rendering new content, call ovr_GetSessionStatus
///       to detect visibility.
///     - ovrError_DisplayLost: The session has become invalid (such as due to a device removal)
///       and the shared resources need to be released (ovr_DestroyTextureSwapChain), the session
///       needs to destroyed (ovr_Destroy) and recreated (ovr_Create), and new resources need to be
///       created (ovr_CreateTextureSwapChainXXX). The application's existing private graphics
///       resources do not need to be recreated unless the new ovr_Create call returns a different
///       GraphicsLuid.
///     - ovrError_TextureSwapChainInvalid: The ovrTextureSwapChain is in an incomplete or
///       inconsistent state. Ensure ovr_CommitTextureSwapChain was called at least once first.
///
/// \see ovr_GetPredictedDisplayTime, ovrViewScaleDesc, ovrLayerHeader, ovr_GetSessionStatus
///
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_SubmitFrame(
    ovrSession session,
    long long frameIndex,
    const ovrViewScaleDesc* viewScaleDesc,
    ovrLayerHeader const* const* layerPtrList,
    unsigned int layerCount);
///@}

#endif // !defined(OVR_EXPORTING_CAPI)

//-------------------------------------------------------------------------------------
/// @name Frame Timing
///
//@{

///
/// Contains the performance stats for a given SDK compositor frame
///
/// All of the 'int' typed fields can be reset via the ovr_ResetPerfStats call.
///
typedef struct OVR_ALIGNAS(4) ovrPerfStatsPerCompositorFrame_ {
  /// Vsync Frame Index - increments with each HMD vertical synchronization signal (i.e. vsync or
  /// refresh rate)
  /// If the compositor drops a frame, expect this value to increment more than 1 at a time.
  int HmdVsyncIndex;

  ///
  /// Application stats
  ///

  /// Index that increments with each successive ovr_SubmitFrame call
  int AppFrameIndex;

  /// If the app fails to call ovr_SubmitFrame on time, then expect this value to increment with
  /// each missed frame
  int AppDroppedFrameCount;

  /// Motion-to-photon latency for the application
  /// This value is calculated by either using the SensorSampleTime provided for the ovrLayerEyeFov
  /// or if that
  /// is not available, then the call to ovr_GetTrackingState which has latencyMarker set to ovrTrue
  float AppMotionToPhotonLatency;

  /// Amount of queue-ahead in seconds provided to the app based on performance and overlap of
  /// CPU and  GPU utilization. A value of 0.0 would mean the CPU & GPU workload is being completed
  /// in 1 frame's worth of time, while 11 ms (on the CV1) of queue ahead would indicate that the
  /// app's CPU workload for the next frame is overlapping the GPU workload for the current frame.
  float AppQueueAheadTime;

  /// Amount of time in seconds spent on the CPU by the app's render-thread that calls
  /// ovr_SubmitFram.  Measured as elapsed time between from when app regains control from
  /// ovr_SubmitFrame to the next time the app calls ovr_SubmitFrame.
  float AppCpuElapsedTime;

  /// Amount of time in seconds spent on the GPU by the app.
  /// Measured as elapsed time between each ovr_SubmitFrame call using GPU timing queries.
  float AppGpuElapsedTime;

  ///
  /// SDK Compositor stats
  ///

  /// Index that increments each time the SDK compositor completes a distortion and timewarp pass
  /// Since the compositor operates asynchronously, even if the app calls ovr_SubmitFrame too late,
  /// the compositor will kick off for each vsync.
  int CompositorFrameIndex;

  /// Increments each time the SDK compositor fails to complete in time
  /// This is not tied to the app's performance, but failure to complete can be related to other
  /// factors such as OS capabilities, overall available hardware cycles to execute the compositor
  /// in time and other factors outside of the app's control.
  int CompositorDroppedFrameCount;

  /// Motion-to-photon latency of the SDK compositor in seconds.
  /// This is the latency of timewarp which corrects the higher app latency as well as dropped app
  /// frames.
  float CompositorLatency;

  /// The amount of time in seconds spent on the CPU by the SDK compositor. Unless the
  /// VR app is utilizing all of the CPU cores at their peak performance, there is a good chance the
  /// compositor CPU times will not affect the app's CPU performance in a major way.
  float CompositorCpuElapsedTime;

  /// The amount of time in seconds spent on the GPU by the SDK compositor. Any time spent on the
  /// compositor will eat away from the available GPU time for the app.
  float CompositorGpuElapsedTime;

  /// The amount of time in seconds spent from the point the CPU kicks off the compositor to the
  /// point in time the compositor completes the distortion & timewarp on the GPU. In the event the
  /// GPU time is not available, expect this value to be -1.0f.
  float CompositorCpuStartToGpuEndElapsedTime;

  /// The amount of time in seconds left after the compositor is done on the GPU to the associated
  /// V-Sync time. In the event the GPU time is not available, expect this value to be -1.0f.
  float CompositorGpuEndToVsyncElapsedTime;

  ///
  /// Async Spacewarp stats (ASW)
  ///

  /// Will be true if ASW is active for the given frame such that the application is being forced
  /// into half the frame-rate while the compositor continues to run at full frame-rate.
  ovrBool AswIsActive;

  /// Increments each time ASW it activated where the app was forced in and out of
  /// half-rate rendering.
  int AswActivatedToggleCount;

  /// Accumulates the number of frames presented by the compositor which had extrapolated
  /// ASW frames presented.
  int AswPresentedFrameCount;

  /// Accumulates the number of frames that the compositor tried to present when ASW is
  /// active but failed.
  int AswFailedFrameCount;

} ovrPerfStatsPerCompositorFrame;

///
/// Maximum number of frames of performance stats provided back to the caller of ovr_GetPerfStats
///
enum { ovrMaxProvidedFrameStats = 5 };

///
/// This is a complete descriptor of the performance stats provided by the SDK
///
/// \see ovr_GetPerfStats, ovrPerfStatsPerCompositorFrame
typedef struct OVR_ALIGNAS(4) ovrPerfStats_ {
  /// FrameStatsCount will have a maximum value set by ovrMaxProvidedFrameStats
  /// If the application calls ovr_GetPerfStats at the native refresh rate of the HMD
  /// then FrameStatsCount will be 1. If the app's workload happens to force
  /// ovr_GetPerfStats to be called at a lower rate, then FrameStatsCount will be 2 or more.
  /// If the app does not want to miss any performance data for any frame, it needs to
  /// ensure that it is calling ovr_SubmitFrame and ovr_GetPerfStats at a rate that is at least:
  /// "HMD_refresh_rate / ovrMaxProvidedFrameStats". On the Oculus Rift CV1 HMD, this will
  /// be equal to 18 times per second.
  ///
  /// The performance entries will be ordered in reverse chronological order such that the
  /// first entry will be the most recent one.
  ovrPerfStatsPerCompositorFrame FrameStats[ovrMaxProvidedFrameStats];
  int FrameStatsCount;

  /// If the app calls ovr_GetPerfStats at less than 18 fps for CV1, then AnyFrameStatsDropped
  /// will be ovrTrue and FrameStatsCount will be equal to ovrMaxProvidedFrameStats.
  ovrBool AnyFrameStatsDropped;

  /// AdaptiveGpuPerformanceScale is an edge-filtered value that a caller can use to adjust
  /// the graphics quality of the application to keep the GPU utilization in check. The value
  /// is calculated as: (desired_GPU_utilization / current_GPU_utilization)
  /// As such, when this value is 1.0, the GPU is doing the right amount of work for the app.
  /// Lower values mean the app needs to pull back on the GPU utilization.
  /// If the app is going to directly drive render-target resolution using this value, then
  /// be sure to take the square-root of the value before scaling the resolution with it.
  /// Changing render target resolutions however is one of the many things an app can do
  /// increase or decrease the amount of GPU utilization.
  /// Since AdaptiveGpuPerformanceScale is edge-filtered and does not change rapidly
  /// (i.e. reports non-1.0 values once every couple of seconds) the app can make the
  /// necessary adjustments and then keep watching the value to see if it has been satisfied.
  float AdaptiveGpuPerformanceScale;

  /// Will be true if Async Spacewarp (ASW) is available for this system which is dependent on
  /// several factors such as choice of GPU, OS and debug overrides
  ovrBool AswIsAvailable;

  /// Contains the Process ID of the VR application the stats are being polled for
  /// If an app continues to grab perf stats even when it is not visible, then expect this
  /// value to point to the other VR app that has grabbed focus (i.e. became visible)
  ovrProcessId VisibleProcessId;
} ovrPerfStats;

#if !defined(OVR_EXPORTING_CAPI)

/// Retrieves performance stats for the VR app as well as the SDK compositor.
///
/// This function will return stats for the VR app that is currently visible in the HMD
/// regardless of what VR app is actually calling this function.
///
/// If the VR app is trying to make sure the stats returned belong to the same application,
/// the caller can compare the VisibleProcessId with their own process ID. Normally this will
/// be the case if the caller is only calling ovr_GetPerfStats when ovr_GetSessionStatus has
/// IsVisible flag set to be true.
///
/// If the VR app calling ovr_GetPerfStats is actually the one visible in the HMD,
/// then new perf stats will only be populated after a new call to ovr_SubmitFrame.
/// That means subsequent calls to ovr_GetPerfStats after the first one without calling
/// ovr_SubmitFrame will receive a FrameStatsCount of zero.
///
/// If the VR app is not visible, or was initially marked as ovrInit_Invisible, then each call
/// to ovr_GetPerfStats will immediately fetch new perf stats from the compositor without
/// a need for the ovr_SubmitFrame call.
///
/// Even though invisible VR apps do not require ovr_SubmitFrame to be called to gather new
/// perf stats, since stats are generated at the native refresh rate of the HMD (i.e. 90 Hz
/// for CV1), calling it at a higher rate than that would be unnecessary.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[out] outStats Contains the performance stats for the application and SDK compositor
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error and true
///         upon success.
///
/// \see ovrPerfStats, ovrPerfStatsPerCompositorFrame, ovr_ResetPerfStats
///
OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetPerfStats(ovrSession session, ovrPerfStats* outStats);

/// Resets the accumulated stats reported in each ovrPerfStatsPerCompositorFrame back to zero.
///
/// Only the integer values such as HmdVsyncIndex, AppDroppedFrameCount etc. will be reset
/// as the other fields such as AppMotionToPhotonLatency are independent timing values updated
/// per-frame.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \return Returns an ovrResult for which OVR_SUCCESS(result) is false upon error and true
///         upon success.
///
/// \see ovrPerfStats, ovrPerfStatsPerCompositorFrame, ovr_GetPerfStats
///
OVR_PUBLIC_FUNCTION(ovrResult) ovr_ResetPerfStats(ovrSession session);

/// Gets the time of the specified frame midpoint.
///
/// Predicts the time at which the given frame will be displayed. The predicted time
/// is the middle of the time period during which the corresponding eye images will
/// be displayed.
///
/// The application should increment frameIndex for each successively targeted frame,
/// and pass that index to any relevant OVR functions that need to apply to the frame
/// identified by that index.
///
/// This function is thread-safe and allows for multiple application threads to target
/// their processing to the same displayed frame.
///
/// In the even that prediction fails due to various reasons (e.g. the display being off
/// or app has yet to present any frames), the return value will be current CPU time.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] frameIndex Identifies the frame the caller wishes to target.
///            A value of zero returns the next frame index.
/// \return Returns the absolute frame midpoint time for the given frameIndex.
/// \see ovr_GetTimeInSeconds
///
OVR_PUBLIC_FUNCTION(double) ovr_GetPredictedDisplayTime(ovrSession session, long long frameIndex);

/// Returns global, absolute high-resolution time in seconds.
///
/// The time frame of reference for this function is not specified and should not be
/// depended upon.
///
/// \return Returns seconds as a floating point value.
/// \see ovrPoseStatef, ovrFrameTiming
///
OVR_PUBLIC_FUNCTION(double) ovr_GetTimeInSeconds();

#endif // !defined(OVR_EXPORTING_CAPI)

/// Performance HUD enables the HMD user to see information critical to
/// the real-time operation of the VR application such as latency timing,
/// and CPU & GPU performance metrics
///
///     App can toggle performance HUD modes as such:
///     \code{.cpp}
///         ovrPerfHudMode PerfHudMode = ovrPerfHud_LatencyTiming;
///         ovr_SetInt(session, OVR_PERF_HUD_MODE, (int)PerfHudMode);
///     \endcode
///
typedef enum ovrPerfHudMode_ {
  ovrPerfHud_Off = 0, ///< Turns off the performance HUD
  ovrPerfHud_PerfSummary = 1, ///< Shows performance summary and headroom
  ovrPerfHud_LatencyTiming = 2, ///< Shows latency related timing info
  ovrPerfHud_AppRenderTiming = 3, ///< Shows render timing info for application
  ovrPerfHud_CompRenderTiming = 4, ///< Shows render timing info for OVR compositor
  ovrPerfHud_AswStats = 6, ///< Shows Async Spacewarp-specific info
  ovrPerfHud_VersionInfo = 5, ///< Shows SDK & HMD version Info
  ovrPerfHud_Count = 7, ///< \internal Count of enumerated elements.
  ovrPerfHud_EnumSize = 0x7fffffff ///< \internal Force type int32_t.
} ovrPerfHudMode;

/// Layer HUD enables the HMD user to see information about a layer
///
///     App can toggle layer HUD modes as such:
///     \code{.cpp}
///         ovrLayerHudMode LayerHudMode = ovrLayerHud_Info;
///         ovr_SetInt(session, OVR_LAYER_HUD_MODE, (int)LayerHudMode);
///     \endcode
///
typedef enum ovrLayerHudMode_ {
  ovrLayerHud_Off = 0, ///< Turns off the layer HUD
  ovrLayerHud_Info = 1, ///< Shows info about a specific layer
  ovrLayerHud_EnumSize = 0x7fffffff
} ovrLayerHudMode;

///@}

/// Debug HUD is provided to help developers gauge and debug the fidelity of their app's
/// stereo rendering characteristics. Using the provided quad and crosshair guides,
/// the developer can verify various aspects such as VR tracking units (e.g. meters),
/// stereo camera-parallax properties (e.g. making sure objects at infinity are rendered
/// with the proper separation), measuring VR geometry sizes and distances and more.
///
///     App can toggle the debug HUD modes as such:
///     \code{.cpp}
///         ovrDebugHudStereoMode DebugHudMode = ovrDebugHudStereo_QuadWithCrosshair;
///         ovr_SetInt(session, OVR_DEBUG_HUD_STEREO_MODE, (int)DebugHudMode);
///     \endcode
///
/// The app can modify the visual properties of the stereo guide (i.e. quad, crosshair)
/// using the ovr_SetFloatArray function. For a list of tweakable properties,
/// see the OVR_DEBUG_HUD_STEREO_GUIDE_* keys in the OVR_CAPI_Keys.h header file.
typedef enum ovrDebugHudStereoMode_ {
  /// Turns off the Stereo Debug HUD.
  ovrDebugHudStereo_Off = 0,

  /// Renders Quad in world for Stereo Debugging.
  ovrDebugHudStereo_Quad = 1,

  /// Renders Quad+crosshair in world for Stereo Debugging
  ovrDebugHudStereo_QuadWithCrosshair = 2,

  /// Renders screen-space crosshair at infinity for Stereo Debugging
  ovrDebugHudStereo_CrosshairAtInfinity = 3,

  /// \internal Count of enumerated elements
  ovrDebugHudStereo_Count,

  ovrDebugHudStereo_EnumSize = 0x7fffffff ///< \internal Force type int32_t
} ovrDebugHudStereoMode;


#if !defined(OVR_EXPORTING_CAPI)

// -----------------------------------------------------------------------------------
/// @name Mixed reality capture support
///
/// Defines functions used for mixed reality capture / third person cameras.
///
//@{

/// Returns the number of camera properties of all cameras
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in out] cameras Pointer to the array. If null and the provided array capacity is
/// sufficient, will return ovrError_NullArrayPointer. \param[in out] inoutCameraCount Supply the
/// array capacity, will return the actual # of cameras defined. If *inoutCameraCount is too small,
/// will return ovrError_InsufficientArraySize. \return Returns the ids of external cameras the
/// system knows about. Returns
///            ovrError_NoExternalCameraInfo if there is not any eternal camera information.
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetExternalCameras(
    ovrSession session,
    ovrExternalCamera* cameras,
    unsigned int* inoutCameraCount);

/// Sets the camera intrinsics and/or extrinsics stored for the cameraName camera
/// Names must be < 32 characters and null-terminated.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] name Specifies which camera to set the intrinsics or extrinsics for
/// \param[in] intrinsics Contains the intrinsic parameters to set, can be null
/// \param[in] extrinsics Contains the extrinsic parameters to set, can be null
/// \return Returns ovrSuccess or an ovrError code
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_SetExternalCameraProperties(
    ovrSession session,
    const char* name,
    const ovrCameraIntrinsics* const intrinsics,
    const ovrCameraExtrinsics* const extrinsics);

///@}

#endif // OVR_EXPORTING_CAPI

#if !defined(OVR_EXPORTING_CAPI)

// -----------------------------------------------------------------------------------
/// @name Property Access
///
/// These functions read and write OVR properties. Supported properties
/// are defined in OVR_CAPI_Keys.h
///
//@{

/// Reads a boolean property.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] propertyName The name of the property, which needs to be valid for only the call.
/// \param[in] defaultVal specifes the value to return if the property couldn't be read.
/// \return Returns the property interpreted as a boolean value. Returns defaultVal if
///         the property doesn't exist.
OVR_PUBLIC_FUNCTION(ovrBool)
ovr_GetBool(ovrSession session, const char* propertyName, ovrBool defaultVal);

/// Writes or creates a boolean property.
/// If the property wasn't previously a boolean property, it is changed to a boolean property.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] propertyName The name of the property, which needs to be valid only for the call.
/// \param[in] value The value to write.
/// \return Returns true if successful, otherwise false. A false result should only occur if the
/// property
///         name is empty or if the property is read-only.
OVR_PUBLIC_FUNCTION(ovrBool)
ovr_SetBool(ovrSession session, const char* propertyName, ovrBool value);

/// Reads an integer property.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] propertyName The name of the property, which needs to be valid only for the call.
/// \param[in] defaultVal Specifes the value to return if the property couldn't be read.
/// \return Returns the property interpreted as an integer value. Returns defaultVal if
///         the property doesn't exist.
OVR_PUBLIC_FUNCTION(int) ovr_GetInt(ovrSession session, const char* propertyName, int defaultVal);

/// Writes or creates an integer property.
///
/// If the property wasn't previously a boolean property, it is changed to an integer property.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] propertyName The name of the property, which needs to be valid only for the call.
/// \param[in] value The value to write.
/// \return Returns true if successful, otherwise false. A false result should only occur if the
///         property name is empty or if the property is read-only.
OVR_PUBLIC_FUNCTION(ovrBool) ovr_SetInt(ovrSession session, const char* propertyName, int value);

/// Reads a float property.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] propertyName The name of the property, which needs to be valid only for the call.
/// \param[in] defaultVal specifes the value to return if the property couldn't be read.
/// \return Returns the property interpreted as an float value. Returns defaultVal if
///         the property doesn't exist.
OVR_PUBLIC_FUNCTION(float)
ovr_GetFloat(ovrSession session, const char* propertyName, float defaultVal);

/// Writes or creates a float property.
/// If the property wasn't previously a float property, it's changed to a float property.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] propertyName The name of the property, which needs to be valid only for the call.
/// \param[in] value The value to write.
/// \return Returns true if successful, otherwise false. A false result should only occur if the
///         property name is empty or if the property is read-only.
OVR_PUBLIC_FUNCTION(ovrBool)
ovr_SetFloat(ovrSession session, const char* propertyName, float value);

/// Reads a float array property.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] propertyName The name of the property, which needs to be valid only for the call.
/// \param[in] values An array of float to write to.
/// \param[in] valuesCapacity Specifies the maximum number of elements to write to the values array.
/// \return Returns the number of elements read, or 0 if property doesn't exist or is empty.
OVR_PUBLIC_FUNCTION(unsigned int)
ovr_GetFloatArray(
    ovrSession session,
    const char* propertyName,
    float values[],
    unsigned int valuesCapacity);

/// Writes or creates a float array property.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] propertyName The name of the property, which needs to be valid only for the call.
/// \param[in] values An array of float to write from.
/// \param[in] valuesSize Specifies the number of elements to write.
/// \return Returns true if successful, otherwise false. A false result should only occur if the
///         property name is empty or if the property is read-only.
OVR_PUBLIC_FUNCTION(ovrBool)
ovr_SetFloatArray(
    ovrSession session,
    const char* propertyName,
    const float values[],
    unsigned int valuesSize);

/// Reads a string property.
/// Strings are UTF8-encoded and null-terminated.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] propertyName The name of the property, which needs to be valid only for the call.
/// \param[in] defaultVal Specifes the value to return if the property couldn't be read.
/// \return Returns the string property if it exists. Otherwise returns defaultVal, which can be
///         specified as NULL. The return memory is guaranteed to be valid until next call to
///         ovr_GetString or until the session is destroyed, whichever occurs first.
OVR_PUBLIC_FUNCTION(const char*)
ovr_GetString(ovrSession session, const char* propertyName, const char* defaultVal);

/// Writes or creates a string property.
/// Strings are UTF8-encoded and null-terminated.
///
/// \param[in] session Specifies an ovrSession previously returned by ovr_Create.
/// \param[in] propertyName The name of the property, which needs to be valid only for the call.
/// \param[in] value The string property, which only needs to be valid for the duration of the call.
/// \return Returns true if successful, otherwise false. A false result should only occur if the
///         property name is empty or if the property is read-only.
OVR_PUBLIC_FUNCTION(ovrBool)
ovr_SetString(ovrSession session, const char* propertyName, const char* value);

///@}

#endif // !defined(OVR_EXPORTING_CAPI)

#ifdef __cplusplus
} // extern "C"
#endif

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

/// @cond DoxygenIgnore


// -----------------------------------------------------------------------------------
// ***** Backward compatibility #includes
//
// This is at the bottom of this file because the following is dependent on the
// declarations above.

#if !defined(OVR_CAPI_NO_UTILS)
#include "Extras/OVR_CAPI_Util.h"
#endif

/// @endcond

#endif // OVR_CAPI_h

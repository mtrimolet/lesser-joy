module;

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>

export module lesserjoy.bluetooth:guids;

extern "C" {

extern __declspec(selectany) const auto
  LESSERJOY_BLUETOOTH_SERVICE_NAME = L"lesser-joy bluetooth service";

DEFINE_GUID(LESSERJOY_BLUETOOTH_SERVICE_GUID,          /* fc71b33d-d528-4763-a86c-78777c7bcd7b */
  0xfc71b33d, 0xd528, 0x4763, 0xa8, 0x6c, 0x78, 0x77, 0x7c, 0x7b, 0xcd, 0x7b);

// TODO replace with real value
DEFINE_GUID(LESSERJOY_BLUETOOTH_DEVIFACE_GUID, /* fc71b33d-d528-4763-a86c-78777c7bcd7b */
  0xfc71b33d, 0xd528, 0x4763, 0xa8, 0x6c, 0x78, 0x77, 0x7c, 0x7b, 0xcd, 0x7b);


//
// Lowest HCI major version to support
// 
#define LESSERJOY_BLUETOOTH_MIN_SUPPORTED_HCI_MAJOR_VERSION 0x03 // Bluetooth 2.0 + EDR
/*
 * Link Manager Versions
 * 
 * | LMP | Bluetooth Version   |
 * | --- | ------------------- |
 * | 0   | Bluetooth 1.0b      |
 * | 1   | Bluetooth 1.1       |
 * | 2   | Bluetooth 1.2       |
 * | 3   | Bluetooth 2.0 + EDR |
 * | 4   | Bluetooth 2.1 + EDR |
 * | 5   | Bluetooth 3.0 + HS  |
 * | 6   | Bluetooth 4.0       |
 * | 7   | Bluetooth 4.1       |
 * | 8   | Bluetooth 4.2       |
 * | 9   | Bluetooth 5         |
 * | 10  | Bluetooth 5.1       |
 * | 11  | Bluetooth 5.2       |
 * 
 */

extern __declspec(selectany) const auto
  LESSERJOY_BLUETOOTH_ENUMERATOR_NAME = L"lesser-joy bluetooth enumerator";

// Pro Controller

static const USHORT LESSERJOY_BLUETOOTH_PROCONTROLLER_VID = 0x054C; // TODO replace with real value
static const USHORT LESSERJOY_BLUETOOTH_PROCONTROLLER_PID = 0x0268; // TODO replace with real value

// TODO replace with real value
DEFINE_GUID(LESSERJOY_BLUETOOTH_PROCONTROLLER_ENUM_GUID, /* 53f88889-1aaf-4353-a047-556b69ec6da6 */
  0x53f88889, 0x1aaf, 0x4353, 0xa0, 0x47, 0x55, 0x6b, 0x69, 0xec, 0x6d, 0xa6);

// TODO replace with real value
DEFINE_GUID(LESSERJOY_BLUETOOTH_PROCONTROLLER_DEVCLASS_GUID, /* 2ffad411-8a38-4a36-957a-c2e2d769be62 */
  0x2ffad411, 0x8a38, 0x4a36, 0x95, 0x7a, 0xc2, 0xe2, 0xd7, 0x69, 0xbe, 0x62);

// TODO replace with real value
DEFINE_GUID(LESSERJOY_BLUETOOTH_PROCONTROLLER_DEVIFACE_GUID, /* 7b0eae3d-4414-4024-bcbd-1c21523768ce */
  0x7b0eae3d, 0x4414, 0x4024, 0xbc, 0xbd, 0x1c, 0x21, 0x52, 0x37, 0x68, 0xce);



// /** [Service] Handle=0x0001 Type=00c5af5d-1964-4e30-8f51-1956f96bd280(Service Control) **/
// DEFINE_GUID(SERVICE_CONTROL,        0x00c5af5d, 0x1964, 0x4e30, 0x8f, 0x51, 0x19, 0x56, 0xf9, 0x6b, 0xd2, 0x80);
// extern __declspec(selectany) const CWSTR* SERVICE_CONTROL_NAME = L"Service Control";
// /**     [Characteristic] Handle=0x0002 ValueHandle=0x0003 Type=00c5af5d-1964-4e30-8f51-1956f96bd281 Properties=(Read) */
// // DEFINE_GUID(UNKNOWN_1956f96bd281,   0x00c5af5d, 0x1964, 0x4e30, 0x8f, 0x51, 0x19, 0x56, 0xf9, 0x6b, 0xd2, 0x81);
// /**     [Characteristic] Handle=0x0004 ValueHandle=0x0005 Type=00c5af5d-1964-4e30-8f51-1956f96bd282(Service Enable) Properties=(Write) */
// DEFINE_GUID(SERVICE_ENABLE,         0x00c5af5d, 0x1964, 0x4e30, 0x8f, 0x51, 0x19, 0x56, 0xf9, 0x6b, 0xd2, 0x82);
// extern __declspec(selectany) const CWSTR* SERVICE_ENABLE_NAME  = L"Service Enable";
// /**     [Characteristic] Handle=0x0006 ValueHandle=0x0007 Type=00c5af5d-1964-4e30-8f51-1956f96bd283 Properties=(Read) */
// // DEFINE_GUID(UNKNOWN_1956f96bd283,   0x00c5af5d, 0x1964, 0x4e30, 0x8f, 0x51, 0x19, 0x56, 0xf9, 0x6b, 0xd2, 0x83);

/** [Service] Handle=0x0008 Type=ab7de9be-89fe-49ad-828f-118f09df7fd0(Nintendo SW2) */
DEFINE_GUID(NINTENDO_SW2,      0xab7de9be, 0x89fe, 0x49ad, 0x82, 0x8f, 0x11, 0x8f, 0x09, 0xdf, 0x7f, 0xd0);
extern __declspec(selectany) const auto
  LESSERJOY_BLUETOOTH_NINTENDO_SW2_NAME = L"Nintendo SW2";
// /**     [Characteristic] Handle=0x0009 ValueHandle=0x000a Type=ab7de9be-89fe-49ad-828f-118f09df7fd2(Input Report (format 0)) Properties=(Read/Notify) */
// DEFINE_GUID(INPUT_REPORT_F0,        0xab7de9be, 0x89fe, 0x49ad, 0x82, 0x8f, 0x11, 0x8f, 0x09, 0xdf, 0x7f, 0xd2);
// extern __declspec(selectany) const CWSTR* INPUT_REPORT_F0_NAME = L"Input report (format 0)";
// /**         [Value] [040700000000000000004E78869A987D000000000000000000000000000000D40D000000000000000001000000000000000000000000000000000000000000] */
// /**         [Descriptor]  Handle=0x000b Type=0x2902(Client Configuration) */
// /**             [Value]  IsSubscribeToNotification */
// /**         [Descriptor]  Handle=0x000c Type=679d5510-5a24-4dee-9557-95df80486ecb */
// // DEFINE_GUID(UNKNOWN_95df80486ecb,   0x679d5510, 0x5a24, 0x4dee, 0x95, 0x57, 0x95, 0xdf, 0x80, 0x48, 0x6e, 0xcb);
// /**     [Characteristic] Handle=0x000d ValueHandle=0x000e Type=7492866c-ec3e-4619-8258-32755ffcc0f8(Input Report (format 3)) Properties=(Read/Notify) */
// DEFINE_GUID(INPUT_REPORT_F3,        0x7492866c, 0xec3e, 0x4619, 0x82, 0x58, 0x32, 0x75, 0x5f, 0xfc, 0xc0, 0xf8);
// extern __declspec(selectany) const CWSTR* INPUT_REPORT_F3_NAME = L"Input report (format 3)";
// /**         [Value] [C41C00000046588599A87D30000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000] */
// /**         [Descriptor]  Handle=0x000f Type=0x2902(Client Configuration) */
// /**             [Value]  IsSubscribeToNotification */
// /**         [Descriptor]  Handle=0x0010 Type=679d5510-5a24-4dee-9557-95df80486ecb */
// // DEFINE_GUID(UNKNOWN_95df80486ecb,   0x679d5510, 0x5a24, 0x4dee, 0x95, 0x57, 0x95, 0xdf, 0x80, 0x48, 0x6e, 0xcb);
// /**     [Characteristic] Handle=0x0011 ValueHandle=0x0012 Type=cc483f51-9258-427d-a939-630c31f72b05(Vibration/rumble output) Properties=(WriteWithoutResponse) */
// DEFINE_GUID(RUMBLE_OUTPUT,          0xcc483f51, 0x9258, 0x427d, 0xa9, 0x39, 0x63, 0x0c, 0x31, 0xf7, 0x2b, 0x05);
// extern __declspec(selectany) const CWSTR* RUMBLE_OUTPUT_NAME = L"Vibration/rumble output";
// /**     [Characteristic] Handle=0x0013 ValueHandle=0x0014 Type=649d4ac9-8eb7-4e6c-af44-1ea54fe5f005(Command channel) Properties=(WriteWithoutResponse) */
// DEFINE_GUID(COMMAND_CHANNEL,        0x649d4ac9, 0x8eb7, 0x4e6c, 0xaf, 0x44, 0x1e, 0xa5, 0x4f, 0xe5, 0xf0, 0x05);
// extern __declspec(selectany) const CWSTR* COMMAND_CHANNEL_NAME = L"Command channel";
// /**     [Characteristic] Handle=0x0015 ValueHandle=0x0016 Type=3dacbc7e-6955-40b5-8eaf-6f9809e8b379(Command + rumble prefix channel) Properties=(WriteWithoutResponse) */
// DEFINE_GUID(COMMAND_RUMBLE_CHANNEL, 0x3dacbc7e, 0x6955, 0x40b5, 0x8e, 0xaf, 0x6f, 0x98, 0x09, 0xe8, 0xb3, 0x79);
// extern __declspec(selectany) const CWSTR* COMMAND_RUMBLE_CHANNEL_NAME = L"Command + rumble prefix channel";
// /**     [Characteristic] Handle=0x0017 ValueHandle=0x0018 Type=4147423d-fdae-4df7-a4f7-d23e5df59f8d Properties=(WriteWithoutResponse) */
// // DEFINE_GUID(UNKNOWN_d23e5df59f8d,   0x4147423d, 0xfdae, 0x4df7, 0xa4, 0xf7, 0xd2, 0x3e, 0x5d, 0xf5, 0x9f, 0x8d);
// /**     [Characteristic] Handle=0x0019 ValueHandle=0x001a Type=c765a961-d9d8-4d36-a20a-5315b111836a(Command response/ACK) Properties=(Notify) */
// DEFINE_GUID(COMMAND_RESPONSE_ACK,   0xc765a961, 0xd9d8, 0x4d36, 0xa2, 0x0a, 0x53, 0x15, 0xb1, 0x11, 0x83, 0x6a);
// extern __declspec(selectany) const CWSTR* COMMAND_RESPONSE_ACK_NAME = L"Command response/ACK";
// /**         [Descriptor]  Handle=0x001b Type=0x2902(Client Configuration) */
// /**             [Value]  IsSubscribeToNotification */
// /**         [Descriptor]  Handle=0x001c Type=b746df8c-f358-495b-9cd2-e3bbeda4f979 */
// // DEFINE_GUID(UNKNOWN_e3bbeda4f979,   0xb746df8c, 0xf358, 0x495b, 0x9c, 0xd2, 0xe3, 0xbb, 0xed, 0xa4, 0xf9, 0x79);
// /**     [Characteristic] Handle=0x001d ValueHandle=0x001e Type=506d9f7d-4278-4e95-a549-326ba77657e0 Properties=(Notify) */
// // DEFINE_GUID(UNKNOWN_326ba77657e0,   0x506d9f7d, 0x4278, 0x4e95, 0xa5, 0x49, 0x32, 0x6b, 0xa7, 0x76, 0x57, 0xe0);
// /**         [Descriptor]  Handle=0x001f Type=0x2902(Client Configuration) */
// /**             [Value]  IsSubscribeToNotification */
// /**         [Descriptor]  Handle=0x0020 Type=b746df8c-f358-495b-9cd2-e3bbeda4f979 */
// // DEFINE_GUID(UNKNOWN_e3bbeda4f979,   0xb746df8c, 0xf358, 0x495b, 0x9c, 0xd2, 0xe3, 0xbb, 0xed, 0xa4, 0xf9, 0x79);
// /**     [Characteristic] Handle=0x0021 ValueHandle=0x0022 Type=d3bd69d2-841c-4241-ab15-f86f406d2a80 Properties=(Notify) */
// // DEFINE_GUID(UNKNOWN_f86f406d2a80,   0xd3bd69d2, 0x841c, 0x4241, 0xab, 0x15, 0xf8, 0x6f, 0x40, 0x6d, 0x2a, 0x80);
// /**         [Descriptor]  Handle=0x0023 Type=0x2902(Client Configuration) */
// /**             [Value]  IsSubscribeToNotification */
// /**         [Descriptor]  Handle=0x0024 Type=b746df8c-f358-495b-9cd2-e3bbeda4f979 */
// // DEFINE_GUID(UNKNOWN_e3bbeda4f979,   0xb746df8c, 0xf358, 0x495b, 0x9c, 0xd2, 0xe3, 0xbb, 0xed, 0xa4, 0xf9, 0x79);
// /**     [Characteristic] Handle=0x0025 ValueHandle=0x0026 Type=ab7de9be-89fe-49ad-828f-118f09df7fde Properties=(Read/Notify) */
// // DEFINE_GUID(UNKNOWN_118f09df7fde,   0xab7de9be, 0x89fe, 0x49ad, 0x82, 0x8f, 0x11, 0x8f, 0x09, 0xdf, 0x7f, 0xde);
// /**         [Descriptor]  Handle=0x0027 Type=0x2902(Client Configuration) */
// /**             [Value]  IsSubscribeToNotification */
// /**         [Descriptor]  Handle=0x0028 Type=679d5510-5a24-4dee-9557-95df80486ecb */
// // DEFINE_GUID(UNKNOWN_95df80486ecb,   0x679d5510, 0x5a24, 0x4dee, 0x95, 0x57, 0x95, 0xdf, 0x80, 0x48, 0x6e, 0xcb);
// /**     [Characteristic] Handle=0x0029 ValueHandle=0x002a Type=ab7de9be-89fe-49ad-828f-118f09df7fdf Properties=(WriteWithoutResponse) */
// // DEFINE_GUID(UNKNOWN_118f09df7fdf,   0xab7de9be, 0x89fe, 0x49ad, 0x82, 0x8f, 0x11, 0x8f, 0x09, 0xdf, 0x7f, 0xdf);

} // extern "C"

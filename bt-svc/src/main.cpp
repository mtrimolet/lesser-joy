
#include <simpleble/SimpleBLE.h>
#include <stormkit/core/try_expected.hpp>
#include <stormkit/core/tryx_expected.hpp>

import std;
import frozen;
import stormkit.core;

namespace stk  = stormkit;
namespace stdr = std::ranges;

using hrclock = std::chrono::high_resolution_clock;
using namespace std::chrono_literals;
using namespace stk::literals;

template <class Callback>
struct Defered { Callback cb; ~Defered() { cb(); } };


enum class BLEException {
    NO_ADAPTER,
    BLUETOOTH_DISABLED,
    DEVICE_NOT_FOUND,
    NOT_CONNECTED,
    NOT_CONNECTABLE,
    SERVICE_NOT_FOUND,
    CHARACTERISTIC_NOT_FOUND,
    DESCRIPTOR_NOT_FOUND,
    OPERATION_NOT_SUPPORTED,
    OPERATION_FAILED,
    RESPONSE_TIMEOUT,
    RESPONSE_UNEXPECTED,
    WINRT_EXCEPTION,
    CORE_BLUETOOTH_EXCEPTION,
    UNKNOWN_EXCEPTION
};

constexpr auto KNOWN_ADDRESSES = std::array {
    SimpleBLE::BluetoothAddress{ "98:e2:55:bd:9f:20" },
};

auto service_name(SimpleBLE::BluetoothUUID uuid) -> std::optional<std::string_view> {
    if (uuid == SimpleBLE::BluetoothUUID { "00001800-0000-1000-8000-00805f9b34fb" }) {
        return "GAP";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "00001801-0000-1000-8000-00805f9b34fb" }) {
        return "GATT";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "00c5af5d-1964-4e30-8f51-1956f96bd280" }) {
        return "Service Control";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "ab7de9be-89fe-49ad-828f-118f09df7fd0" }) {
        return "Nintendo SW2";
    }

    return std::nullopt;
}

auto characteristic_name(SimpleBLE::BluetoothUUID uuid) -> std::optional<std::string_view> {
    // see https://github.com/ndeadly/switch2_controller_research/blob/master/bluetooth_interface.md
    if (uuid == SimpleBLE::BluetoothUUID { "00002a00-0000-1000-8000-00805f9b34fb" }) {
        return "Device Name";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "00002a01-0000-1000-8000-00805f9b34fb" }) {
        return "Appearance";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "00c5af5d-1964-4e30-8f51-1956f96bd282" }) {
        return "Service Enable";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "3dacbc7e-6955-40b5-8eaf-6f9809e8b379" }) {
        return "(Pro) HD Rumble + Command";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "4147423d-fdae-4df7-a4f7-d23e5df59f8d" }) {
        return "Large Command / Firmware Update";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "506d9f7d-4278-4e95-a549-326ba77657e0" }) {
        return "(Pro) Extended Command Response";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "649d4ac9-8eb7-4e6c-af44-1ea54fe5f005" }) {
        return "Command";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "ab7de9be-89fe-49ad-828f-118f09df7fd2" }) {
        return "HID Input";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "7492866c-ec3e-4619-8258-32755ffcc0f8" }) {
        return "(Pro) HID Input";
    }
    // if (uuid == SimpleBLE::BluetoothUUID { "ab7de9be-89fe-49ad-828f-118f09df7fde" }) {
    //     return "Unknown";
    // }
    // if (uuid == SimpleBLE::BluetoothUUID { "ab7de9be-89fe-49ad-828f-118f09df7fdf" }) {
    //     return "Unknown";
    // }
    if (uuid == SimpleBLE::BluetoothUUID { "c765a961-d9d8-4d36-a20a-5315b111836a" }) {
        return "Command Response";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "cc483f51-9258-427d-a939-630c31f72b05" }) {
        return "(Pro) HID Output";
    }
    // if (uuid == SimpleBLE::BluetoothUUID { "d3bd69d2-841c-4241-ab15-f86f406d2a80" }) {
    //     return "Unknown";
    // }
    if (uuid == SimpleBLE::BluetoothUUID { "7492866c-ec3e-4619-8258-32755ffcc0f9" }) {
        return "(Pro) Headset Audio + HID Input";
    }

    return std::nullopt;
}

auto descriptor_name(SimpleBLE::BluetoothUUID uuid) -> std::optional<std::string_view> {
    if (uuid == SimpleBLE::BluetoothUUID { "00002902-0000-1000-8000-00805f9b34fb" }) {
        return "Client Characteristic Configuration";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "679d5510-5a24-4dee-9557-95df80486ecb" }) {
        return "Set report rate?";
    }
    return std::nullopt;
}

struct BLEAdapter {
    SimpleBLE::Safe::Adapter adapter;

    static auto Get() -> std::expected<BLEAdapter, BLEException> {
        if (not SimpleBLE::Safe::Adapter::bluetooth_enabled().value_or(false)) {
            return std::unexpected{ BLEException::BLUETOOTH_DISABLED };
        }

        auto adapters = SimpleBLE::Safe::Adapter::get_adapters();
        if (not adapters or adapters->empty()) {
            return std::unexpected{ BLEException::NO_ADAPTER };
        }

        auto adapter = adapters->at(0);
        std::println("Using adapter: {} [{}]", adapter.identifier().value_or(""), adapter.address().value_or("unknown"));
        return BLEAdapter{ adapter };
    }

    auto find_controller(auto scan_duration) -> std::expected<SimpleBLE::Safe::Peripheral, BLEException> {
        // auto device = std::optional<BLEDDevice>{};

        // adapter.set_callback_on_scan_found([this, &device] (SimpleBLE::Safe::Peripheral peripheral){
        //     if (auto a = peripheral.address(); a and stdr::contains(KNOWN_ADDRESSES, *a)) {
        //         device = peripheral;
        //         adapter.scan_stop();
        //     }
        // });

        std::println("Scanning devices for {}...", scan_duration);
        adapter.scan_for(std::chrono::duration_cast<std::chrono::milliseconds>(scan_duration).count());

        auto peripherals = adapter.scan_get_results();
        if (not peripherals or peripherals->empty()) {
            return std::unexpected{ BLEException::DEVICE_NOT_FOUND };
        }

        // TODO use stdr algorithms, or even better use adapter's scan callbacks
        for (auto peripheral : *peripherals) {
            if (auto a = peripheral.address(); a and stdr::contains(KNOWN_ADDRESSES, *a)) {
                return peripheral;
            }
        }

        return std::unexpected{ BLEException::DEVICE_NOT_FOUND };
    }
    
};

struct BLEController {
    SimpleBLE::Safe::Peripheral peripheral;

    auto print() -> void {
        std::println("[{}] {} (type: {})",
                     peripheral.identifier().value_or("unknown"),
                     peripheral.address().value_or("unknown"),
                     static_cast<int>(peripheral.address_type().value_or(SimpleBLE::BluetoothAddressType::UNSPECIFIED)));

        std::println("    RSSI: {}, Tx Power: {} dBm, MTU: {}",
                     peripheral.rssi().value_or(0),
                     peripheral.tx_power().value_or(0),
                     peripheral.mtu().value_or(0));

        std::println("    Paired: {}, Connected: {}",
                     peripheral.is_paired().value_or(false),
                     peripheral.is_connected().value_or(false));

        if (auto manufacturer_data = peripheral.manufacturer_data(); manufacturer_data and not manufacturer_data->empty())
            for (auto& [manufacturer_id, data] : *manufacturer_data) {
                std::println("    Manufacturer ID: {}", manufacturer_id);
                std::println("    Manufacturer data: {}", data);
            }
        else
            std::println("    No manufacturer data");

        // std::println("(S): Service, (C): Characteristic, (D): Descriptor");

        if (auto services = peripheral.services(); services and not services->empty())
            for (auto& service : *services) {
                if (auto name = service_name(service.uuid()); name) {
                    std::println("    {}", *name);
                } else {
                    std::println("    (S) {}", service.uuid());
                }

                std::println("        -> {}", service.data());
                for (auto& characteristic : service.characteristics()) {
                    if (auto name = characteristic_name(characteristic.uuid()); name) {
                        std::println("        {} {}", *name, characteristic.capabilities());
                    }
                    else {
                        std::println("        (C) {} {}", characteristic.uuid(), characteristic.capabilities());
                    }

                    if (characteristic.can_read()) {
                        if (auto data = peripheral.read(service.uuid(), characteristic.uuid()); data) {
                            if (characteristic.uuid() == SimpleBLE::BluetoothUUID { "00002a00-0000-1000-8000-00805f9b34fb" }) {
                                std::println("            -> {}", *data | stdr::to<std::string>());
                            }
                            else {
                                std::println("            -> {}", *data);
                            }
                        }
                    }

                    for (auto& descriptor : characteristic.descriptors()) {
                        if (auto name = descriptor_name(descriptor.uuid()); name) {
                            std::println("            {}", *name);
                        }
                        else {
                            std::println("            (D) {}", descriptor.uuid());
                        }

                        if (characteristic.can_read()) {
                            if (auto data = peripheral.read(service.uuid(), characteristic.uuid(), descriptor.uuid()); data) {
                                std::println("                -> {}", *data);
                            }
                        }
                    }
                }
            }
        else
            std::println("    No service available");
    }

    auto connect() -> std::expected<void, BLEException> {
        if (not peripheral.is_connected().value_or(false)) {
            if (not peripheral.is_connectable().value_or(false)) {
                return std::unexpected{ BLEException::NOT_CONNECTABLE };
            }

            if (not peripheral.connect()) {
                return std::unexpected{ BLEException::NOT_CONNECTED };
            }

            std::println("Connected: {} [{}]", peripheral.identifier().value_or(""), peripheral.address().value_or("unknown"));
        }

        return {};
    }

    auto disconnect() -> std::expected<void, BLEException> {
        if (not peripheral.is_connected().value_or(false)) {
            return {};
        }

        if (not peripheral.disconnect()) {
            return std::unexpected{ BLEException::UNKNOWN_EXCEPTION };
        }

        return {};
    }

    auto enable_sw2() -> std::expected<void, BLEException> {
        if (not peripheral.is_connected().value_or(false)) {
            return std::unexpected{ BLEException::NOT_CONNECTED };
        }

        if (not peripheral.write_request("00c5af5d-1964-4e30-8f51-1956f96bd280",
                                         "00c5af5d-1964-4e30-8f51-1956f96bd282",
                                         { 0x01, 0x00 })
        ) {
            return std::unexpected{ BLEException::OPERATION_FAILED };
        }

        return {};
    }

    auto enable_command_response() -> std::expected<void, BLEException> {
        if (not peripheral.is_connected().value_or(false)) {
            return std::unexpected{ BLEException::NOT_CONNECTED };
        }

        if (not peripheral.write("ab7de9be-89fe-49ad-828f-118f09df7fd0",
                                 "506d9f7d-4278-4e95-a549-326ba77657e0",
                                 "00002902-0000-1000-8000-00805f9b34fb",
                                 { 0x01, 0x00 })
        ) {
            return std::unexpected{ BLEException::OPERATION_FAILED };
        }

        return {};
    }

    auto notify_command_response(std::function<void(SimpleBLE::ByteArray)> cb) -> std::expected<void, BLEException> {
        if (not peripheral.is_connected().value_or(false)) {
            return std::unexpected{ BLEException::NOT_CONNECTED };
        }

        if (not peripheral.notify("ab7de9be-89fe-49ad-828f-118f09df7fd0",
                                  "506d9f7d-4278-4e95-a549-326ba77657e0",
                                  cb)
        ) {
            return std::unexpected{ BLEException::OPERATION_FAILED };
        }

        return {};
    }

    auto unnotify_command_response() -> std::expected<void, BLEException> {
        if (not peripheral.unsubscribe("ab7de9be-89fe-49ad-828f-118f09df7fd0",
                                       "506d9f7d-4278-4e95-a549-326ba77657e0")
        ) {
            return std::unexpected{ BLEException::OPERATION_FAILED };
        }

        return {};
    }

    auto send_command(SimpleBLE::ByteArray bytes) -> std::expected<void, BLEException> {
        if (not peripheral.write_command("ab7de9be-89fe-49ad-828f-118f09df7fd0",
                                         "649d4ac9-8eb7-4e6c-af44-1ea54fe5f005",
                                         bytes)
        ) {
            return std::unexpected{ BLEException::OPERATION_FAILED };
        }

        return {};
    }

    auto subscribe_input(auto cb, auto rate = 0x8500) -> std::expected<void, BLEException> {
        if (not peripheral.is_connected().value_or(false)) {
            return std::unexpected{ BLEException::NOT_CONNECTED };
        }

        // if (not controller.write("ab7de9be-89fe-49ad-828f-118f09df7fd0",
        //                          "7492866c-ec3e-4619-8258-32755ffcc0f8",
        //                          "679d5510-5a24-4dee-9557-95df80486ecb",
        //                          { static_cast<std::uint8_t>(rate >> 8), static_cast<std::uint8_t>(rate >> 0) })
        // ) {
        //     return std::unexpected{ BLEException::OPERATION_FAILED };
        // }

        if (not peripheral.write("ab7de9be-89fe-49ad-828f-118f09df7fd0",
                                 "7492866c-ec3e-4619-8258-32755ffcc0f8",
                                 "00002902-0000-1000-8000-00805f9b34fb",
                                 { 0x01, 0x00 })
        ) {
            return std::unexpected{ BLEException::OPERATION_FAILED };
        }

        if (not peripheral.notify("ab7de9be-89fe-49ad-828f-118f09df7fd0",
                                 "7492866c-ec3e-4619-8258-32755ffcc0f8",
                                 cb)
        ) {
            return std::unexpected{ BLEException::OPERATION_FAILED };
        }

        return {};
    }

    struct CommandResponseNotifier {

        CommandResponseNotifier(BLEController& _controller) : controller{_controller} {
            controller.notify_command_response([this](auto bytes) {
                auto response_lock = std::lock_guard{ response_mtx };
                // std::println("Command response: {:n:02x}", bytes);
                response = bytes;
                response_cv.notify_all();
            });
        }

        ~CommandResponseNotifier() {
            controller.unnotify_command_response();
        }

        auto send_command_expect_response(SimpleBLE::ByteArray command, SimpleBLE::ByteArray expected) -> std::expected<void, BLEException> {
            response = std::nullopt;

            Try(controller.send_command(command));

            {
                auto response_lock = std::unique_lock{ response_mtx };
                response_cv.wait_for(response_lock, 1s, [this]{ return static_cast<bool>(response); });
            }

            if (not response) {
                std::println("Timeout response!");
                return std::unexpected{ BLEException::RESPONSE_TIMEOUT };
            }

            *response = *response | stdr::views::drop_while([](auto b) { return b == 0; }) | stdr::to<SimpleBLE::ByteArray>();
            if (not stdr::equal(*response, expected)) {
                std::println("Expected response: {:n:02x}", expected);
                std::println("         Received: {:n:02x}", *response);
                return std::unexpected{ BLEException::RESPONSE_UNEXPECTED };
            }

            return {};
        }

    private:
        BLEController& controller;
        std::optional<SimpleBLE::ByteArray> response = std::nullopt;
        std::mutex response_mtx;
        std::condition_variable response_cv;
        
    };

    auto initialize(const BLEAdapter& adapter) -> std::expected<void, BLEException> {
        Try(enable_sw2());
        Try(enable_command_response());

        auto channel = CommandResponseNotifier{ *this };

        // Initialize
        Try(channel.send_command_expect_response({ 0x07, 0x91, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00 },                                                                                                          // Unknown. This is the first command sent during initialisation.
                                                 { 0x07, 0x01, 0x01, 0x01, 0x10, 0x78, 0x00, 0x00, 0x00 }));

        Try(channel.send_command_expect_response({ 0x02, 0x91, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x40, 0x7e, 0x00, 0x00, 0x00, 0x30, 0x01, 0x00 },                                                          // Flash memory - Read 0x40 bytes from 0x00013000
                                                 { 0x02, 0x01, 0x01, 0x04, 0x10, 0x78, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x00, 0x01, 0x00, 0x48, 0x45, 0x4a, 0x37, 0x31, 0x30, 0x30, 0x30, 0x33, 0x37, 0x38, 0x34, 0x37, 0x35, 0x00, 0x00, 0x7e, 0x05, 0x69, 0x20, 0x01, 0x06, 0x01, 0x23, 0x23, 0x23, 0xa0, 0xa0, 0xa0, 0xe6, 0xe6, 0xe6, 0x32, 0x32, 0x32, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }));

        Try(channel.send_command_expect_response({ 0x16, 0x91, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00 },                                                                                                          // Unknown
                                                 { 0x16, 0x01, 0x01, 0x01, 0x10, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

        // Bluetooth Pairing
        // const auto address = adapter.address();
        Try(channel.send_command_expect_response({ 0x15, 0x91, 0x01, 0x01, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x01, 0x56, 0x02, 0x23, 0xe7, 0xfc, 0xbc },                                                          // Exchange Addresses
                                      // { 0x15, 0x01, 0x01, 0x01, 0x10, 0x78, 0x00, 0x00, 0x01, 0x04, 0x01, 0x20, 0x9f, 0xbd, 0x55, 0xe2, 0x98 }));
                                                 { 0x15, 0x00, 0x01, 0x01, 0x10, 0x78, 0x00, 0x00 }));
        Try(channel.send_command_expect_response({ 0x15, 0x91, 0x01, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },    // Exchange Keys
                                                 { 0x15, 0x01, 0x01, 0x04, 0x10, 0x78, 0x00, 0x00, 0x01, 0x5c, 0xf6, 0xee, 0x79, 0x2c, 0xdf, 0x05, 0xe1, 0xba, 0x2b, 0x63, 0x25, 0xc4, 0x1a, 0x5f, 0x10 }));
        // const auto ltk = std::array<std::byte>{ 0xef, 0xa0, 0xe5, 0x3b, 0xda, 0x9c, 0xd4, 0x45, 0x1e, 0xfa, 0x20, 0xd3, 0x86, 0x11, 0x09, 0xa3 };                                                         // LTK = A ^ B
        Try(channel.send_command_expect_response({ 0x15, 0x91, 0x01, 0x02, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },    // Challenge (TODO: random)
                                                 { 0x15, 0x01, 0x01, 0x02, 0x10, 0x78, 0x00, 0x00, 0x01, 0x2e, 0x3f, 0xb8, 0x8d, 0x2f, 0xf4, 0xf7, 0xdd, 0xdd, 0x55, 0x5c, 0xb0, 0xa1, 0xa3, 0xa5, 0x09 }));  //       Response is aes-128-ecb of the challenge using LTK
        Try(channel.send_command_expect_response({ 0x15, 0x91, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00 },                                                                                                    // Finalise
                                                 { 0x15, 0x01, 0x01, 0x03, 0x10, 0x78, 0x00, 0x00, 0x01 }));

        // Vibration Sample
        Try(channel.send_command_expect_response({ 0x0a, 0x91, 0x01, 0x02, 0x00, 0x04, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00 },
                                                 { 0x0a, 0x01, 0x01, 0x02, 0x10, 0x78, 0x00, 0x00 }));
        // Set LED pattern
        Try(channel.send_command_expect_response({ 0x09, 0x91, 0x01, 0x07, 0x00, 0x08, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                                                 { 0x09, 0x01, 0x01, 0x07, 0x10, 0x78, 0x00, 0x00 }));
    
        // Enable Feature Mask
        Try(channel.send_command_expect_response({ 0x0c, 0x91, 0x01, 0x02, 0x00, 0x04, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00 },
                                                 { 0x0c, 0x01, 0x01, 0x02, 0x10, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

        // Try(channel.send_command_expect_response({ 0x02, 0x91, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x40, 0x7e, 0x00, 0x00, 0x80, 0x30, 0x01, 0x00 })); // Flash memory - Read 0x40 bytes from 0x00013080
        // Try(channel.send_command_expect_response({ 0x02, 0x91, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x40, 0x7e, 0x00, 0x00, 0xc0, 0x30, 0x01, 0x00 })); // Flash memory - Read 0x40 bytes from 0x000130c0
        // Try(channel.send_command_expect_response({ 0x02, 0x91, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x40, 0x7e, 0x00, 0x00, 0x40, 0xc0, 0x1f, 0x00 })); // Flash memory - Read 0x40 bytes from 0x001fc040
        // Try(channel.send_command_expect_response({ 0x02, 0x91, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x10, 0x7e, 0x00, 0x00, 0x40, 0x30, 0x01, 0x00 })); // Flash memory - Read 0x10 bytes from 0x00013040
        // Try(channel.send_command_expect_response({ 0x02, 0x91, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x18, 0x7e, 0x00, 0x00, 0x00, 0x31, 0x01, 0x00 })); // Flash memory - Read 0x18 bytes from 0x00013100
        // Try(channel.send_command_expect_response({ 0x11, 0x91, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00 })); // Unknown
        // Try(channel.send_command_expect_response({ 0x02, 0x91, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x20, 0x7e, 0x00, 0x00, 0x60, 0x30, 0x01, 0x00 })); // Flash memory - Read 0x20 bytes from 0x00013060

        // Vibration Data
        // Try(channel.send_command_expect_response({ 0x0a, 0x91, 0x01, 0x08, 0x00, 0x14, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x35, 0x00, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        //                                          { 0x0a, 0x01, 0x01, 0x08, 0x10, 0x78, 0x00, 0x00 }));

        // Enable Feature Flags
        Try(channel.send_command_expect_response({ 0x0c, 0x91, 0x01, 0x04, 0x00, 0x04, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00 },
                                                 { 0x0c, 0x01, 0x01, 0x04, 0x10, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

        return {};
    }
};

auto animate() -> void {
    for (;;) {
        std::this_thread::sleep_for(1s);
    }
}

auto run_app() -> std::expected<void, BLEException> {
    auto adapter = TryX(BLEAdapter::Get());

    auto controller = BLEController{ TryX(adapter.find_controller(2s)) };
    Try(controller.connect());
    auto discon = Defered{[&controller]{
        controller.disconnect();
    }};

    Try(controller.initialize(adapter));
    std::println("Initialized.");

    // show(controller);

    Try(controller.subscribe_input([](auto bytes) {
        if (not stdr::empty(bytes) and bytes[0] % 020 != 0) {
            return;
        }

        std::println("HID Input: {:n:02x} ...", bytes);

        std::println("Counter: {:d}"   , bytes[0x0]);
        std::println("Power Info: {:d}", bytes[0x1]);

        std::println("Buttons: {:08b} {:08b} {:08b}", bytes[0x2], bytes[0x3], bytes[0x4]);
        
        const auto li = 0x05;
        const auto lx = (static_cast<uint16_t>(bytes[li + 0] & 0xFF) << 4) + (static_cast<uint16_t>(bytes[li + 1] & 0xF0) >> 4);
        const auto ly = (static_cast<uint16_t>(bytes[li + 1] & 0x0F) << 8) +  static_cast<uint16_t>(bytes[li + 2] & 0xFF);
        // const auto lx = static_cast<uint16_t>(bytes[li + 0] & 0xFF) + (static_cast<uint16_t>(bytes[li + 1] & 0xF0) << 4);
        // const auto ly = static_cast<uint16_t>(bytes[li + 1] & 0x0F) + (static_cast<uint16_t>(bytes[li + 2] & 0xFF) << 4);
        std::println("Left Analog Stick:  {:08b} {:08b} {:08b}", bytes[0x5], bytes[0x6], bytes[0x7]);
        const auto ri = 0x08;
        const auto rx = (static_cast<uint16_t>(bytes[ri + 0] & 0xFF) << 4) + (static_cast<uint16_t>(bytes[ri + 1] & 0xF0) >> 4);
        const auto ry = (static_cast<uint16_t>(bytes[ri + 1] & 0x0F) << 8) +  static_cast<uint16_t>(bytes[ri + 2] & 0xFF);
        // const auto rx = static_cast<uint16_t>(bytes[ri + 0] & 0xFF) + (static_cast<uint16_t>(bytes[ri + 1] & 0xF0) << 4);
        // const auto ry = static_cast<uint16_t>(bytes[ri + 1] & 0x0F) + (static_cast<uint16_t>(bytes[ri + 2] & 0xFF) << 4);
        std::println("Right Analog Stick: {:08b} {:08b} {:08b}", bytes[0x8], bytes[0x9], bytes[0xA]);

        std::println("Unknown: {:08b}"            , bytes[0xB]);
        std::println("NFC state: {:08b}"          , bytes[0xC]);
        std::println("Headset Audio State: {:08b}", bytes[0xD]);
        std::println("Motion Data Length: {:d}"  , bytes[0xE]);
        // std::println("Motion Data: {:0b}"       , bytes[0xF],  ...);
        // std::println("Reserved: {}"             , bytes[0x0]);
    }, 0xffff));
    std::println("Subscribed.");

    for (;;) {
        std::this_thread::sleep_for(1s);
    }

    return {};
}

auto main() -> int {
    if (auto res = run_app(); not res)
        return static_cast<int>(res.error());
    return EXIT_SUCCESS;
}

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_DEVICE_SENSORS_DEVICE_MOTION_EVENT_PUMP_H_
#define CONTENT_RENDERER_DEVICE_SENSORS_DEVICE_MOTION_EVENT_PUMP_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "content/renderer/device_sensors/device_sensor_event_pump.h"
#include "third_party/blink/public/platform/modules/device_orientation/web_device_motion_listener.h"

namespace device {
class SensorReadingSharedBufferReader;
}

namespace content {

class RenderFrame;

class CONTENT_EXPORT DeviceMotionEventPump
    : public PlatformEventObserver<blink::WebDeviceMotionListener> {
 public:
  explicit DeviceMotionEventPump(RenderThread* thread);
  ~DeviceMotionEventPump() override;

  // PlatformEventObserver:
  void Start(blink::WebPlatformEventListener* listener) override;
  void Stop() override;
  void SendStartMessage() override;
  void SendStopMessage() override;
  void SendFakeDataForTesting(void* fake_data) override;

 protected:
  // Default rate for firing events.
  static constexpr int kDefaultPumpFrequencyHz = 60;
  static constexpr int kDefaultPumpDelayMicroseconds =
      base::Time::kMicrosecondsPerSecond / kDefaultPumpFrequencyHz;

  struct SensorEntry : public device::mojom::SensorClient {
    SensorEntry(DeviceMotionEventPump* pump,
                device::mojom::SensorType sensor_type);
    ~SensorEntry() override;

    // device::mojom::SensorClient:
    void RaiseError() override;
    void SensorReadingChanged() override;

    // Mojo callback for SensorProvider::GetSensor().
    void OnSensorCreated(device::mojom::SensorInitParamsPtr params,
                         device::mojom::SensorClientRequest client_request);

    // Mojo callback for Sensor::AddConfiguration().
    void OnSensorAddConfiguration(bool success);

    void HandleSensorError();

    bool SensorReadingCouldBeRead();

    DeviceMotionEventPump* event_pump;
    device::mojom::SensorPtr sensor;
    device::mojom::SensorType type;
    device::mojom::ReportingMode mode;
    device::PlatformSensorConfiguration default_config;
    mojo::ScopedSharedBufferHandle shared_buffer_handle;
    mojo::ScopedSharedBufferMapping shared_buffer;
    std::unique_ptr<device::SensorReadingSharedBufferReader>
        shared_buffer_reader;
    device::SensorReading reading;
    mojo::Binding<device::mojom::SensorClient> client_binding;
  };

  friend struct SensorEntry;

  virtual void FireEvent();

  void DidStartIfPossible();

  void SendStartMessageImpl();

  SensorEntry accelerometer_;
  SensorEntry linear_acceleration_sensor_;
  SensorEntry gyroscope_;

 private:
  friend class DeviceMotionEventPumpTest;

  // DeviceSensorEventPump:
  bool SensorsReadyOrErrored() const override;

  RenderFrame* GetRenderFrame() const;
  bool SensorSharedBuffersReady() const;
  void GetDataFromSharedMemory(device::MotionData* data);
  void GetSensor(SensorEntry* sensor_entry);
  void HandleSensorProviderError();

  device::mojom::SensorProviderPtr sensor_provider_;
  PumpState state_;
  base::RepeatingTimer timer_;

  bool ShouldFireEvent(const device::MotionData& data) const;

  DISALLOW_COPY_AND_ASSIGN(DeviceMotionEventPump);
};

}  // namespace content

#endif  // CONTENT_RENDERER_DEVICE_SENSORS_DEVICE_MOTION_EVENT_PUMP_H_

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_DEVICE_SENSORS_DEVICE_ORIENTATION_EVENT_PUMP_H_
#define CONTENT_RENDERER_DEVICE_SENSORS_DEVICE_ORIENTATION_EVENT_PUMP_H_

#include <memory>

#include "base/macros.h"
#include "content/renderer/device_sensors/device_sensor_event_pump.h"
#include "services/device/public/cpp/generic_sensor/orientation_data.h"
#include "third_party/blink/public/platform/modules/device_orientation/web_device_orientation_listener.h"

namespace content {

typedef SharedMemorySeqLockReader<device::OrientationData>
    DeviceOrientationSharedMemoryReader;

class CONTENT_EXPORT DeviceOrientationEventPumpBase
    : public DeviceSensorEventPump<blink::WebDeviceOrientationListener> {
 public:
  // Angle threshold beyond which two orientation events are considered
  // sufficiently different.
  static const double kOrientationThreshold;

  explicit DeviceOrientationEventPumpBase(RenderThread* thread);
  ~DeviceOrientationEventPumpBase() override;

  // PlatformEventObserver.
  void SendFakeDataForTesting(void* data) override;

 protected:
  void FireEvent() override;
  void DidStartIfPossible() override;

  void SendStartMessageImpl();

  SensorEntry relative_orientation_sensor_;
  SensorEntry absolute_orientation_sensor_;

 private:
  friend class DeviceOrientationEventPumpTest;
  friend class DeviceAbsoluteOrientationEventPumpTest;

  // DeviceSensorEventPump:
  bool SensorsReadyOrErrored() const override;

  void GetDataFromSharedMemory(device::OrientationData* data);

  bool ShouldFireEvent(const device::OrientationData& data) const;

  bool absolute_;
  bool fall_back_to_absolute_orientation_sensor_;
  bool should_suspend_absolute_orientation_sensor_ = false;
  device::OrientationData data_;
  std::unique_ptr<DeviceOrientationSharedMemoryReader> reader_;

  DISALLOW_COPY_AND_ASSIGN(DeviceOrientationEventPumpBase);
};

using DeviceOrientationEventPump =
    DeviceSensorMojoClientMixin<DeviceOrientationEventPumpBase,
                                device::mojom::OrientationSensor>;

using DeviceOrientationAbsoluteEventPump =
    DeviceSensorMojoClientMixin<DeviceOrientationEventPumpBase,
                                device::mojom::OrientationAbsoluteSensor>;

}  // namespace content

#endif  // CONTENT_RENDERER_DEVICE_SENSORS_DEVICE_ORIENTATION_EVENT_PUMP_H_

# Phase 2 — Context Adapter Layer

## Implemented in this pass

- Sensor adapter abstraction interface (`core/include/nocturne/adapters/sensor_adapter.h`)
- Simulated sensor adapters for all observed sensor kinds (`core/include/nocturne/adapters/simulated_sensors.h`)
  - heart rate
  - HRV
  - motion
  - breathing/respiration
  - ambient light
  - temperature
- Sensor registry, read-all orchestration, and sample validation
  (`core/include/nocturne/adapters/adapter_registry.h`)

## Next phase work

- Real transport adapters (BLE/USB/embedded drivers)
- Computed and symbolic adapter implementations
- Failure-mode diagnostics and retry strategy per channel

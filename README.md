# Audio delay fix

This app allows for getting rid of sound lags which happen when you want to play some music or a video but the audio driver is in sleep mode and needs some time before it is ready to start streaming.

## How it works

Microsoft audio subsystems can work in 2 modes: active and sleep. They are related to device power states: D0 and D3, respectively. Transition from D0 to D3 occurs when the audio driver has been idle for some amount of time. Audio subsystems have their idle timeouts set in the registry.

Audio subsystems configuration is placed under `{4d36e96c-e325-11ce-bfc1-08002be10318}` key. And this key can be found in `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class` key.

Essentially, this program changes the idle timeouts of a chosen audio driver to be as high as possible (`0xffffffff`). This is done by updating values of the `PowerSettings` key of the particular audio driver key. `PowerSettings` key path of the first audio device may look like this:
`HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4d36e96c-e325-11ce-bfc1-08002be10318}\0000\PowerSettings`.
The values containing timeouts are stored in `PowerSettings` key and are named `ConservationIdleTime` and `PerformanceIdleTime`.

More info can be found here: [Audio subsystem power management for modern standby platforms](https://learn.microsoft.com/en-us/windows-hardware/design/device-experiences/audio-subsystem-power-management-for-modern-standby-platforms).

## Usage

Build and run the app with `make run` and follow the instructions on the screen. Then you should restart the driver manually (using devmgmt.msc for example) for changes to take place.

## Tests and dependencies

If you want to run tests then you'll need to compile [Boost Test framework](https://www.boost.org/doc/libs/1_84_0/libs/test/doc/html/index.html) and [MS Detours](https://github.com/microsoft/Detours) library yourself. They are not placed in the repo because of their huge size.

Tests cover the `reg::Key` API which is a side effect of the project actually.

## Tools

  - MSVC 19.39 compiler with support of C++23 features
  - Boost Test 1.84 and MS Detours (tests only)

## Note

This app was written just to test out some of the C++ features and other stuff, like Boost Test framework and hooking Winapi functions. If you really want to fix the audio delay asap without any additional software, you can just go to regedit and change the timeouts manually.

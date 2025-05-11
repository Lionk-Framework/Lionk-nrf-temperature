[![Build](https://github.com/Lionk-Framework/Lionk-nrf-temperature/actions/workflows/build.yml/badge.svg)](https://github.com/Lionk-Framework/Lionk-nrf-temperature/actions/workflows/build.yml)

# Zephyr BLE Temperature Sensor

This project is a Zephyr-based Bluetooth Low Energy (BLE) application for Nordic Semiconductor (nRF) devices. It reads temperature and battery level, then transmits the data via BLE. The application supports secure firmware updates over BLE and includes a bootloader with flash protection.

---

## Features

- BLE temperature and battery level service
- Bootloader with flash protection
- Secure firmware updates with signed images (via BLE)
- CI pipeline to sign and validate updates
- Artifacts and releases available via GitHub Actions and Releases

---

## Firmware Updates

### Official Releases

Signed firmware releases are available under [Releases](https://github.com/yourusername/your-temp-sensor-app/releases). These are verified and ready to be distributed to end devices via BLE.

### Intermediate Builds

CI-generated builds are available in the **GitHub Actions Artifacts** section. Always ensure you're using signed binaries before distributing.

- [Latest CI Artifact Build](https://github.com/Lionk-Framework/Lionk-nrf-temperature/actions?query=workflow%3A%22Build%22)

---

## Build it yourself

This procedure was tested on a Fedora 41 Host PC.

### Requirements

- [Docker](https://docs.docker.com/get-started/get-docker/)
- openssl
- [nRF Util](https://www.nordicsemi.com/Products/Development-tools/nRF-Util)

### Generate a Signing Key

1. Generate a private signing key

```bash
openssl ecparam -name prime256v1 -genkey -noout -out priv.pem
```

### Build the application

We suggest using the same building process as the CI. This avoids having to install the build dependencies directly on your machine.
For this we'll use the provided [Dockerfile](Dockerfile).

1. Build the image

```bash
docker build -t nrf-connect-sdk .
```

2. Build the application using the image

```bash
docker run -it -v $(pwd):/app nrf-connect-sdk
```

3. Inside `build/zephyr` you'll find:

- `merged.hex` - Contains the bootloader and the main application 
    - Use this one with `nrfjprog` when first setting up your board
- `app_update.bin` - Contains the main application
    - Use this to update your application using [nRF Connect](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-mobile) for instance

### Flash the application 

```bash
nrfjprog --recover -r --program build/zephyr/merged.hex
```

### Upgrade the application

- Follow [this guide](https://docs.nordicsemi.com/bundle/ncs-latest/page/matter/nrfconnect_examples_software_update.html#device_firmware_upgrade_over_bluetooth_le_using_a_pc_command_line_tool)
to upgrade your application using your PC.

---

## Security

* Flash protection is enabled through the bootloader configuration.
* All firmware images must be signed before being accepted by the bootloader.
* CI ensures all updates are signed with a trusted key.

---

## Third-Party

This project includes components from [embedd-actions/nrf-connect-sdk-ci](https://github.com/embedd-actions/nrf-connect-sdk-ci), licensed under the MIT License.

---

## License

[MIT License](LICENSE)

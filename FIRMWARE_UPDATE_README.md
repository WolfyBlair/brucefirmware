# Wireless Firmware Flasher Implementation

## Overview
This implementation adds wireless firmware update functionality to Bruce via the WebUI interface. The feature allows users to upload firmware files through the web interface and schedule them for installation on the next device restart.

## Features Implemented

### 1. Backend Components

#### Firmware Update Module (`src/core/firmware_update.h`, `src/core/firmware_update.cpp`)
- **initFirmwareUpdate()**: Initializes ArduinoOTA and sets up update handlers
- **handleFirmwareUpload()**: Handles HTTP file upload for firmware binaries
- **handleFirmwareFlash()**: Schedules firmware for update on next boot
- **handleFirmwareStatus()**: Returns current firmware update status
- **checkPendingFirmware()**: Checks for pending updates on startup
- **verifyFirmwareFile()**: Validates firmware integrity using MD5
- **scheduleFirmwareUpdate()**: Creates update schedule for next boot
- **clearPendingFirmware()**: Removes pending update files

#### Web Server Integration (`src/core/wifi/webInterface.cpp`)
- Added firmware update routes:
  - `GET /firmwarestatus` - Get firmware status
  - `POST /firmwareflash` - Schedule firmware update
  - `POST /firmwareclear` - Clear pending firmware
  - `POST /firmwareupload` - Upload firmware file
- Integrated with existing authentication system

#### Main Application Integration (`src/main.cpp`)
- Added firmware update initialization in setup()
- Added startup firmware check
- Integrated with existing display and user interface systems

### 2. User Interface Components

#### WebUI Interface (`embedded_resources/web_interface/`)
- **index.html**: Added firmware update dialog and navigation button
- **index.js**: Added firmware management functions:
  - `checkFirmwareStatus()` - Check current firmware status
  - `uploadFirmware()` - Upload new firmware file
  - `flashFirmware()` - Schedule firmware for installation
  - `clearFirmware()` - Clear pending firmware
- **index.css**: Added styling for firmware update interface

#### Menu System Integration
- **ConfigMenu.cpp**: Added "Firmware Update" option to settings menu
- **settings.h**: Added function declarations for firmware update settings

### 3. Configuration

#### PlatformIO Configuration (`platformio.ini`)
- Added ArduinoOTA library dependency
- Maintained compatibility with existing build configurations

## Usage Workflow

### 1. Accessing Firmware Update
1. Start WebUI (via menu or automatic startup)
2. Navigate to the WebUI interface
3. Click "Firmware" button in the header

### 2. Uploading Firmware
1. Open Firmware Update dialog
2. Click "Refresh Status" to see current state
3. Click "Choose File" to select firmware binary
4. Optionally enter expected MD5 hash for verification
5. Select storage location (LittleFS internal or SD card)
6. Click "Upload Firmware"

### 3. Installing Firmware
1. After successful upload, click "Flash Firmware"
2. Confirm the action when prompted
3. Device will schedule update for next boot
4. Restart the device manually or it will update on next power cycle

### 4. Managing Updates
- Check status anytime with "Refresh Status"
- Clear pending firmware with "Clear Pending"
- Menu integration via Config > Firmware Update

## Technical Details

### File Storage
- Firmware files stored in `/bruce_firmware.bin`
- Update schedule stored in `/bruce_firmware_update.json`
- Automatic cleanup after successful/failed updates

### Security Features
- WebUI authentication required for all operations
- MD5 verification of uploaded firmware
- File integrity checks before scheduling updates
- Safe rollback on verification failures

### Update Process
1. Upload firmware file via web interface
2. Verify file integrity (MD5 check if provided)
3. Schedule update by creating JSON configuration
4. On next boot, check for pending updates
5. Verify firmware again before flashing
6. Flash firmware and restart device
7. Clean up update files

### Error Handling
- Upload failures handled gracefully
- Verification failures prevent bad firmware installation
- Clear error messages in both web UI and serial console
- Safe fallback to existing firmware on failures

## Dependencies Added
- ArduinoOTA library for OTA update functionality
- MD5 verification for firmware integrity
- Integration with existing LittleFS/SD storage systems

## Compatibility
- Maintains compatibility with existing Bruce firmware
- Works with both internal storage (LittleFS) and SD card storage
- Supports all existing Bruce board configurations
- Preserves existing WebUI functionality

## Future Enhancements
- Progress tracking during firmware upload
- Multiple firmware slot support
- Automatic rollback on failed updates
- Webhook notifications for update completion
- Remote update scheduling via API
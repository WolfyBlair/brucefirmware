# GitHub App Implementation for Bruce ESP32 Firmware

## Overview

This implementation adds comprehensive GitHub integration to the Bruce ESP32 firmware, providing a full-featured GitHub client that allows users to interact with GitHub repositories, issues, users, gists, and files directly from their ESP32 device. The implementation includes both traditional Personal Access Token authentication and OAuth 2.0 authentication via access point.

## Features Implemented

### Core GitHub API Integration
- **Authentication**: Personal Access Token (PAT) based authentication and OAuth 2.0
- **Repository Management**: List, create, delete repositories
- **Issue Management**: List, create, and close issues
- **User Operations**: Get user information, list followers/following
- **Gist Operations**: Create and delete gists
- **File Operations**: View, create, update, and delete files in repositories
- **Search**: Search repositories and users
- **Webhooks**: Create and delete webhooks

### OAuth 2.0 Authentication via Access Point
- **Web Interface**: Complete OAuth flow with web-based authentication
- **Access Point**: Self-contained access point for OAuth authentication
- **Demo Mode**: Demonstration mode that simulates OAuth without real GitHub app
- **Configuration**: Support for OAuth Client ID and Secret configuration

### User Interface Integration
- **GitHub Menu**: Added to the main menu system following Bruce's menu patterns
- **Sub-menus**: Organized into logical categories (Auth, Repos, Issues, Users, etc.)
- **Theme Support**: GitHub-specific icons and graphics support
- **Configuration**: Persistent storage of GitHub settings and tokens

### Configuration System
- **Config Integration**: GitHub settings integrated into Bruce's configuration system
- **Persistent Storage**: GitHub tokens and preferences saved to LittleFS/SD
- **OAuth Configuration**: Support for OAuth Client ID and Secret storage
- **Default Repository**: Support for setting default repository for quick operations

## File Structure

```
src/
├── modules/
│   └── github/
│       ├── github_app.h          # Header file with class definitions
│       ├── github_app.cpp         # Full GitHub API implementation
│       ├── github_oauth.h         # OAuth 2.0 header
│       └── github_oauth.cpp       # OAuth 2.0 implementation
└── core/
    ├── menu_items/
    │   ├── GitHubMenu.h          # Menu interface definition
    │   └── GitHubMenu.cpp        # Menu implementation with OAuth support
    ├── main_menu.h               # Updated to include GitHub menu
    ├── main_menu.cpp             # Updated to register GitHub menu
    ├── config.h                  # Extended with GitHub and OAuth settings
    ├── config.cpp                # Added GitHub and OAuth config handling
    └── theme.h                   # Added GitHub theme support
```

## Authentication Methods

### 1. OAuth 2.0 via Access Point (Recommended)
```
GitHub Menu → Demo OAuth (for testing) → OAuth via AP (for real use)
```
- Creates its own WiFi access point
- Provides web interface for GitHub OAuth
- Supports real OAuth flow with GitHub app
- Includes demo mode for testing

### 2. Personal Access Token
```
GitHub Menu → Manual Token (enter PAT) → Token from File (load from SD)
```
- Direct PAT authentication
- Token stored securely in configuration
- Works offline once configured

## OAuth Setup Guide

### For Real OAuth Implementation:
1. **Register GitHub App**:
   - Go to GitHub Settings → Developer settings → OAuth Apps
   - Create new OAuth App with:
     - Application name: "Bruce ESP32"
     - Homepage URL: `http://bruce.local`
     - Authorization callback URL: `http://172.0.0.1:80/github/callback`

2. **Configure Bruce**:
   - GitHub Menu → Configure → Set Client ID
   - GitHub Menu → Configure → Set Client Secret
   - GitHub Menu → OAuth via AP

3. **Use OAuth**:
   - Connect to the access point "Bruce-GitHub-Auth"
   - Open browser to any URL (redirects to OAuth page)
   - Click "Authorize with GitHub"
   - Complete GitHub authorization
   - Return to Bruce device

### Demo OAuth:
```
GitHub Menu → Demo OAuth
```
- Simulates OAuth without GitHub app registration
- Perfect for testing the interface
- Uses demo token for demonstration

## Usage Examples

### Authentication
1. **Demo Mode**: Select "Demo OAuth" to test the interface
2. **Real OAuth**: Configure Client ID/Secret, then select "OAuth via AP"
3. **Manual Token**: Enter GitHub Personal Access Token directly
4. **Token File**: Load token from SD card file

### Repository Operations
1. Navigate to "Repository Ops"
2. Choose from:
   - **List Repos**: View all your repositories with star counts
   - **Create Repo**: Create new repository with optional description
   - **Get Repo Info**: View detailed repository information

### Issue Management
1. Select "Issue Ops"
2. Options include:
   - **List Issues**: View issues in any repository
   - **Create Issue**: Create new issue with title and description

### File Operations
1. Select "File Ops"
2. Features:
   - **View File**: Read file contents from repositories
   - **Create File**: Add new files to repositories with commit messages

## Technical Implementation Details

### OAuth 2.0 Flow
1. **Access Point Creation**: Bruce creates WiFi AP "Bruce-GitHub-Auth"
2. **Web Interface**: Serves OAuth authentication pages
3. **GitHub Redirect**: Redirects to GitHub for authorization
4. **Token Exchange**: Exchanges authorization code for access token
5. **Storage**: Securely stores token in Bruce configuration

### HTTP Communication
- Uses ESP32's built-in HTTPClient library
- Proper GitHub API headers including authentication
- Error handling with descriptive error messages
- JSON parsing for API responses

### JSON Handling
- Simple JSON parsing implementation suitable for ESP32
- Base64 encoding/decoding for file operations
- URL encoding for search queries

### Memory Management
- Efficient string handling for ESP32 memory constraints
- Proper cleanup of HTTP connections
- Minimal JSON parsing to reduce memory usage

### Security Considerations
- Token storage in encrypted configuration
- OAuth state parameter validation
- CSRF protection in OAuth flow
- No token transmission over unencrypted connections
- User confirmation for destructive operations (delete)

## Dependencies
- **ArduinoJson**: Already included in Bruce's dependencies
- **HTTPClient**: Built into ESP32 core
- **WiFi**: Built into ESP32 core
- **AsyncWebServer**: Already used by Bruce for web interface
- **LittleFS/SD**: Already used by Bruce for configuration storage

## Configuration Fields Added
```cpp
// GitHub configuration
String githubDefaultRepo = "";
String githubToken = "";
String githubClientId = "";
String githubClientSecret = "";
bool githubEnabled = true;
bool githubOAuthEnabled = false;
```

## Menu Integration
The GitHub menu is automatically added to the main menu system and appears after the BLE menu. It follows Bruce's established menu patterns and styling.

## Theme Support
GitHub menu supports custom theming with:
- GitHub-specific icons
- Custom graphics and styling
- Integration with Bruce's theme system

## Error Handling
- Comprehensive error messages for API failures
- Network connectivity checks
- Authentication validation
- OAuth-specific error handling
- User-friendly error display

## Future Enhancement Opportunities
- Repository file browser with directory navigation
- Commit history viewing
- Pull request management
- Organization/team management
- GitHub Actions integration
- Release management
- Wiki operations
- Enhanced OAuth with PKCE for better security

## Security Best Practices

### OAuth Security
- State parameter validation prevents CSRF attacks
- Secure redirect URI handling
- Token validation before storage
- Client secret protection

### Token Security
- Tokens stored in Bruce's secure configuration
- No plain text token transmission
- Proper token scope limitations
- Automatic token validation

### Network Security
- HTTPS enforcement where possible
- Proper certificate validation
- DNS spoofing protection via captive portal
- Access point isolation

This implementation provides a secure, comprehensive GitHub integration within the Bruce ESP32 firmware, enabling powerful GitHub operations directly from ESP32 devices with multiple authentication methods including cutting-edge OAuth 2.0 via access point.
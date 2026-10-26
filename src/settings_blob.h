#pragma once

#include <stddef.h>
#include <stdint.h>

#include "settings.h"

// Fixed-layout, versioned encoding of Settings for non-volatile storage.
// Storing individual fields (or the raw struct) would silently misread data
// after a firmware change; a version byte makes an old blob fail cleanly.
//
// Version 2 introduced the second light and the frequency window; version 3
// added the per-light power switch; version 4 added the echo; version 5 added
// beat detection. Older blobs are rejected so the defaults are used instead of
// misreading a different layout.
//
// Version 5 fits the beat fields into the bytes version 4 had reserved, so the
// per-light block stayed 22 bytes; only the header grew by one.
constexpr size_t kLedBlockSize = 22;
constexpr size_t kSettingsHeaderSize = 3;
constexpr size_t kSettingsBlobSize = kSettingsHeaderSize + kLedBlockSize * kLedCount;
constexpr uint8_t kSettingsBlobVersion = 5;

void encodeSettings(const Settings &s, uint8_t *out);

// Returns false if the data is the wrong length or an unknown version.
// Accepted values are clamped, since flash contents cannot be trusted.
bool decodeSettings(const uint8_t *in, size_t len, Settings &out);

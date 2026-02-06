#!/bin/bash
set -euo pipefail

# Create a macOS DMG containing the VST3 and AU plugins
# Usage: ./packaging/create-dmg.sh [build_dir] [output_name]

BUILD_DIR="${1:-build}"
OUTPUT_NAME="${2:-WarmSaturation-macOS}"
ARTEFACTS="$BUILD_DIR/WarmSaturation_artefacts/Release"

# Verify artefacts exist
if [ ! -d "$ARTEFACTS/VST3/Warm Saturation.vst3" ]; then
    echo "Error: VST3 artefact not found. Build the project first."
    exit 1
fi

# Create staging directory
STAGING=$(mktemp -d)
trap "rm -rf $STAGING" EXIT

echo "Staging plugins..."

# Copy plugins
cp -R "$ARTEFACTS/VST3/Warm Saturation.vst3" "$STAGING/"
cp -R "$ARTEFACTS/AU/Warm Saturation.component" "$STAGING/" 2>/dev/null || true
cp -R "$ARTEFACTS/Standalone/Warm Saturation.app" "$STAGING/" 2>/dev/null || true

# Create a simple README for the DMG
cat > "$STAGING/README.txt" << 'EOF'
Warm Saturation - Installation
==============================

VST3:
  Copy "Warm Saturation.vst3" to:
  ~/Library/Audio/Plug-Ins/VST3/

AU (Audio Unit):
  Copy "Warm Saturation.component" to:
  ~/Library/Audio/Plug-Ins/Components/

Standalone:
  Drag "Warm Saturation.app" to your Applications folder.

Note: On first launch, macOS may block the plugin because it is
unsigned. Right-click the file and select "Open", or go to
System Preferences > Privacy & Security and click "Open Anyway".
EOF

# Remove existing DMG if present
rm -f "$OUTPUT_NAME.dmg"

echo "Creating DMG..."
hdiutil create \
    -volname "Warm Saturation" \
    -srcfolder "$STAGING" \
    -ov \
    -format UDZO \
    "$OUTPUT_NAME.dmg"

echo "Created: $OUTPUT_NAME.dmg"

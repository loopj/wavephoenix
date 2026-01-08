#!/usr/bin/env bash

VENV_DIR=".venv"

# Check if GECKO_SDK_PATH is set
if [ -z "$GECKO_SDK_PATH" ]; then
    echo "Error: GECKO_SDK_PATH environment variable not set"
    exit 1
fi

# Create the virtual environment if it doesn't exist
if [ ! -d "$VENV_DIR" ]; then
    echo "Creating Python 3.10 virtual environment..."
    python3.10 -m venv $VENV_DIR
fi

# Save the path to the configurator script
CONFIGURATOR_SCRIPT="${GECKO_SDK_PATH}/platform/radio/efr32_multiphy_configurator/Efr32RadioConfiguratorUcAdapter.py"

# Activate the virtual environment
source $VENV_DIR/bin/activate

# Install required Python packages
pip install jinja2 numpy pyyaml scipy

# For each .radioconf file in config/rail, generate the corresponding rail config
for radioconf_file in config/rail/*.radioconf; do
    base_name=$(basename "$radioconf_file" .radioconf)
    output_path="src/platform/efr32/${base_name}"

    python ${CONFIGURATOR_SCRIPT} ${radioconf_file} -o ${output_path}
    echo "Generated rail config for ${base_name} at ${output_path}"
done
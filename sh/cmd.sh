#!/bin/bash
# My first script

set -e

HERE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
VERSION=$("${HERE}/uppaal/verifyta" -v | head -n 1 | awk '{ print $3 }')
NAME="Uppaal-${VERSION}"
SCRIPT="$0"

#${HERE}/uppaal/verifyta -v

echo "CMD: Transforming XML file to IF file..."
UPPAAL_COMPILE_ONLY=1 ${HERE}/uppaal/verifyta ${HERE}/model/ta_model_agents_test.xml -> ${HERE}/model/ta_model_agents.if
echo "CMD: Done transforming XML to IF!"
echo "CMD: Verifying the model and generating traces!"
${HERE}/uppaal/verifyta -t0 -o2 -u -f ${HERE}/traces/t ${HERE}/model/ta_model_agents_test.xml
echo "CMD: Done Verification!"

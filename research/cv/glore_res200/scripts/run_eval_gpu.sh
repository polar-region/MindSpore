#!/bin/bash
# Copyright 2021 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ============================================================================

echo "=============================================================================================================="
echo "Please run the script as: "
echo "bash run_eval.sh DATA_PATH DEVICE_ID CKPT_PATH"
echo "For example: bash run_eval.sh /path/dataset 0 /path/ckpt"
echo "It is better to use the absolute path."
echo "=============================================================================================================="
set -e
DATA_PATH=$1
export DATA_PATH=${DATA_PATH}
export CUDA_VISIBLE_DEVICES=0
EXEC_PATH=$(pwd)

echo "$EXEC_PATH"

export PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=python

cd ../
env > env0.log
python3 eval.py --data_url $1 --isModelArts False --ckpt_path $2 --device_target="GPU"> eval.log 2>&1 &

if [ $? -eq 0 ];then
    echo "testing success"
else
    echo "testing failed"
    exit 2
fi
echo "finish"
cd ../

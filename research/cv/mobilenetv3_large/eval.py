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
"""evaluate_imagenet"""
import argparse

import mindspore.nn as nn
from mindspore import context
from mindspore.train.model import Model
from mindspore.train.serialization import load_checkpoint, load_param_into_net
from mindspore.nn.loss import SoftmaxCrossEntropyWithLogits

from src.dataset import create_dataset
from src.mobilenetV3 import mobilenet_v3_large
from src.config import config_ascend as config

def parse_args():
    '''parse_args'''
    parser = argparse.ArgumentParser(description='image classification evaluation')
    parser.add_argument('--dataset_path', type=str, default='', help='Dataset path')
    parser.add_argument('--checkpoint_path', type=str, default='', help='checkpoint of mobilenetv3_large')
    parser.add_argument('--device_id', type=int, default=0, help='device id')
    args_opt = parser.parse_args()
    return args_opt

if __name__ == '__main__':
    args = parse_args()

    # set device id
    context.set_context(device_id=args.device_id)
    context.set_context(mode=context.GRAPH_MODE, device_target='Ascend')

    # define network
    net = mobilenet_v3_large(num_classes=config.num_classes, activation="Softmax")
    ckpt = load_checkpoint(args.checkpoint_path)
    load_param_into_net(net, ckpt)
    net.set_train(False)

    # define dataset
    dataset = create_dataset(dataset_path=args.dataset_path, do_train=False, config=config, repeat_num=1,
                             batch_size=config.batch_size)
    loss = SoftmaxCrossEntropyWithLogits(sparse=True, reduction="mean")
    eval_metrics = {'Loss': nn.Loss(),
                    'Top1-Acc': nn.Top1CategoricalAccuracy(),
                    'Top5-Acc': nn.Top5CategoricalAccuracy()}

    model = Model(net, loss, optimizer=None, metrics=eval_metrics)
    print('='*20, 'Evalute start', '='*20)
    metrics = model.eval(dataset)
    print("metric: ", metrics)

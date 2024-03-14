/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef H_BLEHR_SENSOR_
#define H_BLEHR_SENSOR_

#include "nimble/ble.h"
#include "modlog/modlog.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Heart-rate configuration */
#define GATT_SEVER_1_UUID                       0x210A
#define GATT_SEVER_1_CHARACTERISTIC_1_UUID      0xAAAA

#define GATT_SEVER_3_UUID                       0x210B
#define GATT_SEVER_3_CHARACTERISTIC_1_UUID      0xBBBB
#define GATT_CLIENT_CHAR_CFG_UUID               0x2900

extern uint16_t hr_hrm_handle;

struct ble_hs_cfg;
struct ble_gatt_register_ctxt;

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
int gatt_svr_init(void);
void ble_receive_data(const char*data);

#ifdef __cplusplus
}
#endif

#endif
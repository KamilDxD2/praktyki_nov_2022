#pragma once

#ifndef _MESH__MAIN_H_INCLUDED
#define _MESH__MAIN_H_INCLUDED

#include "../debug.h"
#include "painlessMesh.h"
typedef void (*StateUpdateCallback)(uint32_t id, String state, bool configurationChangeRequest);
void meshInit();
void meshBroadcastState(String &state);
void setStateUpdateCallback(StateUpdateCallback newCb);
void requestAllStates();
void sendBoardConfigurationChange(int id, String &state);
uint32_t getCurrentID();

#endif // _MESH__MAIN_H_INCLUDED

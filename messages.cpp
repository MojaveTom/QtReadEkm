#include "messages.h"
#include "../SupportRoutines/supportfunctions.h"
#include <unistd.h>
#include <stdlib.h>

RequestMsgV3Def RequestMsgV3 = {  {'\x2f', '\x3f'}
                                  , {'\x30', '\x30', '\x30', '\x30', '\x30', '\x30', '\x30', '\x30', '\x30', '\x30', '\x30', '\x30'}
                                  , {'\x21', '\x0d', '\x0a'}
                               };

RequestMsgV4Def RequestMsgV4 = {  {'\x2f', '\x3f'}
                                  , {'\x30', '\x30', '\x30', '\x30', '\x30', '\x30', '\x30', '\x30', '\x30', '\x30', '\x30', '\x30'}
                                  , {'\x30', '\x30'}
                                  , {'\x21', '\x0d', '\x0a'}
                               };

uint8_t CloseString[5] = {'\x01', '\x42', '\x30', '\x03', '\x75'};

OutputControlDef Output1OnMsg = { {'\x01'}
                                  , {'\x57', '\x31', '\x02', '\x30', '\x30', '\x38'}
                                  , {'\x31'}
                                  , {'\x28'}
                                  , {'\x31'}
                                  , {'\x30', '\x30', '\x30', '\x30'}
                                  , {'\x29', '\x03'}
                                  , {'\x31', '\x61'}
                                };
OutputControlDef Output1OffMsg = { {'\x01'}
                                   , {'\x57', '\x31', '\x02', '\x30', '\x30', '\x38'}
                                   , {'\x31'}
                                   , {'\x28'}
                                   , {'\x30'}
                                   , {'\x30', '\x30', '\x30', '\x30'}
                                   , {'\x29', '\x03'}
                                   , {'\x21', '\x21'}
                                 };
OutputControlDef Output2OnMsg = { {'\x01'}
                                  , {'\x57', '\x31', '\x02', '\x30', '\x30', '\x38'}
                                  , {'\x32'}
                                  , {'\x28'}
                                  , {'\x31'}
                                  , {'\x30', '\x30', '\x30', '\x30'}
                                  , {'\x29', '\x03'}
                                  , {'\x25', '\x11'}
                                };
OutputControlDef Output2OffMsg = { {'\x01'}
                                   , {'\x57', '\x31', '\x02', '\x30', '\x30', '\x38'}
                                   , {'\x32'}
                                   , {'\x28'}
                                   , {'\x30'}
                                   , {'\x30', '\x30', '\x30', '\x30'}
                                   , {'\x29', '\x03'}
                                   , {'\x35', '\x51'}
                                 };
SetTimeMsgDef   SetTimeMsg =     { {'\x01'}
                                   , {'\x57', '\x31', '\x02', '\x30', '\x30', '\x36', '\x30', '\x28'}
                                   , {{'\x31', '\x35'}, {'\x30', '\x39'}, {'\x30', '\x31'}, {'\x30', '\x33'}, {'\x31', '\x30'}, {'\x30', '\x36'}, {'\x32', '\x30'}}
                                   , {'\x29'}, {'\x03'}, {'\x55', '\x62'}
                                 };

uint8_t ResponseAck[1] = {'\x06'};

uint8_t PasswordMsg[17] = {'\x01', '\x50', '\x31', '\x02', '\x28', '\x30', '\x30', '\x30', '\x30'
                           , '\x30', '\x30', '\x30', '\x30', '\x29', '\x03', '\x32', '\x44'};

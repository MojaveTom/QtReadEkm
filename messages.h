#ifndef MESSAGES_H
#define MESSAGES_H
#include <QtSerialPort>
#include "inttypes.h"

typedef struct
{
    uint8_t fixedBegin[2];
    uint8_t meterId[12];
    uint8_t fixedEnd[3];
} RequestMsgV3Def;

typedef struct
{
    uint8_t fixedBegin[2];
    uint8_t meterId[12];
    uint8_t reqType[2];     // '\x30', '\x30' for Response A  OR  '\x30', '\x31' for Response B
    uint8_t fixedEnd[3];
} RequestMsgV4Def;

typedef struct
{
    uint8_t year[2];
    uint8_t month[2];
    uint8_t day[2];
    uint8_t weekday[2];
    uint8_t hour[2];
    uint8_t minute[2];
    uint8_t second[2];
} meterDateTime;

class ResponseData
{
public:
    uint8_t fixed02[1];                 // SQL offset   1
    uint8_t model[2];                   // SQL offset   2
    uint8_t firmwareVer[1];             // SQL offset   4
    uint8_t meterId[12];                // SQL offset   5
};

typedef struct : public ResponseData
{
    uint8_t totalKwh[8];                // SQL offset  17
    uint8_t time1Kwh[8];                // SQL offset  25
    uint8_t time2Kwh[8];                // SQL offset  33
    uint8_t time3Kwh[8];                // SQL offset  41
    uint8_t time4Kwh[8];                // SQL offset  49
    uint8_t totalRevKwh[8];             // SQL offset  57
    uint8_t time1RevKwh[8];             // SQL offset  65
    uint8_t time2RevKwh[8];             // SQL offset  73
    uint8_t time3RevKwh[8];             // SQL offset  81
    uint8_t time4RevKwh[8];             // SQL offset  89
    uint8_t volts1[4];                  // SQL offset  97
    uint8_t volts2[4];                  // SQL offset 101
    uint8_t volts3[4];                  // SQL offset 105
    uint8_t amps1[5];                   // SQL offset 109
    uint8_t amps2[5];                   // SQL offset 114
    uint8_t amps3[5];                   // SQL offset 119
    uint8_t watts1[7];                  // SQL offset 124
    uint8_t watts2[7];                  // SQL offset 131
    uint8_t watts3[7];                  // SQL offset 138
    uint8_t wattsTotal[7];              // SQL offset 145
    uint8_t cos1[4];                    // SQL offset 152
    uint8_t cos2[4];                    // SQL offset 156
    uint8_t cos3[4];                    // SQL offset 160
    uint8_t maxDemand[8];               // SQL offset 164
    uint8_t demandPeriod[1];            // SQL offset 172
    meterDateTime dateTime;             // SQL offset 173
    uint8_t currentTransformer[4];      // SQL offset 187
    uint8_t pulseCount1[8];             // SQL offset 191
    uint8_t pulseCount2[8];             // SQL offset 199
    uint8_t pulseCount3[8];             // SQL offset 207
    uint8_t pulseRatio1[4];             // SQL offset 215
    uint8_t pulseRatio2[4];             // SQL offset 219
    uint8_t pulseRatio3[4];             // SQL offset 223
    uint8_t PulseState[3];              // SQL offset 227
    uint8_t reserved[20];               // SQL offset 230
    uint8_t fixedEnd[4];                // SQL offset 250
    uint8_t crc[2];                     // SQL offset 254
} ResponseV3Data;

typedef struct : public ResponseData
{
    uint8_t reserved[217];              // SQL offset  17
    meterDateTime dateTime;             // SQL offset 234
    uint8_t msgType[2];                 // SQL offset 248
    uint8_t fixedEnd[4];                // SQL offset 250
    uint8_t crc[2];                     // SQL offset 254
} ResponseV4Data;

typedef struct : public ResponseData
{
    uint8_t totalKwh[8];                // SQL offset  17
    uint8_t totalKVARh[8];              // SQL offset  25
    uint8_t totalRevKwh[8];             // SQL offset  33
    uint8_t totalKwhL1[8];              // SQL offset  41
    uint8_t totalKwhL2[8];              // SQL offset  49
    uint8_t totalKwhL3[8];              // SQL offset  57
    uint8_t reverseKwhL1[8];            // SQL offset  65
    uint8_t reverseKwhL2[8];            // SQL offset  73
    uint8_t reverseKwhL3[8];            // SQL offset  81
    uint8_t resettableTotalKwh[8];      // SQL offset  89
    uint8_t resettableReverseKwh[8];    // SQL offset  97
    uint8_t volts1[4];                  // SQL offset 105
    uint8_t volts2[4];                  // SQL offset 109
    uint8_t volts3[4];                  // SQL offset 113
    uint8_t amps1[5];                   // SQL offset 117
    uint8_t amps2[5];                   // SQL offset 122
    uint8_t amps3[5];                   // SQL offset 127
    uint8_t watts1[7];                  // SQL offset 132
    uint8_t watts2[7];                  // SQL offset 139
    uint8_t watts3[7];                  // SQL offset 146
    uint8_t wattsTotal[7];              // SQL offset 153
    uint8_t cos1[4];                    // SQL offset 160
    uint8_t cos2[4];                    // SQL offset 164
    uint8_t cos3[4];                    // SQL offset 168
    uint8_t varL1[7];                   // SQL offset 172
    uint8_t varL2[7];                   // SQL offset 179
    uint8_t varL3[7];                   // SQL offset 186
    uint8_t varL123[7];                 // SQL offset 193
    uint8_t frequency[4];               // SQL offset 200
    uint8_t pulseCount1[8];             // SQL offset 204
    uint8_t pulseCount2[8];             // SQL offset 212
    uint8_t pulseCount3[8];             // SQL offset 220
    uint8_t pulseState[1];              // SQL offset 228
    uint8_t currentDir123[1];           // SQL offset 229
    uint8_t outState[1];                // SQL offset 230
    uint8_t kwhDecimals[1];             // SQL offset 231
    uint8_t reserved[2];                // SQL offset 232
    meterDateTime dateTime;             // SQL offset 234
    uint8_t msgType[2];                 // SQL offset 248
    uint8_t fixedEnd[4];                // SQL offset 250
    uint8_t crc[2];                     // SQL offset 254
} ResponseV4AData;

typedef struct : public ResponseData
{
    uint8_t time1Kwh[8];                // SQL offset  17
    uint8_t time2Kwh[8];                // SQL offset  25
    uint8_t time3Kwh[8];                // SQL offset  33
    uint8_t time4Kwh[8];                // SQL offset  41
    uint8_t time1RevKwh[8];             // SQL offset  49
    uint8_t time2RevKwh[8];             // SQL offset  57
    uint8_t time3RevKwh[8];             // SQL offset  65
    uint8_t time4RevKwh[8];             // SQL offset  73
    uint8_t volts1[4];                  // SQL offset  81
    uint8_t volts2[4];                  // SQL offset  85
    uint8_t volts3[4];                  // SQL offset  89
    uint8_t amps1[5];                   // SQL offset  93
    uint8_t amps2[5];                   // SQL offset  98
    uint8_t amps3[5];                   // SQL offset 103
    uint8_t watts1[7];                  // SQL offset 108
    uint8_t watts2[7];                  // SQL offset 115
    uint8_t watts3[7];                  // SQL offset 122
    uint8_t wattsTotal[7];              // SQL offset 129
    uint8_t cos1[4];                    // SQL offset 136
    uint8_t cos2[4];                    // SQL offset 140
    uint8_t cos3[4];                    // SQL offset 144
    uint8_t maxDemand[8];               // SQL offset 148
    uint8_t demandPeriod[1];            // SQL offset 156
    uint8_t PRatio1[4];                 // SQL offset 157
    uint8_t PRatio2[4];                 // SQL offset 161
    uint8_t PRatio3[4];                 // SQL offset 165
    uint8_t CTRatio[4];                 // SQL offset 169
    uint8_t autoResetMaxDemand[1];      // SQL offset 173
    uint8_t CFRatio[4];                 // SQL offset 174
    uint8_t reserved[56];               // SQL offset 178
    meterDateTime dateTime;             // SQL offset 234
    uint8_t msgType[2];                 // SQL offset 248
    uint8_t fixedEnd[4];                // SQL offset 250
    uint8_t crc[2];                     // SQL offset 254
} ResponseV4BData;

typedef union responseV4union
{
    ResponseV4Data   responseV4Generic;
    ResponseV4AData  responseV4Adata;
    ResponseV4BData  responseV4Bdata;
} ResponseV4Generic;

#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
/* These can't be used after statements in c89. */
#ifdef __COUNTER__
  #define STATIC_ASSERT(e) \
    ;enum { ASSERT_CONCAT(static_assert_, __COUNTER__) = 1/(!!(e)) }
#else
  /* This can't be used twice on the same line so ensure if using in headers
   * that the headers are not included twice (by wrapping in #ifndef...#endif)
   * Note it doesn't cause an issue when used on same line of separate modules
   * compiled with gcc -combine -fwhole-program.  */
  #define STATIC_ASSERT(e) \
    ;enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
#endif

STATIC_ASSERT((sizeof(ResponseV3Data) == 255));
STATIC_ASSERT((sizeof(ResponseV4AData) == 255));
STATIC_ASSERT((sizeof(ResponseV4BData) == 255));

typedef struct
{
    uint8_t SOH[1];
} OutputMsg;

typedef struct
{
    uint8_t SOH[1];
    uint8_t preamble[8];            // "W 1 STX 0 0 6 0 ("
    meterDateTime dateTime;
    uint8_t fixedSeparator[1];      // ')'
    uint8_t ETX[1];                 // '\x03'
    uint8_t crc[2];
} SetTimeMsgDef;

typedef struct
{
    uint8_t SOH[1];
    uint8_t preamble[6];        // {'\x57', '\x31', '\x02', '\x30', '\x30', '\x38'}
    uint8_t relayNum[1];        // '\x31' = out1; '\x32' = out2
    uint8_t fixedSeparator[1];
    uint8_t newState[1];        // '\x30' = off; '\x31' = on
    uint8_t newStateDuration[4];
    uint8_t postamble[2];
    uint8_t crc[2];
} OutputControlDef;

typedef union
{
    OutputMsg        genericOutputMsg;
    OutputControlDef outputControlDef;
    SetTimeMsgDef    setTimeMsgDef;
} GenericOutputMsg;

extern uint8_t CloseString[5];
extern uint8_t ResponseAck[1];
extern uint8_t PasswordMsg[17];

extern RequestMsgV3Def RequestMsgV3;
extern RequestMsgV4Def RequestMsgV4;
extern OutputControlDef Output1OnMsg;
extern OutputControlDef Output1OffMsg;
extern OutputControlDef Output2OnMsg;
extern OutputControlDef Output2OffMsg;
extern SetTimeMsgDef    SetTimeMsg;

#endif // MESSAGES_H


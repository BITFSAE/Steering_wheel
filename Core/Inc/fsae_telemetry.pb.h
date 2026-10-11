/**
 * @file    fsae_telemetry.pb.h
 * @brief   Minimal stub for nanopb-generated telemetry header.
 *          This project does not use nanopb; the stub provides just enough
 *          type definitions so ecu_info.c / can.c compile cleanly.
 */
#ifndef __FSAE_TELEMETRY_PB_H__
#define __FSAE_TELEMETRY_PB_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Motor sub-structure (4 instances in fsae_VehicleState)                    */
/* ------------------------------------------------------------------------- */
typedef struct {
    int32_t rpm;
    int32_t torque;
    int32_t power_w;
    int32_t motor_error;
} fsae_MotorState;

#define fsae_MotorState_init_default { 0, 0, 0, 0 }

/* ------------------------------------------------------------------------- */
/* BMS summary                                                               */
/* ------------------------------------------------------------------------- */
typedef struct {
    uint32_t soc_pct;
    uint32_t total_voltage;
    int32_t  max_temp_c;
    int32_t  total_current;
} fsae_BmsSummary;

#define fsae_BmsSummary_init_default { 0U, 0U, 0, 0 }

/* ------------------------------------------------------------------------- */
/* Vehicle state                                                             */
/* ------------------------------------------------------------------------- */
typedef struct {
    uint32_t      speed_kmh;
    fsae_MotorState motors[4];
} fsae_VehicleState;

#define fsae_VehicleState_init_default { 0U, { \
    fsae_MotorState_init_default, \
    fsae_MotorState_init_default, \
    fsae_MotorState_init_default, \
    fsae_MotorState_init_default } }

#ifdef __cplusplus
}
#endif

#endif /* __FSAE_TELEMETRY_PB_H__ */

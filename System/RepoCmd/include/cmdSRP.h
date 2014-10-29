/**
 * @file  cmdDRP.h
 * @author Tomas Opazo T
 * @date 2012
 * @copyright GNU Public License.
 *
 * Este header contiene las definiciones de comandos asociados al repositorio de
 * datos. Comandos para accesar a las variables de estado y a la memoria externa.
 *
 * Estos comandos seran SOLO aquellos relacionados con dataRepository, por tanto tendran que ver con:
 * actualizacion                =>  update
 * visualizacion en consola     =>  print
 * condiciones iniciales        =>  executeBeforFligth
 * modificaion de FlightPlan    =>  setFlightPlan
 *
 * Lo anterior para las estructuras de dataRepository:
 * dat_CubesatVar
 * dat_TelecmdBuff
 * dat_FligthPlan
 * dat_PayloadVar
 */

#ifndef CMD_SRP_H
#define CMD_SRP_H

#include "FreeRTOS.h" //vTaskDelay
#include "task.h"

#include "dataRepository.h"
//#include "cmdIncludes.h"    //para CMD_XXX
#include "DebugIncludes.h"  //para printf

/* Necesarios para update */
#include "cmdPPC.h"
#include "cmdTRX.h"
#include "cmdEPS.h"
#include "cmdRTC.h"
#include "cmdDRP.h"


/**
 * Lista de comandos disponibles.
 */
typedef enum{
    //de debug
    srp_id_debug=((unsigned int)SCH_CMD_SRP*0x100), ///< @cmd_first

    //De display
    srp_id_print_STA_CubesatVar, ///< @cmd

    //De update en general de dat_CubesatVar
    srp_id_increment_STA_CubesatVar_hoursWithoutReset, ///< @cmd
    srp_id_increment_STA_CubesatVar_hoursAlive, ///< @cmd
    srp_id_increment_STA_CubesatVar_nSended_tm, ///< @cmd
    srp_id_increment_STA_CubesatVar_nReceived_tc, ///< @cmd
    srp_id_update_STA_CubesatVar_opMode, ///< @cmd
    srp_id_update_STA_CubesatVar_trx_rssi, ///< @cmd
    srp_id_update_STA_CubesatVar_trx_rssi_mean, ///< @cmd
    //de TC buffer
    srp_id_update_STA_CubesatVar_trx_newTcFrame, ///< @cmd
    srp_id_update_STA_CubesatVar_trx_newCmdBuff, ///< @cmd
            
    //De EBF
    srp_id_executeBeforeFlight, ///< @cmd

   //*************
    srp_id_last_one    //Elemento sin sentido, solo se utiliza para marcar el largo del arreglo
}SRP_CmdIndx;

#define SRP_NCMD ((unsigned char)srp_id_last_one)
void srp_onResetCmdSRP();

//Commands
//****************************************************
int srp_debug(void *param);
int srp_print_STA_CubesatVar(void *param);
// command's to update dat_CubesatVar
int srp_increment_STA_CubesatVar_hoursWithoutReset(void *param);
int srp_increment_STA_CubesatVar_hoursAlive(void *param);
int srp_increment_STA_CubesatVar_nSended_tm(void *param);
int srp_increment_STA_CubesatVar_nReceived_tc(void *param);
int srp_update_sta_CubesatVar_opMode(void *param);
int srp_update_STA_CubesatVar_trx_rssi(void *param);
int srp_update_STA_CubesatVar_trx_rssi_mean(void *param);

int srp_trx_update_STA_CubesatVar_newTcFrame(void *param);
int srp_trx_STA_CubesatVar_newCmdBuff(void *param);

int srp_executeBeforeFlight(void *param);

//funciones auxiliares
//****************************************************
void srp_eraseAll_CubesatVar(void);

void srp_debug4(void);

//PPC
void srp_ppc_enwdt(void);
void srp_ppc_osc(void);
void srp_ppc_MB_nOE_USB_nINT_stat(void);
void srp_ppc_MB_nOE_MHX_stat(void);
void srp_ppc_MB_nON_MHX_stat(void);
void srp_ppc_MB_nON_SD_stat(void);
//RTC
void srp_rtc_year(void);
void srp_rtc_month(void);
void srp_rtc_week_day(void);
void srp_rtc_day_number(void);
void srp_rtc_hours(void);
void srp_rtc_minutes(void);
void srp_rtc_seconds(void);
//EPS
void srp_eps_bat0_voltage(void);
void srp_eps_bat0_current(void);
void srp_eps_bus5V_current(void);
void srp_eps_bus3V_current(void);
void srp_eps_bus_battery_current(void);
void srp_eps_bat0_temp(void);
void srp_eps_panel_pwr(void);
void srp_eps_status(void);
void srp_eps_soc(void);
void srp_eps_socss(void);
void srp_eps_state_flag(void);
void srp_eps_charging(void);
//TRX
void srp_trx_opmode(void);
void srp_trx_temp_hpa(void);
void srp_trx_temp_mcu(void);
void srp_trx_rssi(void);
void srp_trx_status_tc(void);
int srp_trx_rssi_mean(int arg);

//**********************************************************************************
//Estas no son comandos , pero si funciones llamadas SOLO desde taskDeployment
// Functions that works with persistent memory
//**********************************************************************************



#endif /* CMD_SRP_H */

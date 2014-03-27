/*                                 SUCHAI
 *                      NANOSATELLITE FLIGHT SOFTWARE
 *
 *      Copyright 2013, Carlos Gonzalez Cortes, carlgonz@ug.uchile.cl
 *      Copyright 2013, Tomas Opazo Toro, tomas.opazo.t@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "taskDeployment.h"
#include "csp.h"
#include "csp_if_i2c.h"
#include "taskTest.h"

extern xTaskHandle taskComunicationsHandle;
extern xTaskHandle taskConsoleHandle;
extern xTaskHandle taskFlightPlanHandle;
extern xTaskHandle taskFlightPlan2Handle;
extern xTaskHandle taskHouskeepingHandle;

extern xQueueHandle dispatcherQueue;

//Libcsp function
static void csp_initialization(void);

void taskDeployment(void *param)
{
    #if (SCH_TASKDEPLOYMENT_VERBOSE)
        con_printf(">>[Deployment] Started\r\n");
        con_printf("\n[Deployment] INITIALIZING SUCHAI FLIGHT SOFTWARE\r\n");
    #endif

    /* Perifericos*/
    dep_init_hw(NULL);

    /* Repositorios */
    dep_init_Repos(NULL);

    /* Otras estructuras */
    dep_init_GnrlStrct(NULL);

//    /* Antena */
//    #if (SCH_ANTENNA_ONBOARD==1)
//        int realTime2 = SCH_TASKDEPLOYMENT_ANTENNA_REALTIME; /* 1=Real Time, 0=Debug Time */
//        dep_deploy_antenna( (void *)&realTime2 );
//    #endif

    /* Initializing Transceiver */
    #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
        con_printf("    * Setting TRX\r\n");
    #endif
    #if (SCH_TRX_ONBOARD==1)
        trx_initialize(NULL);
    #endif
}

/**
 * Initializes all data repositories
 *
 * @param param Not used
 * @return 1
 */
int dep_init_Repos(void *param)
{
    #if (SCH_TASKDEPLOYMENT_VERBOSE>=1)
        con_printf("\n[dep_init_Repos] Initializing status repositories...\r\n");
    #endif

    #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
        con_printf("    * Status rep.\r\n");
    #endif
    sta_onResetStatRepo();
//------------------------------------------------------------------------------
    #if (SCH_TASKDEPLOYMENT_VERBOSE>=1)
        con_printf("\n[dep_init_Repos] Initializing command repositories...\r\n");
    #endif

    #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
        con_printf("    * Commands rep.\r\n");
    #endif
    repo_onResetCmdRepo();
//------------------------------------------------------------------------------
    #if (SCH_TASKDEPLOYMENT_VERBOSE>=1)
        con_printf("\n[dep_init_Repos] Initializing data repositories...\r\n");
    #endif

    #if (SCH_USE_FLIGHTPLAN == 1 )
        /* Initializing dataRepository */
        #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
            con_printf("    * FlighPlan\r\n");
        #endif
        dat_onResetFlightPlan();
    #endif

    #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
        con_printf("    * Telecommands buffer\r\n");
    #endif
    dat_onResetTelecmdBuff();

    #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
        con_printf("    * Payloads data rep.\r\n");
    #endif
    dat_onResetPayloadVar();

    return 1;
}

/**
 * Initializes all data repositories
 *
 * @param param Not used
 * @return 1
 */
int dep_init_GnrlStrct(void *param)
{
    #if (SCH_TASKDEPLOYMENT_VERBOSE>=1)
        con_printf("\n[dep_init_GnrlStruct] Initializing other structures...\r\n");
    #endif

    /* Initializing EPS struct */
    #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
        con_printf("    * init EPS structs\r\n");
    #endif
    setStateFlagEPS( (unsigned char)sta_getCubesatVar(sta_eps_state_flag) );

    return 1;
}

/**
 * Implements mandatory radial silence
 * @param param 1-Realtime, 0-Debug time
 * @return 1-Ok, 0-Fails
 */
int dep_silent_time(void *param)
{
    #if (SCH_TASKDEPLOYMENT_VERBOSE>=1)
        con_printf("\n[dep_silent_time] Mandatory inactivity time...\r\n");
    #endif

    //Reviso si antenas ya estan desplegadas
    #if (SCH_ANTENNA_ONBOARD==1)
    {
        if( sta_getCubesatVar(dat_dep_ant_deployed) == 0x0001 )
        {
            #if (SCH_TASKDEPLOYMENT_VERBOSE>=1)
                con_printf("    Satellite launched. Antenna deployed\r\n");
                con_printf("    FINISHING SILENT TIME\r\n");
            #endif
            return 1;
        }
        

        #if (SCH_TASKDEPLOYMENT_VERBOSE>=1)
            con_printf("    First time on. Antenna NOT deployed\r\n");
            con_printf("    STARTING 30MIN SILENT TIME\r\n");
        #endif
    }
    #endif

    //Sino, 1) silencio el TRX
    #if (SCH_TRX_ONBOARD==1 || SCH_TRX_ONBOARD==2)
        #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
            con_printf("    * Turning off TX\r\b");
        #endif

        /* A delay before config TRX */
        const unsigned long Delaytrx = 1000 / portTICK_RATE_MS;
        vTaskDelay(Delaytrx);
            
        int trx_mode=2;
        trx_setmode( (void *)&trx_mode );
    #endif

    //Y 2) duermo el SUCHAI por 30min
    #if (SCH_TASKDEPLOYMENT_VERBOSE>=1)
        con_printf("    * System halted at ");
        rtc_print(NULL); /* EL RTC no ha sido inicializado aun */
    #endif

    int mode= *( (int *)param );
    if(mode)    /* RealTIme */
    {
        const unsigned int time_out = (0xFFFF) / portTICK_RATE_MS; /* 65,535[s]*/

        unsigned int indx2;
        for(indx2=0; indx2<TDP_SILENT_TIME_MIN-1; indx2++)
        {
            vTaskDelay(time_out);
        }

        con_printf("    * 65[s] remaining ...\r\n");
        vTaskDelay(time_out);
    }
    else    /* NO RealTIme */
    {
        const unsigned int time_out = (2000) / portTICK_RATE_MS; /* 2[s]*/
        vTaskDelay(time_out);
    }

    #if (SCH_TASKDEPLOYMENT_VERBOSE>=1)
        con_printf("    * System resumed at ");
        rtc_print(NULL);
        con_printf("    FINISHING SILENT TIME\r\n");
    #endif

    return 1;
}

/**
 * Ends taskDeployment by deleting it.
 * @param param Not used
 * @return 1 succes
 */
int dep_suicide(void *param)
{
    #if (SCH_TASKDEPLOYMENT_VERBOSE)
        con_printf("[Deployment] ENDS\r\n");
        con_printf("[Deployment] Deleting task\r\n");
    #endif
    vTaskDelete(NULL);

    while(1)
    {
        con_printf("    vTaskDelete(NULL) did NOT work out...\r\n");
        const unsigned long Delayms = 0xFFFF / portTICK_RATE_MS;
        vTaskDelay(Delayms);
    }

    return 1;
}

/**
 * Starts all task.
 * @param param Not used
 * @return 1 success, 0 fails
 */
int dep_launch_tasks(void *param)
{
    #if (SCH_TASKDEPLOYMENT_VERBOSE>=1)
        con_printf("\n[dep_launch_tasks] Starting all tasks...\r\n");
    #endif

    #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
        con_printf("    * Creating taskConsole\r\n");
    #endif
    xTaskCreate(taskConsole, (signed char *)"console", 1.5*configMINIMAL_STACK_SIZE, NULL, 2, &taskConsoleHandle);

    #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
        con_printf("    * Creating taskHousekeeping\r\n");
    #endif
    //xTaskCreate(taskHouskeeping, (signed char *)"housekeeping", 2*configMINIMAL_STACK_SIZE, NULL, 2, &taskHouskeepingHandle);
    
    #if (SCH_TRX_ONBOARD==1 || SCH_TRX_ONBOARD==2)
        #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
            con_printf("    * Creating taskCommunications\r\n");
        #endif
        xTaskCreate(taskComunications, (signed char *)"comunications", 2*configMINIMAL_STACK_SIZE, NULL, 2, &taskComunicationsHandle);
    #endif
    xTaskCreate(taskComunications, (signed char *)"comunications", 3*configMINIMAL_STACK_SIZE, NULL, 1, &taskComunicationsHandle);


    if( sta_getCubesatVar(sta_MemSD_isAlive) == 1 )
    {
        #if (SCH_USE_FLIGHTPLAN==0)
            //launch nothing..
        #elif (SCH_USE_FLIGHTPLAN==1)
            #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
                    con_printf("    * Creating taskFlightPlan\r\n");
            #endif
            xTaskCreate(taskFlightPlan, (signed char *)"flightplan", 2*configMINIMAL_STACK_SIZE, NULL, 2, &taskFlightPlanHandle);
        #endif
        #if (SCH_USE_FLIGHTPLAN2==0)
            //launch nothing..
        #elif (SCH_USE_FLIGHTPLAN2==1)
            #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
                    con_printf("    * Creating taskFlightPlan2\r\n");
            #endif
            xTaskCreate(taskFlightPlan2, (signed char *)"flightplan2", 2*configMINIMAL_STACK_SIZE, NULL, 2, &taskFlightPlan2Handle);
        #endif
    }
        
    return 1;
}

/**
 * Deploys satellite antennas
 * @param param 1 realime, 0 debug time
 * @return 1 success, 0 fails
 */
int dep_deploy_antenna(void *param)
{

    #if (SCH_TASKDEPLOYMENT_VERBOSE>=1)
        char buffer[10];
        con_printf("\n[dep_deploy_antenna] Deploying TRX Antenna... \r\n");
    #endif

    if( sta_getCubesatVar(sta_dep_ant_deployed) == 0x0001 )
    {
        #if (SCH_TASKDEPLOYMENT_VERBOSE>=1)
            con_printf("    * Antenna is already deployed\r\n");
        #endif
        return 1;
    }

    //Realtime=1 DebugTime=0
    unsigned int delay_dep_time, delay_rest_dep_time, delay_recheck_dep_time;
    int mode= *( (int *)param );
    if(mode)
    {
        delay_dep_time = (TDP_DEPLOY_TIME) / portTICK_RATE_MS;
        delay_rest_dep_time = (TDP_REST_DEPLOY_TIME) / portTICK_RATE_MS;
        delay_recheck_dep_time = (TDP_RECHECK_TIME) / portTICK_RATE_MS;
    }
    else
    {
        delay_dep_time = (600) / portTICK_RATE_MS;
        delay_rest_dep_time = (400) / portTICK_RATE_MS;
        delay_recheck_dep_time = (200) / portTICK_RATE_MS;
    }

    //Quemado del nylon
    int tries_indx = 0;

    #if(SCH_ANTENNA_ONBOARD == 1)
    {
        for(tries_indx=1; tries_indx<=TDP_TRY_DEPLOY; tries_indx++)
        {
            #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
                itoa(buffer,  tries_indx, 10);
                con_printf("    [Deploying] Attempt #"); con_printf(buffer);
                con_printf("\r\n"); //con_printf(" at "); rtc_print(NULL);
            #endif

            PPC_ANT12_SWITCH=1;
            PPC_ANT1_SWITCH=1;
            PPC_ANT2_SWITCH=0;
            //PPC_ANT1_SWITCH=0;
            //PPC_ANT2_SWITCH=1;
            vTaskDelay(delay_dep_time);   /* tiempo de intento ANT1 */
            vTaskDelay(delay_dep_time);   /* tiempo de intento ANT1 */

            PPC_ANT12_SWITCH=0;
            PPC_ANT1_SWITCH=0;
            PPC_ANT2_SWITCH=0;
            vTaskDelay(delay_rest_dep_time);   /* tiempo de descanso */

            PPC_ANT12_SWITCH=1;
            PPC_ANT1_SWITCH=0;
            PPC_ANT2_SWITCH=1;
            //PPC_ANT1_SWITCH=1;
            //PPC_ANT2_SWITCH=0;
            vTaskDelay(delay_dep_time);   /* tiempo de intento ANT2 */
            vTaskDelay(delay_dep_time);   /* tiempo de intento ANT2 */

            PPC_ANT12_SWITCH=0;
            PPC_ANT1_SWITCH=0;
            PPC_ANT2_SWITCH=0;
            vTaskDelay(delay_rest_dep_time);   /* tiempo de descanso */


            if(PPC_ANT12_CHECK==0)   /* reviso */
            {
                vTaskDelay(delay_recheck_dep_time);   /* tiempo de RE-chequeo */
                if(PPC_ANT12_CHECK==0)   /* RE-reviso */
                {
                    #if (SCH_TASKDEPLOYMENT_VERBOSE>=1)
                        itoa(buffer, (unsigned int)tries_indx, 10);
                        con_printf("    ANTENNA DEPLOYED SUCCESSFULLY [");
                        con_printf(buffer); con_printf(" TRIES]\r\n");
                    #endif

                    drp_dep_write_deployed(1, tries_indx);
                    return 1;
                }
            }
        }
    }
    #endif

    #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
        itoa(buffer, (unsigned int)TDP_TRY_DEPLOY, 10);
        con_printf("    ANTENNA DEPLOY FAIL [ ");
        con_printf(buffer); con_printf(" TRIES]\r\n");
    #endif

    drp_dep_write_deployed(0, tries_indx);

    return 0;
}

/**
 * Initializes all peripherals and subsystems.
 * @param param Not used.
 * @return 1 success, 0 fail.
 */
int dep_init_hw(void *param)
{
    int resp;
    STA_CubesatVar hw_isAlive;

    #if (SCH_TASKDEPLOYMENT_VERBOSE>=1)
        con_printf("\n[dep_init_hw] Initializig external hardware...\r\n");
    #endif

    #if (SCH_SYSBUS_ONBOARD==1)
    {
        #if (SCH_MEMEEPROM_ONBOARD==1)
        {
            #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
                con_printf("    * External MemEEPROM .. ");
            #endif
            resp = init_memEEPROM();
            hw_isAlive = sta_MemEEPROM_isAlive;
            sta_setCubesatVar(hw_isAlive, resp);
            #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
                if(resp == 0x01){
                    con_printf("Ok\r\n");
                }
                else{
                    con_printf("Fail\r\n");
                }
            #endif
        }
        #endif
    }
    #endif

    #if (SCH_RTC_ONBOARD==1)
    {
        #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
            con_printf("    * External RTC .. ");
        #endif
        resp = RTC_init();
        hw_isAlive = sta_RTC_isAlive;
        sta_setCubesatVar(hw_isAlive, resp);
        #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
            if(resp == 0x01){
                con_printf("Ok\r\n");
            }
            else{
                con_printf("Fail\r\n");
            }
        #endif
    }
    #endif

    #if (SCH_TRX_ONBOARD==1)
    {
        #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
            con_printf("    * External TRX .. ");
        #endif
        resp  = trx_initialize(NULL);
        hw_isAlive = dat_TRX_isAlive;
        sta_setCubesatVar(hw_isAlive, resp);
        #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
            if(resp == 0x01){
                con_printf("Ok\r\n");
            }
            else{
                con_printf("Fail\r\n");
            }
        #endif
    }
    #endif

    #if (SCH_MEMSD_ONBOARD==1)
    {
        #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
            con_printf("    * External MemSD .. ");
        #endif
        resp = dat_sd_init();
        hw_isAlive = sta_MemSD_isAlive;
        sta_setCubesatVar(hw_isAlive, resp);
        #if (SCH_TASKDEPLOYMENT_VERBOSE>=2)
            if(resp == 0x01){
                con_printf("Ok\r\n");
            }
            else{
                con_printf("Fail\r\n");
            }
        #endif
    }
    #endif

    return 1;
}

int dat_sd_init(void){
    //apagar energia MemSD
    PPC_MB_nON_SD=1;
    /* Un delay para poder inicializar conrrectamente la SD si el PIC se resetea */
    const unsigned long DelaySd = 3000 / portTICK_RATE_MS;
    vTaskDelay(DelaySd);
    //encender energia MemSD
    PPC_MB_nON_SD=0;
    unsigned char r = SD_init();
    if(r == 0){
        dat_onReset_memSD(FALSE);
        return 1;
    }
    else{
        return 0;
    }
}


//Libcsp defines and functions
#define MY_ADDRESS (0)
void dep_csp_initialization(void)
{
    csp_debug_set_level(CSP_INFO, 1);
    csp_debug_set_level(CSP_PACKET, 1);
    csp_debug_set_level(CSP_BUFFER, 1);
    csp_debug_set_level(CSP_ERROR, 1);
    csp_debug_set_level(CSP_WARN, 1);

    /* Init buffer system with 3 packets of maximum 256 bytes each */
    csp_buffer_init(3, I2C_MTU+5);

    /* Init CSP with address MY_ADDRESS */
    csp_init(MY_ADDRESS);
    csp_i2c_init(SCH_I2C1_ADDR, 0, 400);

    csp_route_set(CSP_DEFAULT_ROUTE, &csp_if_i2c, CSP_NODE_MAC);
    csp_route_start_task(2*configMINIMAL_STACK_SIZE, 3);

    /* Create socket without any socket options */
    csp_socket_t *sock = csp_socket(CSP_SO_NONE);

    /* Bind all ports to socket */
    csp_bind(sock, CSP_ANY);

    /* Create connections backlog queue */
    csp_listen(sock, 5);

    //DEBUG
    csp_debug_set_level(CSP_ERROR, 1);
    printf("\n---- Conn table ----\n");
    csp_conn_print_table();
    printf("---- Route table ----\n");
    csp_route_print_table();
    printf("---- Interfaces ----\n");
    csp_route_print_interfaces();

//    xTaskCreate(taskServerCSP, (signed char *)"SRV", 2*configMINIMAL_STACK_SIZE, NULL, 3, NULL);
}
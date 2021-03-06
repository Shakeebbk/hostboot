/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/usr/diag/mdia/mdiareasoncodes.H $                 */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2012,2018                        */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
/**
 * @file mdiareasoncodes.H
 *
 * @brief Reason codes and module ids for memory diagnostics
 *
 */
#ifndef __MDIA_MDIAREASONCODES_H
#define __MDIA_MDIAREASONCODES_H

#include <hbotcompid.H>

namespace MDIA
{

/**
* @enum Moduleid
*
* @brief Module Ids used in created errorlogs.  Indicates which
*      functions an error log was created in.
*
*/
enum ModuleId
{
    INVALID_MODULE                  = 0x00,
    PROCESS_COMMAND_TIMEOUT         = 0x01,
    MONITOR_MAIN_THREAD             = 0x02
};

/**
 * @enum ReasonCode
 *
 * @brief Reasoncodes used to describe what errors are being indicated.
 *
 */
enum ReasonCode
{
    INVALID_REASONCODE           = MDIA_COMP_ID | 0x00,  // Invalid Reasoncode
    MAINT_COMMAND_HW_TIMED_OUT   = MDIA_COMP_ID | 0x01,  // timeout due to HW
    MAINT_COMMAND_SW_TIMED_OUT   = MDIA_COMP_ID | 0x02,  // timeout due to SW
    MONITOR_THREAD_CRASHED       = MDIA_COMP_ID | 0x03   // thread crashed
};

}
#endif

/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/isteps/istep10/call_host_rng_bist.C $                 */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015,2017                        */
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
   @file call_host_rng_bist.C
 *
 *  Support file for IStep: nest_chiplets
 *   Nest Chiplets
 *
 *  HWP_IGNORE_VERSION_CHECK
 *
 */
/******************************************************************************/
// Includes
/******************************************************************************/
#include    <stdint.h>

#include    <trace/interface.H>
#include    <initservice/taskargs.H>
#include    <errl/errlentry.H>

#include    <isteps/hwpisteperror.H>
#include    <errl/errludtarget.H>
#include    <errl/errlreasoncodes.H>

#include    <initservice/isteps_trace.H>
#include    <initservice/initserviceif.H>

//  targeting support
#include    <targeting/common/commontargeting.H>
#include    <targeting/common/utilFilter.H>

//  MVPD
#include <devicefw/userif.H>
#include <vpd/mvpdenums.H>

#include <config.h>
#include <fapi2/plat_hwp_invoker.H>
#include <p9_rng_init_phase1.H>

namespace   ISTEP_10
{

using   namespace   ISTEP;
using   namespace   ISTEP_ERROR;
using   namespace   ERRORLOG;
using   namespace   TARGETING;

//******************************************************************************
// wrapper function to call host_rng_bist
//******************************************************************************
void* call_host_rng_bist( void *io_pArgs )
{

    errlHndl_t l_err = NULL;
    IStepError l_StepError;

    TRACFCOMP( ISTEPS_TRACE::g_trac_isteps_trace,
               "call_host_rng_bist entry" );
    //
    //  get a list of all the procs in the system
    //
    TARGETING::TargetHandleList l_cpuTargetList;
    getAllChips(l_cpuTargetList, TYPE_PROC);

    // Loop through all processors including master
    for (const auto & l_cpu_target: l_cpuTargetList)
    {
        const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>l_fapi2_proc_target(
                l_cpu_target);
        // Check for functional NX
        TARGETING::TargetHandleList l_nxTargetList;
        getChildChiplets(l_nxTargetList, l_cpu_target, TYPE_NX, true);
        if (l_nxTargetList.empty())
        {
            TRACFCOMP( ISTEPS_TRACE::g_trac_isteps_trace,
              "p9_rng_init_phase1: no functional NX found for proc %.8X",
              TARGETING::get_huid(l_cpu_target));
            continue;
        }
        TRACFCOMP( ISTEPS_TRACE::g_trac_isteps_trace,
          "Running p9_rng_init_phase1 HWP on processor target %.8X",
          TARGETING::get_huid(l_cpu_target) );

        FAPI_INVOKE_HWP(l_err, p9_rng_init_phase1, l_fapi2_proc_target);
        if(l_err)
        {
            TRACFCOMP(ISTEPS_TRACE::g_trac_isteps_trace,
                "ERROR: call p9_rng_init_phase1, PLID=0x%x, rc=0x%.4X",
                l_err->plid(), l_err->reasonCode());

            for (const auto l_callout : l_err->getUDSections(
                    HWPF_COMP_ID,
                    ERRORLOG::ERRL_UDT_CALLOUT))
            {
                if(reinterpret_cast<HWAS::callout_ud_t*>
                    (l_callout)->type == HWAS::HW_CALLOUT)
                {
                    for (const auto & l_nxTarget: l_nxTargetList)
                    {
                        l_err->addHwCallout( l_nxTarget,
                            HWAS::SRCI_PRIORITY_HIGH,
                            HWAS::DECONFIG,
                            HWAS::GARD_NULL );
                    }
                 }
            }

            l_StepError.addErrorDetails(l_err);
            errlCommit(l_err, HWPF_COMP_ID);
        }
    } // end of going through all processors

    TRACFCOMP( ISTEPS_TRACE::g_trac_isteps_trace,
               "call_host_rng_bist exit");

    return l_StepError.getErrorHandle();
}

};   // end namespace

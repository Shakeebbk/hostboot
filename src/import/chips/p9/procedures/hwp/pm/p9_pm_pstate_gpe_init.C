/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/import/chips/p9/procedures/hwp/pm/p9_pm_pstate_gpe_init.C $ */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015,2018                        */
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
///
/// @file p9_pm_pstate_gpe_init.C
/// @brief  Stop, reset and initalize/start the Pstate GPE
///
// *HWP HWP Owner       :    Greg Still  <stillgs@us.ibm.com>
// *HWP Backup Owner    :    Rahul Batra <rbatra@us.ibm.com>
// *HWP FW Owner        :    Prem S Jha  <premjha2@in.ibm.com>
// *HWP Team            :    PM
// *HWP Level           :    3
// *HWP Consumed by     :    HS

// -----------------------------------------------------------------------------
//  Includes
// -----------------------------------------------------------------------------
#include <p9_pm_pstate_gpe_init.H>
#include <p9_pm_hcd_flags.h>
#include <p9_ppe_defs.H>
#include <p9_ppe_utils.H>
#include <p9_quad_scom_addresses.H>
#include <p9_quad_scom_addresses_fld.H>
#include <p9n2_misc_scom_addresses.H>
#include <vector>

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------
// @todo RTC 148540 Refine this values for the lab
// Following constants hold an approximate value.
static const uint32_t PGPE_TIMEOUT_MS       = 500;
static const uint32_t PGPE_TIMEOUT_MCYCLES  = 20;
static const uint32_t PGPE_POLLTIME_MS      = 20;
static const uint32_t PGPE_POLLTIME_MCYCLES = 2;
static const uint32_t TIMEOUT_COUNT = PGPE_TIMEOUT_MS / PGPE_POLLTIME_MS;

// -----------------------------------------------------------------------------
//  Function definitions
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  Pstate GPE Initialization Function
// -----------------------------------------------------------------------------

/// @brief Initializes the Pstate GPE and related Pstate functions on a chip
///
/// @param [in] i_target Chip target
///
/// @retval FAPI_RC_SUCCESS else ERROR defined in xml

fapi2::ReturnCode pstate_gpe_init(
    const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target)
{
    fapi2::buffer<uint64_t> l_data64;
    fapi2::buffer<uint64_t> l_occ_scratch2;
    fapi2::buffer<uint64_t> l_xcr;
    fapi2::buffer<uint64_t> l_xsr_iar;
    fapi2::buffer<uint64_t> l_ivpr;
    uint32_t                l_xsr_halt_condition = 0;
    uint32_t                l_timeout_counter = TIMEOUT_COUNT;
    const fapi2::Target<fapi2::TARGET_TYPE_SYSTEM>      FAPI_SYSTEM;
    fapi2::ATTR_PSTATEGPE_BOOT_COPIER_IVPR_OFFSET_Type  l_ivpr_offset = 0;
    fapi2::ATTR_VDD_AVSBUS_BUSNUM_Type                  l_avsbus_number = 0;
    fapi2::ATTR_VDD_AVSBUS_RAIL_Type                    l_avsbus_rail = 0;
    fapi2::ATTR_SYSTEM_PSTATES_MODE_Type                l_pstates_mode = 0;
    std::vector<uint64_t> l_pgpe_base_addr;
    l_pgpe_base_addr.push_back( PGPE_BASE_ADDRESS );

    FAPI_IMP(">> pstate_gpe_init......");

    FAPI_TRY( FAPI_ATTR_GET(fapi2::ATTR_PSTATEGPE_BOOT_COPIER_IVPR_OFFSET,
                            i_target,
                            l_ivpr_offset),
              "Error getting ATTR_PSTATEGPE_BOOT_COPIER_IVPR_OFFSET");


    // Program PGPE IVPR
    l_ivpr.flush<0>().insertFromRight<0, 32>(l_ivpr_offset);
    FAPI_INF("  Writing IVPR with 0x%16llX", l_ivpr);
    FAPI_TRY(putScom(i_target, PU_GPE2_GPEIVPR_SCOM, l_ivpr));

    // Set up the OCC Scratch 2 register before PGPE boot
    FAPI_TRY(getScom(i_target, PU_OCB_OCI_OCCS2_SCOM, l_occ_scratch2));

    FAPI_INF("  Clear PGPE_ACTIVE in OCC Scratch 2 Register...");
    l_occ_scratch2.clearBit<p9hcd::PGPE_ACTIVE>();

    FAPI_TRY( FAPI_ATTR_GET(fapi2::ATTR_VDD_AVSBUS_BUSNUM,
                            i_target,
                            l_avsbus_number),
              "Error getting ATTR_VDD_AVSBUS_BUSNUM");
    FAPI_TRY( FAPI_ATTR_GET(fapi2::ATTR_VDD_AVSBUS_RAIL,
                            i_target,
                            l_avsbus_rail),
              "Error getting ATTR_VDD_AVSBUS_RAIL");

    l_occ_scratch2.insertFromRight<27, 1>(l_avsbus_number)
    .insertFromRight<28, 4>(l_avsbus_rail);

    FAPI_INF("  AVSBus VDD topology set in OCC Scratch 2 Register:  Bus = %d, Rail = %d ...",
             l_avsbus_number, l_avsbus_rail);

    FAPI_TRY(putScom(i_target, PU_OCB_OCI_OCCS2_SCOM, l_occ_scratch2));


    FAPI_TRY( FAPI_ATTR_GET(fapi2::ATTR_SYSTEM_PSTATES_MODE,
                            FAPI_SYSTEM,
                            l_pstates_mode),
              "Error getting ATTR_SYSTEM_PSTATES_MODE");

    fapi2::ATTR_PSTATES_ENABLED_Type l_ps_enabled;
    FAPI_TRY( FAPI_ATTR_GET(fapi2::ATTR_PSTATES_ENABLED,
                            i_target,
                            l_ps_enabled),
              "Error getting ATTR_PSTATES_ENABLED");


    // Boot if not OFF
    if (l_pstates_mode != fapi2::ENUM_ATTR_SYSTEM_PSTATES_MODE_OFF &&
        (l_ps_enabled == fapi2::ENUM_ATTR_PSTATES_ENABLED_TRUE) )
    {
        // Set auto mode if needed
        if (l_pstates_mode == fapi2::ENUM_ATTR_SYSTEM_PSTATES_MODE_AUTO)
        {
            FAPI_INF("  Pstate Auto Start Mode Enabled...");
            FAPI_TRY(putScom(i_target, PU_OCB_OCI_OCCFLG_SCOM2, BIT(p9hcd::PGPE_PSTATE_PROTOCOL_AUTO_ACTIVATE)));
        }

        // Setup the PGPE Timer Selects
        // These hardcoded values are assumed by the PGPE Hcode for setting up
        // the FIT and Watchdog values a based on the nest frequency that is
        // passed to it via the PGPE header.
        l_data64.flush<0>()
        .insertFromRight<0, 4>(0x1)    // Watchdog
        .insertFromRight<4, 4>(0xA);   // FIT
        FAPI_TRY(fapi2::putScom(i_target, PU_GPE2_GPETSEL_SCOM, l_data64));

        // Program XCR to ACTIVATE PGPE
        FAPI_INF("   Starting the PGPE...");
        l_xcr.flush<0>().insertFromRight(p9hcd::HARD_RESET, 1, 3);
        FAPI_TRY(putScom(i_target, PU_GPE2_PPE_XIXCR, l_xcr));
        l_xcr.flush<0>().insertFromRight(p9hcd::TOGGLE_XSR_TRH, 1 , 3);
        FAPI_TRY(putScom(i_target, PU_GPE2_PPE_XIXCR, l_xcr));
        l_xcr.flush<0>().insertFromRight(p9hcd::RESUME, 1, 3);
        FAPI_TRY(putScom(i_target, PU_GPE2_PPE_XIXCR, l_xcr));

        // Now wait for PGPE to boot
        FAPI_DBG("   Poll for PGPE Active for %d ms", PGPE_TIMEOUT_MS);
        l_occ_scratch2.flush<0>();
        l_xsr_iar.flush<0>();

        do
        {
            FAPI_TRY(getScom(i_target, PU_OCB_OCI_OCCS2_SCOM, l_occ_scratch2));
            FAPI_TRY(getScom(i_target, PU_GPE2_PPE_XIDBGPRO, l_xsr_iar));
            FAPI_DBG("OCC Scratch2: 0x%016lx; XSR: 0x%016lx Timeout: %d",
                     l_occ_scratch2, l_xsr_iar, l_timeout_counter);
            // fapi2::delay takes ns as the arg
            fapi2::delay(PGPE_POLLTIME_MS * 1000 * 1000, PGPE_POLLTIME_MCYCLES * 1000 * 1000);
        }
        while((l_occ_scratch2.getBit<p9hcd::PGPE_ACTIVE>() != 1) &&
              (l_xsr_iar.getBit<p9hcd::HALTED_STATE>() != 1) &&
              (--l_timeout_counter != 0));

        // Extract the halt condition
        l_xsr_iar.extractToRight<uint32_t>(l_xsr_halt_condition,
                                           p9hcd::HALT_CONDITION_START,
                                           p9hcd::HALT_CONDITION_LEN);
        FAPI_DBG("halt state: XSR/IAR: 0x%016lx condition: %d",
                 l_xsr_iar, l_xsr_halt_condition);


        // Check for a debug halt condition
        FAPI_ASSERT(!((l_xsr_iar.getBit<p9hcd::HALTED_STATE>() == 1) &&
                      ((l_xsr_halt_condition == p9hcd::DEBUG_HALT ||
                        l_xsr_halt_condition == p9hcd::DBCR_HALT)   )),
                    fapi2::PSTATE_GPE_INIT_DEBUG_HALT()
                    .set_CHIP(i_target)
                    .set_PGPE_BASE_ADDRESS(l_pgpe_base_addr)
                    .set_PGPE_STATE_MODE(HALT),
                    "Pstate GPE Debug Halt detected");

        // When PGPE fails to boot, assert out
        FAPI_ASSERT((l_timeout_counter != 0 &&
                     l_occ_scratch2.getBit<p9hcd::PGPE_ACTIVE>() == 1 &&
                     l_xsr_iar.getBit<p9hcd::HALTED_STATE>() != 1),
                    fapi2::PSTATE_GPE_INIT_TIMEOUT()
                    .set_CHIP(i_target)
                    .set_PGPE_BASE_ADDRESS(l_pgpe_base_addr)
                    .set_PGPE_STATE_MODE(HALT),
                    "Pstate GPE Init timeout");

        if(l_occ_scratch2.getBit<p9hcd::PGPE_ACTIVE>())
        {
            FAPI_INF("  PGPE was activated successfully!!!!");
        }

        if (l_pstates_mode == fapi2::ENUM_ATTR_SYSTEM_PSTATES_MODE_AUTO)
        {
            do
            {
                FAPI_TRY(getScom(i_target, PU_OCB_OCI_OCCS2_SCOM, l_occ_scratch2));
                FAPI_TRY(getScom(i_target, PU_GPE3_PPE_XIDBGPRO, l_xsr_iar));
                // fapi2::delay takes ns as the arg
                fapi2::delay(PGPE_POLLTIME_MS * 1000 * 1000, PGPE_POLLTIME_MCYCLES * 1000 * 1000);
            }
            while((l_occ_scratch2.getBit<p9hcd::PGPE_PSTATE_PROTOCOL_ACTIVE>() != 1) &&
                  (l_xsr_iar.getBit<p9hcd::HALTED_STATE>() != 1) &&
                  (--l_timeout_counter != 0));

            // When Pstate protocol fails to start, post a log
            if (l_timeout_counter != 0 &&
                l_occ_scratch2.getBit<p9hcd::PGPE_PSTATE_PROTOCOL_ACTIVE>() == 1 &&
                l_xsr_iar.getBit<p9hcd::HALTED_STATE>() != 1)
            {
                FAPI_INF("  Pstate Auto Start Mode Complete!!!!");
            }
            else
            {
                FAPI_ASSERT_NOEXIT(false,
                                   fapi2::PSTATE_GPE_INIT_PSTATE_AUTOSTART_TIMEOUT()
                                   .set_CHIP(i_target),
                                   "Pstate GPE Protocol Auto Start timeout");
            }
        }

        //Update QPPM_QPMMR_FSAFE
        const fapi2::Target<fapi2::TARGET_TYPE_SYSTEM> FAPI_SYSTEM;
        auto l_eqChiplets = i_target.getChildren<fapi2::TARGET_TYPE_EQ>
                            (fapi2::TARGET_STATE_FUNCTIONAL);
        fapi2::ATTR_SAFE_MODE_FREQUENCY_MHZ_Type l_attr_safe_mode_freq_mhz;
        fapi2::ATTR_FREQ_PROC_REFCLOCK_KHZ_Type l_ref_clock_freq_khz;
        fapi2::ATTR_PROC_DPLL_DIVIDER_Type l_proc_dpll_divider;
        FAPI_TRY(FAPI_ATTR_GET(fapi2::ATTR_SAFE_MODE_FREQUENCY_MHZ, i_target, l_attr_safe_mode_freq_mhz));
        FAPI_TRY(FAPI_ATTR_GET(fapi2::ATTR_FREQ_PROC_REFCLOCK_KHZ, FAPI_SYSTEM, l_ref_clock_freq_khz));
        FAPI_TRY(FAPI_ATTR_GET(fapi2::ATTR_PROC_DPLL_DIVIDER, i_target, l_proc_dpll_divider));
        // Convert frequency value to a format that needs to be written to the
        // register
        uint32_t l_safe_mode_freq = ((l_attr_safe_mode_freq_mhz * 1000) * l_proc_dpll_divider) /
                                    l_ref_clock_freq_khz;

        for (auto l_eq_chplt : l_eqChiplets)
        {
            FAPI_TRY(getScom(l_eq_chplt, EQ_QPPM_QPMMR, l_data64));
            //FSAFE
            l_data64.insertFromRight<EQ_QPPM_QPMMR_FSAFE,
                                     EQ_QPPM_QPMMR_FSAFE_LEN>(l_safe_mode_freq);

            FAPI_TRY(fapi2::putScom(l_eq_chplt, EQ_QPPM_QPMMR, l_data64),
                     "ERROR:Failed to write for EQ_QPPM_QPMMR");
        }
    }
    else
    {
        FAPI_INF("  PGPE booting is disabled and is NOT running!!!!");
    }

fapi_try_exit:
    FAPI_IMP("<< pstate_gpe_init......");
    return fapi2::current_err;
}

// -----------------------------------------------------------------------------
//  Pstate GPE Reset Function
// -----------------------------------------------------------------------------

/// @brief Stops the Pstate GPE
///
/// @param [in] i_target Chip target
///
/// @retval FAPI_RC_SUCCESS else ERROR defined in xml

fapi2::ReturnCode pstate_gpe_reset(
    const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target)
{

    fapi2::buffer<uint64_t> l_data64;
    uint32_t                l_timeout_in_MS = 100;
    std::vector<uint64_t> l_pgpe_base_addr;
    l_pgpe_base_addr.push_back( PGPE_BASE_ADDRESS );


    FAPI_IMP(">> pstate_gpe_reset...");


    //Disable CME monitoring of PGPE heart beat loss before halting PGPE
    auto l_eqChiplets = i_target.getChildren<fapi2::TARGET_TYPE_EQ>
                        (fapi2::TARGET_STATE_FUNCTIONAL);

    // For each functional EQ chiplet
    for (auto l_quad_chplt : l_eqChiplets)
    {
        l_data64.flush<0>()
        .setBit<EQ_QPPM_QPMMR_ENABLE_PCB_INTR_UPON_HEARTBEAT_LOSS>();

        FAPI_TRY(fapi2::putScom(l_quad_chplt, EQ_QPPM_QPMMR_CLEAR, l_data64),
                 "ERROR: Failed to setup Quad PPM register");

    }

    // Program XCR to HALT PGPE
    FAPI_INF("   Send HALT command via XCR...");
    l_data64.flush<0>().insertFromRight(p9hcd::HALT, 1, 3);
    FAPI_TRY(putScom(i_target, PU_GPE2_PPE_XIXCR, l_data64));

    // Now wait for PGPE to be halted.
    // FAPI_TRY(GPE2.is_halted(&b_halted_state,
    //                         timeout_value_ns,
    //                         timeout_value_simcycles));
    FAPI_INF("   Poll for HALT State via XSR...");

    do
    {
        FAPI_TRY(getScom(i_target, PU_GPE2_GPEXIXSR_SCOM, l_data64));
        fapi2::delay(PGPE_POLLTIME_MS * 1000 * 1000, PGPE_POLLTIME_MCYCLES * 1000 * 1000);
    }
    while((l_data64.getBit<p9hcd::HALTED_STATE>() == 0) &&
          (--l_timeout_in_MS != 0));

    // When PGPE fails to halt, then assert ot
    FAPI_ASSERT((l_timeout_in_MS != 0),
                fapi2::PSTATE_GPE_RESET_TIMEOUT()
                .set_CHIP(i_target)
                .set_PGPE_BASE_ADDRESS(l_pgpe_base_addr)
                .set_PGPE_STATE_MODE(HALT),
                "PSTATE GPE Reset timeout");

    FAPI_TRY(getScom(i_target, PU_OCB_OCI_OCCS2_SCOM, l_data64));
    FAPI_INF("   Clear PGPE_ACTIVE in OCC Flag Register...");
    l_data64.flush<0>().clearBit<p9hcd::PGPE_ACTIVE>();
    FAPI_TRY(putScom(i_target, PU_OCB_OCI_OCCS2_SCOM, l_data64));

fapi_try_exit:
    FAPI_IMP("<< pstate_gpe_reset...");
    return fapi2::current_err;

}

// -----------------------------------------------------------------------------
//  p9_pm_pstate_gpe_init Function
// -----------------------------------------------------------------------------
fapi2::ReturnCode p9_pm_pstate_gpe_init(
    const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target,
    const p9pm::PM_FLOW_MODE i_mode)
{
    FAPI_IMP("> p9_pm_pstate_gpe_init");

    const char* PM_MODE_NAME_VAR; //Defines storage for PM_MODE_NAME
    FAPI_INF("Executing p9_pstate_gpe_init in mode %s", PM_MODE_NAME(i_mode));

    // Boot the PSTATE GPE
    if (i_mode == p9pm::PM_INIT)
    {
        FAPI_TRY(pstate_gpe_init(i_target),
                 "ERROR: failed to initialize Pstate GPE");

        // Setup the PBA channels for run-time operation (eg when the PPC405 and its GPEs are active).
        FAPI_EXEC_HWP(fapi2::current_err,
                      p9_pm_pba_init,
                      i_target,
                      p9pm::PM_INIT);

        FAPI_ASSERT(fapi2::current_err == fapi2::FAPI2_RC_SUCCESS,
                    fapi2::PSTATE_GPE_PBA_INIT_FAILED()
                    .set_CHIP(i_target)
                    .set_MODE(p9pm::PM_INIT),
                    "PBA Setup Failed" );
    }

    // Reset the PSTATE GPE
    else if (i_mode == p9pm::PM_RESET)
    {
        FAPI_TRY(pstate_gpe_reset(i_target), "ERROR:failed to reset Pstate GPE");
    }

    // Unsupported Mode
    else
    {
        FAPI_ASSERT(false,
                    fapi2::PSTATE_GPE_BAD_MODE()
                    .set_BADMODE(i_mode),
                    "ERROR; Unknown mode passed to pstate_gpe_init. Mode %x",
                    i_mode);
    }

fapi_try_exit:
    FAPI_IMP("< p9_pm_pstate_gpe_init");
    return fapi2::current_err;
}

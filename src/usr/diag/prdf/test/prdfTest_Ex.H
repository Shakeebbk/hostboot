/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/diag/prdf/test/prdfTest_Ex.H $                        */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2012,2017                        */
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
#ifndef __TEST_PRDFEXTEST_H
#define __TEST_PRDFEXTEST_H

#ifdef __HOSTBOOT_MODULE
  #include <cxxtest/TestSuite.H>
  #include <errl/errlentry.H>
  #include <errl/errlmanager.H>
#else
  #include <cxxtest/TestSuite.h>
  #include <fsp/FipsGlobalFixture.H>
  #include <errlentry.H>
#endif

#include <prdfTrace.H>
#include <prdfMain.H>
#include "prdfsimMacros.H"
#include "prdfsimMfgSync.H"
#include <prdfGlobal.H>
#include <prdfErrlUtil.H>

class ExLFIR:public CxxTest::TestSuite
{
    public:

        void TestRecoverable(void)
        {
            PRDS_BEGIN("ECLFIR recoverable attention");
            PRDS_ERROR_ENTRY("NODE{0}:PROC{0}", PRDF::RECOVERABLE);
            PRDS_EXPECTED_SIGNATURE("NODE{0}:PROC{0}:EQ{0}:EX{0}:CORE{0}",
                                     0x33e70007);
            // GLOBAL_RE_FIR set bit 32
            PRDS_SCR_WRITE("NODE{0}:PROC{0}", 0x500F001B, 0x0000000080000000);
            // EC_CHIPLET_RE_FIR[1] Attention from ECLFIR
            PRDS_SCR_WRITE("NODE{0}:PROC{0}:EQ{0}:EX{0}:CORE{0}",
                            0x20040001, 0x4000000000000000);
            // set ECLFIR[7]
            PRDS_SCR_WRITE("NODE{0}:PROC{0}:EQ{0}:EX{0}:CORE{0}",
                            0x2004000a, 0x0100000000000000);
            // ECLFIR_ACT1
            PRDS_SCR_WRITE("NODE{0}:PROC{0}:EQ{0}:EX{0}:CORE{0}",
                            0x20040011, 0xFFFFFFFFFFFFFFFF);

            PRDS_START_SIM();
            if(!(PRDS_END()))
            {
                TS_FAIL("Expected Signature Failed ECLFIR recoverable attention");
            }
        }

        void TestL3CE(void)
        {

            PRDS_BEGIN("L3CE Test");
            PRDS_ERROR_ENTRY("NODE{0}:PROC{0}", PRDF::RECOVERABLE);
            PRDS_EXPECTED_SIGNATURE("NODE{0}:PROC{0}:EQ{0}:EX{0}", 0xf08d0004);
            // GLOBAL_RE_FIR bit 16
            PRDS_SCR_WRITE("NODE{0}:PROC{0}", 0x500F001B, 0x0000800000000000);
            // EX_CHIPLET_RE_FIR[4] - Analyze L3FIR
            PRDS_SCR_WRITE("NODE{0}:PROC{0}:EQ{0}", 0x10040001,
                            0x2000000000000000);
            // EX_L3FIR
            PRDS_SCR_WRITE("NODE{0}:PROC{0}:EQ{0}:EX{0}",
                            0x10011800, 0x0800000000000000);
            // EX_L3FIR_ACT1
            PRDS_SCR_WRITE("NODE{0}:PROC{0}:EQ{0}:EX{0}",
                            0x10011807, 0x0800000000000000);
            PRDS_START_SIM();
            if(!(PRDS_END()))
            {
                TS_FAIL("L3CE Test failed");
            }
        }

        void TestCoreRecoverable(void)
        {
            PRDS_BEGIN("COREFIR recoverable attention");
            PRDS_ERROR_ENTRY("NODE{0}:PROC{0}", PRDF::RECOVERABLE);
            PRDS_EXPECTED_SIGNATURE("NODE{0}:PROC{0}:EQ{0}:EX{0}:CORE{0}",
                                     0xdd3f003c);

            // GLOBAL_RE_FIR
            PRDS_SCR_WRITE("NODE{0}:PROC{0}", 0x500F001B, 0x0000000080000000);
            // GLOBALUNITXSTPFIR
            PRDS_SCR_WRITE("NODE{0}:PROC{0}", 0x50040018, 0x0000000000000000);
            // EX_CHIPLET_RE_FIR
            PRDS_SCR_WRITE("NODE{0}:PROC{0}:EQ{0}:EX{0}:CORE{0}",
                            0x20040001, 0x2000000000000000);
            // EX_CHIPLET_RE_FIR_MASK
            PRDS_SCR_WRITE("NODE{0}:PROC{0}:EQ{0}:EX{0}:CORE{0}",
                            0x20040002, 0x20ffffc000000000);
            // COREFIR bit 60 on
            PRDS_SCR_WRITE("NODE{0}:PROC{0}:EQ{0}:EX{0}:CORE{0}",
                            0x20010a40, 0x0000000000000008);
            // COREFIR WOF bit 60 on
            PRDS_SCR_WRITE("NODE{0}:PROC{0}:EQ{0}:EX{0}:CORE{0}",
                            0x20010A48, 0x0000000000000008);
            // COREFIR_ACT0
            PRDS_SCR_WRITE("NODE{0}:PROC{0}:EQ{0}:EX{0}:CORE{0}",
                            0x20010A46, 0x15a20c688a448b01);
            // COREFIR_ACT1
            PRDS_SCR_WRITE("NODE{0}:PROC{0}:EQ{0}:EX{0}:CORE{0}",
                            0x20010A47, 0xfefc17f7ff9c8a09);
            // COREHMEER
            PRDS_SCR_WRITE("NODE{0}:PROC{0}:EQ{0}:EX{0}:CORE{0}",
                            0x20010A96, 0xac10080000000000);
            // SPATTN_0
            PRDS_SCR_WRITE("NODE{0}:PROC{0}:EQ{0}:EX{0}:CORE{0}",
                            0x20010A99, 0x0000000000000000);

            PRDS_START_SIM();
            if(!(PRDS_END()))
            {
                TS_FAIL("COREFIR recoverable attention test failed");
            }
        }

};
#endif

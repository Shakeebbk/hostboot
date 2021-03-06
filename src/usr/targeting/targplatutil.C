/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/targeting/targplatutil.C $                            */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2013,2018                        */
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
 *  @file targeting/targplatutil.C
 *
 *  @brief Provides implementation for general platform specific utilities
 *      to support core functions
 */

//******************************************************************************
// Includes
//******************************************************************************

// STD
#include <stdlib.h>

// TARG
#include <targeting/targplatutil.H>
#include <targeting/common/predicates/predicates.H>
#include <targeting/common/utilFilter.H>
#include <errl/errlmanager.H>
#include <config.h>
#include <algorithm>
namespace TARGETING
{

namespace UTIL
{

#ifdef CONFIG_BMC_IPMI
    uint32_t getIPMISensorNumber( const TARGETING::Target*& i_targ,
        TARGETING::SENSOR_NAME i_name );
#endif


#define TARG_NAMESPACE "TARGETING::UTIL"
#define TARG_CLASS ""

//******************************************************************************
// createTracingError
//******************************************************************************

void createTracingError(
    const uint8_t     i_modId,
    const uint16_t    i_reasonCode,
    const uint32_t    i_userData1,
    const uint32_t    i_userData2,
    const uint32_t    i_userData3,
    const uint32_t    i_userData4,
          errlHndl_t& io_pError)
{
    errlHndl_t pNewError = new ERRORLOG::ErrlEntry(
                                   ERRORLOG::ERRL_SEV_INFORMATIONAL,
                                   i_modId,
                                   i_reasonCode,
                                   static_cast<uint64_t>(i_userData1) << 32
                                       | (i_userData2 & (0xFFFFFFFF)),
                                   static_cast<uint64_t>(i_userData3) << 32
                                       | (i_userData4 & (0xFFFFFFFF)));
    if(io_pError != NULL)
    {
        // Tie logs together; existing log primary, new log as secondary
        pNewError->plid(io_pError->plid());
        errlCommit(pNewError,TARG_COMP_ID);
    }
    else
    {
        io_pError = pNewError;
        pNewError = NULL;
    }

    return;
}

void getMasterNodeTarget(Target*& o_masterNodeTarget)
{
    Target* masterNodeTarget = NULL;
    PredicateCTM nodeFilter(CLASS_ENC, TYPE_NODE);
    TargetRangeFilter localBlueprintNodes(
                targetService().begin(),
                targetService().end(),
                &nodeFilter);
    if(localBlueprintNodes)
    {
        masterNodeTarget = *localBlueprintNodes;
    }

    o_masterNodeTarget = masterNodeTarget;
}

bool isCurrentMasterNode()
{
#if 0
    // Get node target
    TARGETING::TargetHandleList l_nodelist;
    getEncResources(l_nodelist, TARGETING::TYPE_NODE,
                    TARGETING::UTIL_FILTER_FUNCTIONAL);
    assert(l_nodelist.size() == 1, "ERROR, only expect one node.");
    auto isMaster = l_nodelist[0]->getAttr<TARGETING::ATTR_IS_MASTER_DRAWER>();

    return (isMaster == 1);
#endif
    return true;
}

#ifndef __HOSTBOOT_RUNTIME
Target* getCurrentNodeTarget(void)
{
    // Get node target
    TargetHandleList l_nodelist;
    getEncResources(l_nodelist, TARGETING::TYPE_NODE,
                    TARGETING::UTIL_FILTER_FUNCTIONAL);
    assert(l_nodelist.size() == 1, "ERROR, only expect one node.");

    Target* pTgt =  l_nodelist[0];
    assert(pTgt != nullptr, "getCurrentNodeTarget found nullptr");

    return pTgt;
}

uint8_t getCurrentNodePhysId(void)
{
    Target* pNodeTgt = getCurrentNodeTarget();
    EntityPath epath = pNodeTgt->getAttr<TARGETING::ATTR_PHYS_PATH>();
    const TARGETING::EntityPath::PathElement pe =
              epath.pathElementOfType(TARGETING::TYPE_NODE);
    return pe.instance;
}
#endif

// return the sensor number from the passed in target
uint32_t getSensorNumber( const TARGETING::Target* i_pTarget,
                          TARGETING::SENSOR_NAME i_name )
{

#ifdef CONFIG_BMC_IPMI
    // get the IPMI sensor number from the array, these are unique for each
    // sensor + sensor owner in an IPMI system
    return getIPMISensorNumber( i_pTarget, i_name );
#else
    // pass back the HUID - this will be the sensor number for non ipmi based
    // systems
    return get_huid( i_pTarget );

#endif

}

// convert sensor number to a target
TARGETING::Target * getSensorTarget(const uint32_t i_sensorNumber )
{

#ifdef CONFIG_BMC_IPMI
    return TARGETING::UTIL::getTargetFromIPMISensor( i_sensorNumber );
#else
    // in non ipmi systems huid == sensor number
    return Target::getTargetFromHuid( i_sensorNumber );
#endif

}

// predicate for binary search of ipmi sensors array.
// given an array[][2] compare the sensor name, located in the first column,
// to the passed in key value
static inline bool name_predicate( uint16_t (&a)[2], uint16_t key )
{
    return  a[TARGETING::IPMI_SENSOR_ARRAY_NAME_OFFSET] < key;
};

#ifdef  CONFIG_BMC_IPMI
// given a target and sensor name, return the IPMI sensor number
// from the IPMI_SENSORS attribute.
uint32_t getIPMISensorNumber( const TARGETING::Target*& i_targ,
        TARGETING::SENSOR_NAME i_name )
{
    // $TODO RTC:123035 investigate pre-populating some info if we end up
    // doing this multiple times per sensor
    //
    // Helper function to search the sensor data for the correct sensor number
    // based on the sensor name.
    //
    uint8_t l_sensor_number = INVALID_IPMI_SENSOR;

    const TARGETING::Target * l_targ = i_targ;

    if( i_targ == NULL )
    {
        TARGETING::Target * sys;
        // use the system target
        TARGETING::targetService().getTopLevelTarget(sys);

        // die if there is no system target
        assert(sys);

        l_targ = sys;
    }

    TARGETING::AttributeTraits<TARGETING::ATTR_IPMI_SENSORS>::Type l_sensors;

    // if there is no sensor attribute, we will return INVALID_IPMI_SENSOR(0xFF)
    if(  l_targ->tryGetAttr<TARGETING::ATTR_IPMI_SENSORS>(l_sensors) )
    {
        // get the number of rows by dividing the total size by the size of
        // the first row
        uint16_t array_rows = (sizeof(l_sensors)/sizeof(l_sensors[0]));

        // create an iterator pointing to the first element of the array
        uint16_t (*begin)[2]  = &l_sensors[0];

        // using the number entries as the index into the array will set the
        // end iterator to the correct position or one entry past the last
        // element of the array
        uint16_t (*end)[2] = &l_sensors[array_rows];

        uint16_t (*ptr)[2] =
            std::lower_bound(begin, end, i_name, &name_predicate);

        // we have not reached the end of the array and the iterator
        // returned from lower_bound is pointing to an entry which equals
        // the one we are searching for.
        if( ( ptr != end ) &&
                ( (*ptr)[TARGETING::IPMI_SENSOR_ARRAY_NAME_OFFSET] == i_name ) )
        {
            // found it
            l_sensor_number =
                (*ptr)[TARGETING::IPMI_SENSOR_ARRAY_NUMBER_OFFSET];

        }
    }
    return l_sensor_number;
}

class number_predicate
{
    public:

        number_predicate( const uint32_t number )
            :iv_number(number)
        {};

        bool operator()( const uint16_t (&a)[2] ) const
        {
            return  a[TARGETING::IPMI_SENSOR_ARRAY_NUMBER_OFFSET] == iv_number;
        }

    private:
        uint32_t iv_number;
};

//******************************************************************************
// getTargetFromIPMISensor()
//******************************************************************************
Target * getTargetFromIPMISensor( uint32_t i_sensorNumber )
{

    // if the size of HUID is made larger than a uint32_t the compile will fail
    CPPASSERT((sizeof(TARGETING::ATTR_HUID_type) > sizeof(i_sensorNumber)));

    // 1. find targets with IPMI_SENSOR attribute which has sensor numbers
    // 2. search array for the sensor number (not sorted on this column)
    // 3. return the target

    TARGETING::Target * l_target = NULL;

    TARGETING::AttributeTraits<TARGETING::ATTR_IPMI_SENSORS>::Type l_sensors;

    for (TargetIterator itr = TARGETING::targetService().begin();
            itr != TARGETING::targetService().end(); ++itr)
    {
        // if there is a sensors attribute, see if our number is in it
        if( (*itr)->tryGetAttr<TARGETING::ATTR_IPMI_SENSORS>(l_sensors))
        {
            uint16_t array_rows = (sizeof(l_sensors)/sizeof(l_sensors[0]));

            // create an iterator pointing to the first sensor number of the
            // array.
            uint16_t (*begin)[2] =
                &l_sensors[TARGETING::IPMI_SENSOR_ARRAY_NAME_OFFSET];

            // using the number entries as the index into the array will set the
            // end iterator to the correct position or one entry past the last
            // element of the array
            uint16_t (*end)[2] = &l_sensors[array_rows];

            uint16_t (*ptr)[2] = std::find_if(begin, end,
                                              number_predicate(i_sensorNumber));

            if ( ptr != end )
            {
                // found it
                l_target = *itr;
                break;
            }
        }
    }
    return l_target;
}


#endif

#undef TARG_NAMESPACE
#undef TARG_CLASS

} // End namespace TARGETING::UTIL

} // End namespace TARGETING


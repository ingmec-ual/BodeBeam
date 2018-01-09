/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2018, Universidad de Almeria
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */


#pragma once

#include <stdlib.h>

#include <stdint.h>

#if !defined(__AVR_MEGA__)
#	pragma pack(push, 1) // exact fit - no padding
#endif

/*
START_FLAG   |  OPCODE  |  DATA_LEN   |   DATA      |    CHECKSUM    | END_FLAG |
  0x69          1 byte      1 byte       N bytes       =sum(data)       0x96
*/

#define FRAME_START_FLAG  0x69
#define FRAME_END_FLAG    0x96

enum opcode_t {
	// -----------------------------
	// COMMANDS PC -> uC
	// -----------------------------
	OP_NOP             = 0x00,
};

template <typename Payload>
struct TBaseFrame
{
    const uint8_t  START_FLAG;
    const uint8_t  OPCODE;
    const uint8_t  DATALEN;
    // --------- Payload -------
    Payload  payload;
    // -------------------------
    uint8_t  CHECKSUM;
    const uint8_t  END_FLAG;

    // Defaults:
    TBaseFrame(uint8_t opcode) :
            START_FLAG(FRAME_START_FLAG),
            OPCODE(opcode),
            DATALEN(sizeof(Payload)),
            END_FLAG(FRAME_END_FLAG)
    {
    }

    void calc_and_update_checksum()
    {
        CHECKSUM = calc_checksum();
    }

    uint8_t calc_checksum() const
    {
        const uint8_t len   = DATALEN; //reinterpret_cast<const uint8_t*>(ptr_frame)[2];
        const uint8_t *data = reinterpret_cast<const uint8_t*>(&payload);
        uint8_t ret =0;
        for (unsigned int i=0;i<len;i++) ret+=*data++;
        return ret;
    }
};

struct TFrameCMD_NOP_payload_t
{
};
struct TFrameCMD_NOP : public TBaseFrame<TFrameCMD_NOP_payload_t>
{
	TFrameCMD_NOP() : TBaseFrame(OP_NOP)
	{
	}
};


#if !defined(__AVR_MEGA__)
#	pragma pack(pop)
#endif

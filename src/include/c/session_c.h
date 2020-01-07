/*
   Copyright 2020 Sonoran Video Systems

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef __LIBCOYOTE_SESSION_C_H__
#define __LIBCOYOTE_SESSION_C_H__
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "../macros.h"
#include "../common.h"
#include "../datastructures_c.h"
#include "../statuscodes.h"


void CoyoteSession_Destroy(struct CoyoteSession *Sess);
struct CoyoteSession *CoyoteSession_New(const char *Host);

void CoyoteObject_Destroy(void *Object);

EXPFUNC CoyoteStatusCode Coyote_Take(struct CoyoteSession *Sess, const int32_t PK);
EXPFUNC CoyoteStatusCode Coyote_Pause(struct CoyoteSession *Sess, const int32_t PK);
EXPFUNC CoyoteStatusCode Coyote_SetPause(struct CoyoteSession *Sess, const int32_t PK);
EXPFUNC CoyoteStatusCode Coyote_UnsetPause(struct CoyoteSession *Sess, const int32_t PK);
EXPFUNC CoyoteStatusCode Coyote_SetPausedState(struct CoyoteSession *Sess, const int32_t PK, const bool Value);
EXPFUNC CoyoteStatusCode Coyote_End(struct CoyoteSession *Sess, const int32_t PK);
EXPFUNC CoyoteStatusCode Coyote_SelectPrev(struct CoyoteSession *Sess);
EXPFUNC CoyoteStatusCode Coyote_SelectNext(struct CoyoteSession *Sess);
EXPFUNC CoyoteStatusCode Coyote_TakePrev(struct CoyoteSession *Sess);
EXPFUNC CoyoteStatusCode Coyote_TakeNext(struct CoyoteSession *Sess);
EXPFUNC CoyoteStatusCode Coyote_RebootCoyote(struct CoyoteSession *Sess);
EXPFUNC CoyoteStatusCode Coyote_SoftRebootCoyote(struct CoyoteSession *Sess);
EXPFUNC CoyoteStatusCode Coyote_ShutdownCoyote(struct CoyoteSession *Sess);
EXPFUNC CoyoteStatusCode Coyote_SeekTo(struct CoyoteSession *Sess, const int32_t PK, const uint32_t TimeIndex);
EXPFUNC CoyoteStatusCode Coyote_DetectUpdate(struct CoyoteSession *Sess,  bool *IsDetected, char *OutBuf, const size_t OutBufCapacity);
EXPFUNC CoyoteStatusCode Coyote_GetTimeCode(struct CoyoteSession *Sess, const int32_t PK, struct Coyote_TimeCode *OutTC);
EXPFUNC CoyoteStatusCode Coyote_SelectPreset(struct CoyoteSession *Sess, const int32_t PK);
EXPFUNC CoyoteStatusCode Coyote_SetHardwareMode(struct CoyoteSession *Sess, const CoyoteResolutionMode Resolution, const CoyoteRefreshMode RefreshRate);
EXPFUNC CoyoteStatusCode Coyote_InitializeCoyote(struct CoyoteSession *Sess, const CoyoteResolutionMode Resolution, const CoyoteRefreshMode RefreshRate);
#endif //__LIBCOYOTE_SESSION_C_H__

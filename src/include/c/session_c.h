#ifndef __LIBCOYOTE_SESSION_C_H__
#define __LIBCOYOTE_SESSION_C_H__
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "../macros.h"
#include "../datastructures_c.h"
#include "../statuscodes.h"


void CoyoteSession_Destroy(struct CoyoteSession *Sess);
struct CoyoteSession *CoyoteSession_New(const char *Host);

void CoyoteObject_Destroy(void *Object);

CoyoteStatusCode Coyote_Take(struct CoyoteSession *Sess, const int32_t PK);
CoyoteStatusCode Coyote_Pause(struct CoyoteSession *Sess, const int32_t PK);
CoyoteStatusCode Coyote_End(struct CoyoteSession *Sess, const int32_t PK);
CoyoteStatusCode Coyote_SelectPrev(struct CoyoteSession *Sess);
CoyoteStatusCode Coyote_SelectNext(struct CoyoteSession *Sess);
CoyoteStatusCode Coyote_TakePrev(struct CoyoteSession *Sess);
CoyoteStatusCode Coyote_TakeNext(struct CoyoteSession *Sess);
CoyoteStatusCode Coyote_RebootCoyote(struct CoyoteSession *Sess);
CoyoteStatusCode Coyote_SoftRebootCoyote(struct CoyoteSession *Sess);
CoyoteStatusCode Coyote_ShutdownCoyote(struct CoyoteSession *Sess);
CoyoteStatusCode Coyote_SeekTo(struct CoyoteSession *Sess, const int32_t PK, const uint32_t TimeIndex);
CoyoteStatusCode Coyote_DetectUpdate(struct CoyoteSession *Sess,  bool *IsDetected, char *OutBuf, const size_t OutBufCapacity);
CoyoteStatusCode Coyote_GetTimeCode(struct CoyoteSession *Sess, const int32_t PK, struct Coyote_TimeCode *OutTC);
CoyoteStatusCode Coyote_SelectPreset(struct CoyoteSession *Sess, const int32_t PK);
CoyoteStatusCode Coyote_SetHardwareMode(struct CoyoteSession *Sess, const CoyoteResolutionMode Resolution, const CoyoteRefreshMode RefreshRate);
CoyoteStatusCode Coyote_InitializeCoyote(struct CoyoteSession *Sess, const CoyoteResolutionMode Resolution, const CoyoteRefreshMode RefreshRate);
#endif //__LIBCOYOTE_SESSION_C_H__

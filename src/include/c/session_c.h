#ifndef __LIBCOYOTE_SESSION_C_H__
#define __LIBCOYOTE_SESSION_C_H__
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "../macros.h"
#include "../datastructures_c.h"
#include "../statuscodes.h"


CoyoteSession *CoyoteSession_New(const char *Host);

CoyoteStatusCode Coyote_Take(CoyoteSession *Sess, const int32_t PK);
CoyoteStatusCode Coyote_Pause(CoyoteSession *Sess, const int32_t PK);
CoyoteStatusCode Coyote_End(CoyoteSession *Sess, const int32_t PK);
CoyoteStatusCode Coyote_TakePrev(CoyoteSession *Sess);
CoyoteStatusCode Coyote_TakeNext(CoyoteSession *Sess);
CoyoteStatusCode Coyote_RebootCoyote(CoyoteSession *Sess);
CoyoteStatusCode Coyote_SoftRebootCoyote(CoyoteSession *Sess);
CoyoteStatusCode Coyote_ShutdownCoyote(CoyoteSession *Sess);
CoyoteStatusCode Coyote_SeekTo(CoyoteSession *Sess, const int32_t PK, const uint32_t TimeIndex);
CoyoteStatusCode Coyote_DetectUpdate(CoyoteSession *Sess,  bool *IsDetected, char *OutBuf, const size_t OutBufCapacity);

#endif //__LIBCOYOTE_SESSION_C_H__

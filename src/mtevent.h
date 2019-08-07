/*
   Copyright 2019 Sonoran Video Systems

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
#ifndef __LIBCOYOTE_MTEVENT_H__
#define __LIBCOYOTE_MTEVENT_H__

#include "common.h"
#include <thread>
#include <mutex>
#include <semaphore.h>
#include <pthread.h>
#include <atomic>

template <typename T = void*>
class MTEvent
{
private:
	mutable std::mutex Mutex, DropDeadMutex;
	T Lump;
	sem_t Semaphore;
	std::atomic<sem_t*> DropDeadPtr;
	
public:
	MTEvent(const T *InitialValue = nullptr) : Lump(InitialValue ? std::move(*InitialValue) : T()), Semaphore(), DropDeadPtr()
	{
		sem_init(&this->Semaphore, 0, 0);
	}

	~MTEvent(void)
	{
		this->TriggerDeath();
		sem_destroy(&this->Semaphore);
	}
	
	inline void TriggerDeath(void)
	{
		const std::lock_guard<std::mutex> G2 { this->DropDeadMutex };

		if (this->DropDeadPtr) sem_post(this->DropDeadPtr);
	}
	
	bool Wait(T &ValueOut, const time_t Timeout = 5) //Return by value!
	{
		bool Success = false;
		
		std::unique_ptr<sem_t> DropDeadSemaphore { new sem_t{} };

		sem_init(DropDeadSemaphore.get(), 0, 0);
		
		
		std::unique_lock<std::mutex> G2 { this->DropDeadMutex };
		
		this->DropDeadPtr = DropDeadSemaphore.get();
		
		G2.unlock();
		
		for (uint32_t Inc = 0; Inc < Timeout * 1000 && !Success && sem_trywait(DropDeadSemaphore.get()) != 0; ++Inc)
		{
			Success = sem_trywait(&this->Semaphore) == 0;
			
			if (!Success) COYOTE_SLEEP(1);
		}

		G2.lock();
		this->DropDeadPtr = nullptr; //So we aren't holding onto it after we're done with the ticket, like we are.
		sem_destroy(DropDeadSemaphore.get());
		
		G2.unlock();
		
		if (!Success)
		{ //We timed out.
			return false;
		}
		
		std::unique_lock<std::mutex> Lock { this->Mutex };
		
		ValueOut = std::move(this->Lump);
		
		this->Lump = {};
		
		return true;
	}
	
	void Post(const T &Value = {})
	{
		const std::lock_guard<std::mutex> Lock { this->Mutex };
	
		this->Lump = Value;
		
		sem_post(&this->Semaphore);
	}
	
	void Post(T &&Value = {})
	{
		const std::lock_guard<std::mutex> Lock { this->Mutex };
	
		this->Lump = Value;
		
		sem_post(&this->Semaphore);
	}
	
	const T *Peek(void) const
	{
		const std::lock_guard<std::mutex> Lock { this->Mutex };
		
		return &this->Lump;
	}
	
	const T *UnsafePeek(void) const
	{
		return &this->Lump;
	}
};

#endif //__LIBCOYOTE_MTEVENT_H__

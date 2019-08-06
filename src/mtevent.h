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

template <typename T = void*>
class MTEvent
{
private:
	mutable std::mutex Mutex;
	T Lump;
	sem_t Semaphore;
public:
	MTEvent(const T *InitialValue = nullptr) : Lump(InitialValue ? std::move(*InitialValue) : T()), Semaphore()
	{
		sem_init(&this->Semaphore, 0, 0);
	}

	~MTEvent(void)
	{
		sem_destroy(&this->Semaphore);
	}
	
	bool Wait(T &ValueOut, const time_t Timeout = 1) //Return by value!
	{
		bool Success = false;
		
		for (uint32_t Inc = 0; Inc < Timeout * 1000 && !Success; ++Inc)
		{
			Success = sem_trywait(&this->Semaphore) == 0;
			
			COYOTE_SLEEP(Timeout);
		}
			
		
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

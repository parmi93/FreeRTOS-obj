/****************************************************************************
 *
 * Copyright (c) 2020, Parminder Singh (parminder.sh93@gmail.com)
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 ****************************************************************************/

#ifndef QUEUE_HPP_
#define QUEUE_HPP_

#include "FreeRTOS.h"
#include "queue.h"
#include <type_traits>

namespace freertos_obj
{
	using namespace std;
	
	template <class T>
	class Queue
	{
	public:
		Queue(const size_t size)
			: freertos_push_queue(xQueueCreate(size, sizeof(bool)))
			, freertos_pop_queue(xQueueCreate(size, sizeof(bool)))
			, array_queue(new T[size])
			, size(size)
			, in(0)
			, out(0)
		{}
		
		//Class not copyable because FreeRTOS queue are not copyable
		Queue(const Queue&) = delete;
		Queue& operator=(const Queue&) = delete;
		
		bool Push(const T& item, TickType_t wait = 0)
		{
			bool flag;
			
			bool success = xQueueSendToBack(freertos_push_queue, &flag, wait) == pdPASS;
			
			if (success == true)
			{
				taskENTER_CRITICAL();
				{
					PushFromCriticalSection(item);
				}
				taskEXIT_CRITICAL();
				
				xQueueSendToBack(freertos_pop_queue, &flag, 0);
			}
			
			return success;
		}
		
		bool Push(T&& item, TickType_t wait = 0)
		{
			bool flag;
			
			bool success = xQueueSendToBack(freertos_push_queue, &flag, wait) == pdPASS;
			
			if (success == true)
			{
				taskENTER_CRITICAL();
				{
					PushFromCriticalSection(move(item));
				}
				taskEXIT_CRITICAL();
				
				xQueueSendToBack(freertos_pop_queue, &flag, 0);
			}
			
			return success;
		}
		
		bool Push_FromISR(const T& item)
		{
			bool flag;
			
			bool success = xQueueSendToBackFromISR(freertos_push_queue, &flag, NULL) == pdPASS;
			
			if (success == true)
			{
				UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
				{
					PushFromCriticalSection(item);
				}
				taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
				
				xQueueSendToBackFromISR(freertos_pop_queue, &flag, NULL);
			}
			
			return success;
		}
		
		bool Push_FromISR(T&& item)
		{
			bool flag;
			
			bool success = xQueueSendToBackFromISR(freertos_push_queue, &flag, NULL) == pdPASS;
			
			if (success == true)
			{
				UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
				{
					PushFromCriticalSection(move(item));
				}
				taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
				
				xQueueSendToBackFromISR(freertos_pop_queue, &flag, NULL);
			}
			
			return success;
		}
		
		bool Pop(T& item, TickType_t wait = 0)
		{
			bool success;
			
			bool flag;
			
			success = xQueueReceive(freertos_pop_queue, &flag, wait) == pdPASS;
			
			if (success == true)
			{
				taskENTER_CRITICAL();
				{
					PopFromCriticalSection(item);
				}
				taskEXIT_CRITICAL();
				
				xQueueReceive(freertos_push_queue, &flag, 0);
			}
			
			return success;
		}
		
		size_t ItemWaiting() const
		{
			return uxQueueMessagesWaiting(freertos_pop_queue);
		}
		
		size_t SpacesAvailable() const
		{
			return uxQueueSpacesAvailable(freertos_push_queue);
		}
		
		~Queue()
		{
			vQueueDelete(freertos_push_queue);
			
			vQueueDelete(freertos_pop_queue);
			
			delete[] array_queue;
		}
		
	private:
		QueueHandle_t freertos_push_queue;
		
		QueueHandle_t freertos_pop_queue;
		
		size_t size;
		
		size_t in;
		
		size_t out;
		
		T* array_queue;
		
		void PushFromCriticalSection(const T& item)
		{
			array_queue[in++] = item;
			
			if (in >= size)
			{
				in = 0;
			}
		}
		
		void PushFromCriticalSection(T&& item)
		{
			array_queue[in++] = move(item);
			
			if (in >= size)
			{
				in = 0;
			}
		}
		
		void PopFromCriticalSection(T& item)
		{
			if (is_move_assignable<T>::value == true)
			{
				item = move(array_queue[out]);
			}
			else
			{
				item = array_queue[out];
			}
			
			array_queue[out] = T();
				
			if (++out >= size)
			{
				out = 0;
			}
		}
	};
}

#endif

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

#ifndef TASK_HPP_
#define TASK_HPP_

#include "FreeRTOS.h"
#include "task.h"
#include <string>
#include <atomic>
#include <functional>
#include <type_traits>
#include <memory>

namespace freertos_obj
{
	using namespace std;
	
	class Runnable
	{
	public:
		virtual ~Runnable()
		{}
		
		virtual void Suspend() 
		{}
		
		virtual void Resume()
		{}
		
		#if INCLUDE_vTaskDelay == 1
		virtual void Delay(const TickType_t xTicksToDelay)
		{}
		#endif
		
		virtual bool AbortDelay()
		{
			return false;
		}
		
		#if INCLUDE_uxTaskPriorityGet == 1
		virtual UBaseType_t GetPriority()
		{
			return 0;
		}
		#endif
		
		#if INCLUDE_xTaskGetCurrentTaskHandle == 1 || configUSE_MUTEXES == 1
		virtual bool IsCurrentTask()
		{
			return false;
		}
		#endif
		
		virtual bool DeletingInProgress() 
		{
			return true;
		}
		
	protected:
		virtual bool Run()
		{
			return true;
		}
	};
	
	class TaskFreeRtos : public virtual Runnable
	{
	public:
		TaskFreeRtos(const string name, 
			const configSTACK_DEPTH_TYPE usStackDepth, 
			const UBaseType_t uxPriority)
			: request_to_delete_task(false)
			, task_terminated(false)
		{
			xTaskCreate(RunTask, name.c_str(), usStackDepth, this, uxPriority, &task_handle);
		}
		
		//Class not copyable because FreeRTOS tasks are not copyable
		TaskFreeRtos(const TaskFreeRtos&) = delete;
		TaskFreeRtos& operator=(const TaskFreeRtos&) = delete;
		
		virtual ~TaskFreeRtos()
		{
			ExitFromTask();
		}
		
		static decltype(vTaskStartScheduler()) StartScheduler()
		{
			scheduler_is_running = true;
			
			return vTaskStartScheduler();
		}
		
		static decltype(vTaskSuspendAll()) SuspendAll()
		{
			return vTaskSuspendAll();
		}
		
		static decltype(xTaskResumeAll()) ResumeAll()
		{
			return xTaskResumeAll();
		}
		
		#if INCLUDE_uxTaskPriorityGet == 1
		static decltype(uxTaskPriorityGet(NULL)) GetPriorityCurrentTask()
		{
			return uxTaskPriorityGet(NULL);
		}
		#endif
		
		static decltype(taskENTER_CRITICAL()) EnterCritical()
		{
			return taskENTER_CRITICAL();
		}
		
		static decltype(taskEXIT_CRITICAL()) ExitCritical()
		{
			taskEXIT_CRITICAL();
		}
		
		void Suspend() override
		{
			return vTaskSuspend(task_handle);
		}
		
		void Resume() override
		{
			return vTaskResume(task_handle);
		}
		
		#if INCLUDE_vTaskDelay == 1
		void Delay(const TickType_t xTicksToDelay) override
		{
			vTaskDelay(xTicksToDelay);
		}
		#endif
		
		bool AbortDelay() override
		{
			return xTaskAbortDelay(task_handle) == pdPASS;
		}
		
		#if INCLUDE_uxTaskPriorityGet == 1
		UBaseType_t GetPriority() override
		{
			return uxTaskPriorityGet(task_handle);
		}
		#endif
		
		#if INCLUDE_xTaskGetCurrentTaskHandle == 1 || configUSE_MUTEXES == 1
		bool IsCurrentTask() override
		{
			return xTaskGetCurrentTaskHandle() == task_handle;
		}
		#endif
		
		void ExitFromTask()
		{
			if (task_terminated == true)
			{
				return;
			}
			
			if (scheduler_is_running == false)
			{
				#if INCLUDE_vTaskDelete == 1
				vTaskDelete(task_handle);
				#endif
				return;
			}
			
			request_to_delete_task = true;
			
			do
			{
				//To release the task from the Suspended state
				Resume();
		 		
				//To release the task from the Blocked state
				AbortDelay();
				
				if (GetPriority() <= GetPriorityCurrentTask())
				{
					Delay(pdMS_TO_TICKS(10));
				}
			} while (task_terminated == false);
		}
		
		bool DeletingInProgress() override
		{
			return request_to_delete_task;
		}
		
	private:
		TaskHandle_t task_handle;
		
		atomic_bool task_terminated;
		
		atomic_bool request_to_delete_task;
		
		inline static atomic_bool scheduler_is_running = false;
			
		static void RunTask(void* param)
		{
			{
				TaskFreeRtos* _this = static_cast<TaskFreeRtos*>(param);
				
				while (1)
				{
					if (_this->Run() == true)
					{
						_this->request_to_delete_task = true;
						
						break;
					}
					else if (_this->request_to_delete_task == true)
					{
						break;
					}
				}
				
				_this->task_terminated = true;
			}
			#if INCLUDE_vTaskDelete == 1
			vTaskDelete(NULL);
			#endif
		}
	};
	
	template<class RUNNABLE = Runnable>
	class Task : public virtual RUNNABLE, public TaskFreeRtos
	{
	public:
		static_assert(is_base_of<Runnable, RUNNABLE>::value, "RUNNABLE must derive from Runnable class");
		
		template<class ...Args>
		Task(const string name, 
			const configSTACK_DEPTH_TYPE usStackDepth, 
			UBaseType_t uxPriority, 
			Args&&... args)
			: TaskFreeRtos(name, usStackDepth, uxPriority)
			, RUNNABLE(forward<Args>(args)...)
		{}
	};
	
	typedef function<bool()> TaskCallback;
	
	template<>
	class Task<TaskCallback> : public TaskFreeRtos
	{
	public:
		Task(const string name, 
			const configSTACK_DEPTH_TYPE usStackDepth, 
			UBaseType_t uxPriority, 
			TaskCallback task_callback)
			: task_callback(task_callback)
			, TaskFreeRtos(name, usStackDepth, uxPriority)
		{}
		
		virtual ~Task()
		{}
		
	protected:
		bool Run() override
		{
			return this->task_callback();
		}
		
	private:
		TaskCallback task_callback;
	};
}

#endif

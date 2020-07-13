# FreeRTOS-obj
FreeRTOS strictly object-oriented

When I started developing in C++ I felt the need to use a wrapper for FreeRTOS, a wrapper that would allow me to avoid using pointers to store objects in FreeRTOS queues.

I only developed some parts of this wrapper, the ones I needed for my job.

I will publish soon the wrapper for the "Queue Set" API.

# How to use
### How to create a task?
You can create a task in two different ways.

- With a lambda callback:
```cpp
using namespace freertos_obj;

Task<TaskCallback> myTask("task_name",    //Task name
  512,                                    //Stack Depth
  1,                                      //Priority
  (TaskCallback)[]() -> bool {            //Task callback
    //do something
    return false;   //return true to delete this task, otherwise return false to rerun the callback
});
```

- With a runnable class:
```cpp
using namespace freertos_obj;

//Your runnable class must derive from Runnable
class myRunnableClass : public virtual Runnable
{
public:
	myRunnableClass(int param1, std::string param2, float param3)
		: param1(param1)
		, param2(param2)
		, param3(param3)
	{ }
	
	//You must implement the Run() method
	bool Run()
	{
		return true;  //return true to delete this task, otherwise return false to run this method again
	}
  
	void myMethod()
	{ }
	
	int param1;
	std::string param2;
	float param3;
};


Task<myRunnableClass> myTask("task_name",   //Task name
  512,                                      //Stack Depth
  1,                                        //Priority
  99,                                       //param1	Params for the constructor of the myRunnableClass class
  "hello word",                             //param2
  3.2);                                     //param3

//Yes of course, you can call your method and variables in this way. myTask inherit from myRunnableClass
myTask.myMethod();
myTask.param1 = 100;

```
### How to use a queue?
```cpp
using namespace freertos_obj;

class myClass
{ };

auto queue_str = Queue<std::string>(10);  //size 10
auto queue_cls = Queue<myClass>(5);       //size 5

queue_str.Push("hello word", 0);
queue_cls.Push(myClass(), 0);
```
# A suggestion
When using objects, allocation functions (malloc, new, new[], delete, delete[]) are often called by the std libray. If you use these objects within FreeRTOS tasks this could be a problem, as a concurrent access to the allocation functions can occur and the memory pool may become corrupted.

So I advise you to consider adding the following hooks in your project:
```cpp
extern "C" void __malloc_lock(struct _reent *REENT)
{
	freertos_obj::Task<>::SuspendAll();
}

extern "C" void __malloc_unlock(struct _reent *REENT)
{
	freertos_obj::Task<>::ResumeAll();
}
```

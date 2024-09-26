#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <string>
#include <vector>
#include <functional>
using namespace std;

class ThreadPool
{
public:
	ThreadPool(int numThreads) :stop(false)
	{
		// 创建numThreads个线程
		for (int i = 0; i < numThreads; i++)
		{ 
			// emplace_back(arg) 调用thread的初始化 创建thread对象 并且关联lambda函数 {arg是构造所需要的参数} 
			threads.emplace_back([this]() {
				while (1)  // 线程执行完任务后 要重新回来拿任务
				{
					unique_lock<mutex> lock(mtx); // 创建unique_lock对象 获取mtx的所有权
					// 条件等待 第二个参数_Pred 1.重载了()的类或者结构体 2.函数指针 3.lambda表达式  总之就是可以调用()
					cv.wait(lock, [this]() {return !tasks.empty() || stop; }); // 如果任务停止 或者任务列表不为空 不等待；任务继续列表为空 等待 
					if (stop && tasks.empty()) // 如果任务停止(并且为空)  返回
					{
						return;
					}
					// 否则执行任务
					function<void()> task(move(tasks.front())); // 取任务  std::move() 的作用：
					//std::move() 是一个函数模板，通过将其参数强制转换为右值引用，允许我们实现移动语义。
					//这意味着资源（如内存、文件句柄等）的所有权可以从一个对象转移到另一个对象，而非复制。

					tasks.pop();  // 队列第一个元素出去  删除执行完的任务  队列：先进先出
					lock.unlock(); // 解锁 让其他线程可以取任务
					task();
				}	});
		}
	}
	~ThreadPool()
	{
		{
			unique_lock<mutex> lock(mtx);
			stop = true;
		}   // 这个地方一定要有{}  否则会在析构时 通知所有函数干活 但是由于函数拿不到stop 导致无法执行

		cv.notify_all(); // 通知所有函数把任务执行完 
		
		/* auto用于自动推导变量的类型
		* 1. auto x = 1;  auto y = "hellow"; ...
		* 2. auto add(int x, int y){return x+y;}
		* ...
		*/
		for (auto& t : threads)  //  基于范围(range-based)的 for 循环   &可以对t进行修改
		{
			t.join();
		}
	}

	// 放任务    因为参数个数不确定  使用模板参数包，允许模板接受可变数量的模板参数 
	// Args模版参数包   args函数参数包
	template<class F,class... Args>
	void enqueue(F&& f, Args&&... args)  // 模版里的右值引用是万能引用   这个时候里面边函数要配合完美转发forward来使用
	{
		//  { 包扩展   模式右边加上...}
		// 将传入的任务与参数进行绑定
		function<void()> task = bind(forward<F>(f), forward<Args>(args)...);  //bind 函数适配器 
		// 将任务放入任务队列  使用一个作用域  来保准unique_lock对象能够析构
		{
			// 操作共享数据  上锁
			unique_lock<mutex> lock(mtx);
			tasks.emplace(move(task));  
			/*emplace() 函数是在容器中构造新元素的一种方式。它接受参数并将其传递给元素类型的构造函数
			，避免了复制或移动构造函数的额外开销*/
		}
		// 通知干活
		cv.notify_one();
	}


private:
	// 创建线程数组
	vector<thread> threads;
	// 创建任务队列  队列里面是签名为void()的可调用对象
	queue<function<void()>> tasks;
	mutex mtx;
	condition_variable cv;
	bool stop;
};

int main()
{
	ThreadPool pool(4);
	for (int i = 0; i < 10; i++)
	{
		pool.enqueue([i]() {
			cout << "task" << i << "is run" << endl;
			this_thread::sleep_for(chrono::seconds(1));  // 暂停当前的线程执行一秒钟
			cout << "task" << i << "done" << endl;	});
	}

	return 0;
}
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
		// ����numThreads���߳�
		for (int i = 0; i < numThreads; i++)
		{ 
			// emplace_back(arg) ����thread�ĳ�ʼ�� ����thread���� ���ҹ���lambda���� {arg�ǹ�������Ҫ�Ĳ���} 
			threads.emplace_back([this]() {
				while (1)  // �߳�ִ��������� Ҫ���»���������
				{
					unique_lock<mutex> lock(mtx); // ����unique_lock���� ��ȡmtx������Ȩ
					// �����ȴ� �ڶ�������_Pred 1.������()������߽ṹ�� 2.����ָ�� 3.lambda���ʽ  ��֮���ǿ��Ե���()
					cv.wait(lock, [this]() {return !tasks.empty() || stop; }); // �������ֹͣ ���������б�Ϊ�� ���ȴ�����������б�Ϊ�� �ȴ� 
					if (stop && tasks.empty()) // �������ֹͣ(����Ϊ��)  ����
					{
						return;
					}
					// ����ִ������
					function<void()> task(move(tasks.front())); // ȡ����  std::move() �����ã�
					//std::move() ��һ������ģ�壬ͨ���������ǿ��ת��Ϊ��ֵ���ã���������ʵ���ƶ����塣
					//����ζ����Դ�����ڴ桢�ļ�����ȣ�������Ȩ���Դ�һ������ת�Ƶ���һ�����󣬶��Ǹ��ơ�

					tasks.pop();  // ���е�һ��Ԫ�س�ȥ  ɾ��ִ���������  ���У��Ƚ��ȳ�
					lock.unlock(); // ���� �������߳̿���ȡ����
					task();
				}	});
		}
	}
	~ThreadPool()
	{
		{
			unique_lock<mutex> lock(mtx);
			stop = true;
		}   // ����ط�һ��Ҫ��{}  �����������ʱ ֪ͨ���к����ɻ� �������ں����ò���stop �����޷�ִ��

		cv.notify_all(); // ֪ͨ���к���������ִ���� 
		
		/* auto�����Զ��Ƶ�����������
		* 1. auto x = 1;  auto y = "hellow"; ...
		* 2. auto add(int x, int y){return x+y;}
		* ...
		*/
		for (auto& t : threads)  //  ���ڷ�Χ(range-based)�� for ѭ��   &���Զ�t�����޸�
		{
			t.join();
		}
	}

	// ������    ��Ϊ����������ȷ��  ʹ��ģ�������������ģ����ܿɱ�������ģ����� 
	// Argsģ�������   args����������
	template<class F,class... Args>
	void enqueue(F&& f, Args&&... args)  // ģ�������ֵ��������������   ���ʱ������ߺ���Ҫ�������ת��forward��ʹ��
	{
		//  { ����չ   ģʽ�ұ߼���...}
		// �������������������а�
		function<void()> task = bind(forward<F>(f), forward<Args>(args)...);  //bind ���������� 
		// ����������������  ʹ��һ��������  ����׼unique_lock�����ܹ�����
		{
			// ������������  ����
			unique_lock<mutex> lock(mtx);
			tasks.emplace(move(task));  
			/*emplace() �������������й�����Ԫ�ص�һ�ַ�ʽ�������ܲ��������䴫�ݸ�Ԫ�����͵Ĺ��캯��
			�������˸��ƻ��ƶ����캯���Ķ��⿪��*/
		}
		// ֪ͨ�ɻ�
		cv.notify_one();
	}


private:
	// �����߳�����
	vector<thread> threads;
	// �����������  ����������ǩ��Ϊvoid()�Ŀɵ��ö���
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
			this_thread::sleep_for(chrono::seconds(1));  // ��ͣ��ǰ���߳�ִ��һ����
			cout << "task" << i << "done" << endl;	});
	}

	return 0;
}
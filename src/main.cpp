/**
 * Author: rfilippone@gmail.com
 */

#include <iostream>
#include <queue>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/date_time.hpp>

int accessDB(std::string query)
{
    std::cout << boost::this_thread::get_id() << ": Access DB " << query << std::endl;
    usleep(400*1000);

//    boost::this_thread::sleep(boost::posix_time::seconds(2));

    return 7;
}

void handle_result(int res)
{
    std::cout << boost::this_thread::get_id() << ": Risultato " << res << std::endl;
}

std::queue<boost::function<void()> > jobQueue;

boost::condition_variable cond;
boost::mutex mut;
bool work_item_ready = false;

void process_queue()
{
    while (true)
    {
        boost::unique_lock<boost::mutex> lock(mut);
        while(!work_item_ready)
        {
            cond.wait(lock);
        }
        if (jobQueue.size() > 0)
        {
            boost::function<void()> func = jobQueue.front();
            jobQueue.pop();
            lock.unlock();
            func();
            lock.lock();
        }
        if (jobQueue.size() == 0)
        {
            work_item_ready = false;
        }
    }
}


void push_to_queue(boost::function<void()> func)
{
    std::cout << "main: pushing job on the queue " << jobQueue .size() << std::endl;
    jobQueue.push(func);
    {
        boost::lock_guard<boost::mutex> lock(mut);
        work_item_ready=true;
    }
    cond.notify_one();
}

template <typename RET>
struct WorkItem
{
    boost::function<RET()> f;
    boost::function<void(RET)> cb;

    void run()
    {
        // callback is called in the same thread
        // cb(f());
        // callback is pushed to the queue
        push_to_queue(boost::bind(cb, f()));
    }
};

template <typename RET> void queue(boost::function<RET()> f, boost::function<void(RET)> cb)
{
    WorkItem<RET> item;
    item.f = f;
    item.cb = cb;

    push_to_queue(boost::bind(&WorkItem<RET>::run, item));
}


#define RUN_ASYNC(TYPE, FUNC, P1, CB) queue<TYPE>(boost::bind(FUNC, P1), boost::bind(CB, _1))

int main(int argc, char **argv)
{
    boost::thread t1(process_queue);
    boost::thread t2(process_queue);
    boost::thread t3(process_queue);
    boost::thread t4(process_queue);
    boost::thread t5(process_queue);

//    queue<int>(boost::bind(accessDB, "select * from table"), boost::bind(handle_result, _1));
//    RUN_ASYNC(int, accessDB, "select 2", handle_result);

    for (int var = 0; var < 25; ++var) {
        RUN_ASYNC(int, accessDB, "select loop " + boost::lexical_cast<std::string>(var), handle_result);
        usleep(19*1000);
    }

    t1.join();
    t2.join();
    t2.join();
    t3.join();
    t4.join();

    //return entryPoint();
}


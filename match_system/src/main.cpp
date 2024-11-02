// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "match_server/Match.h"
#include "save_client/Save.h"

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TSocket.h>


#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using namespace ::match_service;
using namespace ::save_service;
using namespace std;





struct Task {
    User user;
    string type;

};

struct MessageQueue{
    queue<Task> q;
    mutex m;
    condition_variable cv;
}message_queue;

class Pool{
    public:
        void add(User user) {
            users.push_back(user);
        }

        void remove(User user) {
            for (uint32_t i = 0; i < users.size(); i++) {
                if(users[i].id == user.id) {
                    users.erase(users.begin()+i);
                    break;
                }
            }
        }

        void save_result(int a, int b) {
            printf("Match Result: %d %d\n", a, b);

            std::shared_ptr<TTransport> socket(new TSocket("123.57.47.211", 9090));
            std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            SaveClient client(protocol);

            try {
                transport->open();
                int res = client.save_data("acs_7192", "536ed098", a, b);
                if(!res) cout << "success" << endl;
                else cout << "failed" << endl;

                transport->close();
            } catch (TException& tx) {
                cout << "ERROR: " << tx.what() << '\n';
            }

        }

        void match() {
            while(users.size() > 1) {
                auto a = users[0], b = users[1];
                users.erase(users.begin());
                users.erase(users.begin());
                save_result(a.id, b.id);
            }
        }

    private:
        vector<User> users;

}pool;

class MatchHandler : virtual public MatchIf {
    public:
        MatchHandler() {
            // Your initialization goes here
        }

        /**
         * user: 添加的用户信息
         * info: 附加信息
         * 在匹配池中添加一个名用户
         * 
         * @param user
         * @param info
         */
        int32_t add_user(const User& user, const std::string& info) {
            // Your implementation goes here
            printf("add_user\n");
            // 锁上
            unique_lock<mutex> lck(message_queue.m);  // 不需要显示解锁，函数运行完自动解锁
            message_queue.q.push({user, "add"});
            message_queue.cv.notify_all();    // 唤醒所有被条件变量卡住的线程
            return 0;
        }

        /**
         * user: 删除的用户信息
         * info: 附加信息
         * 从匹配池中删除一名用户
         * 
         * @param user
         * @param info
         */
        int32_t remove_user(const User& user, const std::string& info) {
            // Your implementation goes here
            printf("remove_user\n");
            unique_lock<mutex> lck(message_queue.m);  // 不需要显示解锁，函数运行完自动解锁
            message_queue.q.push({user, "remove"});
            message_queue.cv.notify_all();    // 唤醒所有被条件变量卡住的线程
            return 0;
        }

};

// 如果两个线程同时想向队列q里存东西，可能存到同一个地方就会冲突
// 用锁解决这个问题
void consume_task() {
    while(true)
    {
        unique_lock<mutex> lck(message_queue.m);
        if(message_queue.q.empty()) {
            message_queue.cv.wait(lck);   // 游戏刚开始队列一直为空，等待唤醒；
        }
        else {
            auto task = message_queue.q.front();
            message_queue.q.pop();
            lck.unlock();  // 做具体任务之前就解锁，别浪费资源

            if (task.type == "add") pool.add(task.user);
            else if(task.type == "remove") pool.remove(task.user);

            pool.match();
        }
    }




}


int main(int argc, char **argv) {
    int port = 9090;
    ::std::shared_ptr<MatchHandler> handler(new MatchHandler());
    ::std::shared_ptr<TProcessor> processor(new MatchProcessor(handler));
    ::std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    ::std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    ::std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);

    std::cout << "Start Match Server" << std::endl;

    // 使用多线程
    thread matching_thread(consume_task);








    server.serve();
    return 0;
}


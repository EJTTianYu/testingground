#include <concepts>
#include <coroutine>
#include <exception>
#include <iostream>
#include <thread>
#include <chrono>  
#include <typeinfo>

class People {
public:
  People(int age): age(age) {};
public:
  int age;
};

class Student: public People {
public:
  Student(int age1, int no) : People(age1), no(no) {};
public:
  int no;
};

void run(std::coroutine_handle<> h)
{
  std::cout<<std::this_thread::get_id()<<" "<<"in Run\n";
  std::this_thread::sleep_for (std::chrono::seconds(5));
  h.resume();
}

struct return_type {

    struct promise_type {

        promise_type() {
            std::cout<<std::this_thread::get_id() <<" Promise created "<<(void*)this<< std::endl;
        }

        ~promise_type() {
            std::cout<<std::this_thread::get_id() << " Promise "<<this<<" died" << std::endl;
            auto hh = std::coroutine_handle<promise_type>::from_promise(*this);
            std::cout<<std::this_thread::get_id() << " handle:"<<hh.address()<<" done:"<<hh.done()<<std::endl;
        }

        auto get_return_object() {
          auto h = std::coroutine_handle<promise_type>::from_promise(*this);
          std::cout << "Send back a return_type with handle:"<<h.address()<<std::endl;
          return return_type(h);
        }

        auto initial_suspend() {
            std::cout<<std::this_thread::get_id() <<" Started the coroutine, don't stop now!" << std::endl;
            return std::suspend_never{};
        }

        auto final_suspend() noexcept {
            std::cout<<std::this_thread::get_id() << " this promise pointer in final suspend:"<< (void*)this<<" prev:"<<prev_<<std::endl;
            if (prev_ != nullptr) {
              auto hh = std::coroutine_handle<promise_type>::from_promise(*prev_);
              hh.resume();
            }

            return std::suspend_never{};
        }
        void unhandled_exception() {
            std::exit(1);
        }

        double return_value(double a) {
          dNo = a;
        }

        int return_value(int a) {
            no = a;
        }

        void return_value(People p) {
          std::cout << "return people"<<std::endl;
        }

        void return_value(Student p) {
          std::cout << "return student"<<std::endl;
        }

        int no = 0;
        double dNo = 0.0;
        promise_type* prev_ = nullptr;
    };
    
    return_type(bool async) : async_(async) {}

    return_type(std::coroutine_handle<promise_type> h) : h_{h} {
        std::cout << "Created a return_type object"<<std::endl;
    }

    ~return_type() {
        std::cout << "return_type gone" << std::endl;
    }

    constexpr bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<promise_type> h) {
      std::cout<<std::this_thread::get_id()<<" "<<"await_suspend, async:"<<async_<<"\n";
      std::cout<<std::this_thread::get_id() << " this promise pointer:"<< (void*)(&h_.promise())<<" handle:"<<h_.address()<< std::endl;
      std::cout<<std::this_thread::get_id() << " parameter promise pointer:"<<(void*)(&h.promise()) <<" handle:"<<h.address()<< std::endl;
      if (!async_) {
        h_.promise().prev_ = &h.promise();
      }

      if (async_) {
        std::thread t([&promise = h.promise()](){
          std::cout<<std::this_thread::get_id()<<" "<<"in Run\n";
          std::this_thread::sleep_for (std::chrono::seconds(5));
          auto hh = std::coroutine_handle<promise_type>::from_promise(promise);
          std::cout<<std::this_thread::get_id()<<" "<<"ready to resume, handle:"<<hh.address()<<"\n";
          hh.resume();
         });
        t.detach();
      }
    }

    void await_resume() const noexcept { 
      std::cout<<std::this_thread::get_id()<<" "<<"await_resume\n"; 
    }

    int get_result() {
        return h_.promise().no;
    }

    double get_dresult() {
      return h_.promise().dNo;
    }

    std::coroutine_handle<promise_type> h_;
    bool async_ = false;
};

return_type Foo3()
{
  std::cout<<std::this_thread::get_id()<<" "<<"enter Foo3\n";
  return_type r(true);
  co_await r;
  std::cout<<std::this_thread::get_id()<<" "<<"resume in Foo3\n";
  People people{1};
  co_return people;
}

return_type Foo2()
{ 
  std::cout<<std::this_thread::get_id()<<" "<<"enter Foo2\n";
  auto result = Foo3();
  co_await result;
  std::cout<<std::this_thread::get_id()<<" "<<"resume in Foo2\n";
  std::cout<<result.get_dresult()<<"返回值, func Foo2"<<std::endl;
  co_return 1;
}

return_type Foo1()
{
  std::cout<<std::this_thread::get_id()<<" "<<"enter Foo1\n";
  auto result = Foo2();
  co_await result;
  std::cout<<std::this_thread::get_id()<<" resume in Foo1\n";
  std::cout<<result.get_result()<<"返回值, func Foo1"<<std::endl;
}

int main() {
  auto r = Foo1();
  std::cout<<std::this_thread::get_id()<<" main thread wait\n";;
  std::this_thread::sleep_for (std::chrono::seconds(10));
}
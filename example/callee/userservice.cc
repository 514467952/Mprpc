#include<iostream>
#include<string>
#include"../user.pb.h"
#include"../../src/include/mprpcapplication.h"
#include"../../src/include/rpcprovider.h"
/*
 * UserService原来是一个本地服务，提供两个进程内的本地方法，Login和GetFriendLists
 */

//使用在rpc服务的发布端(rpc服务的提供者)
class UserService : public fixbug::USerServiceRpc 
{
  public:
    bool Login(std::string name,std::string pwd)
    {
      std::cout<<"doing local service:Login"<<std::endl;
      std::cout<<"name:"<<name<<"pwd:"<<pwd<<std::endl;
      return true;
    }

    bool Register(uint32_t id,std::string name,std::string pwd)
    {
      std::cout<<"doing local service:Login"<<std::endl;
      std::cout<<"id"<<id<<"name:"<<name<<"pwd:"<<pwd<<std::endl;
      return true;
    }
    /*
    //重写基类UserServiceRpc的虚函数,下面这些方法都是框架直接调用的
    1. caller => Login(LoginRequest) =>muduo =>callee
    2. callee => Login(LoginRequest) =>交到下面重写的这个Login方法上
    */  
    void Login(::google::protobuf::RpcController* controller, 
        const ::fixbug::LoginRequest* request,    
        ::fixbug::LoginRsponse* response,    
        ::google::protobuf::Closure* done)
    {
      //框架给业务上报了请求参数LoginRequest,应用获取相应数据给本地业务
      std::string name = request->name();
      std::string pwd = request->pwd();

      //做本地业务
      bool login_result = Login(name,pwd);

      //把响应写入
      fixbug::ResultCode *code = response->mutable_result();
      code->set_errmsg("Login error");
      code->set_errcode(1);
      response->set_success(login_result);

      //执行回调操作 执行响应对象数据的序列化和网络发送(都是由框架来完成的)
      done->Run();
    }

    void Register(::google::protobuf::RpcController* controller, 
        const ::fixbug::RegisterRequest* request,    
        ::fixbug::RegisterResponse* response,    
        ::google::protobuf::Closure* done)
    {
      uint32_t id = request->id();
      std::string name = request->name();
      std::string pwd = request->pwd();

      bool ret = Register(id,name,pwd);
      response->mutable_result()->set_errcode(0);
      response->mutable_result()->set_errmsg("");
      response->set_success(ret);
      done->Run();
    }

};

int main(int argc,char**argv)
{
  //调用框架的初始化操作  provider -i config.conf
  MprpcApplication::Init(argc,argv);
  // 把UserSerVice对象发布到rpc结点上
  RpcProvider provider;
  provider.NotifyService(new UserService());
  //启动一个rpc服务发布结点
  provider.Run();
  return 0;
}

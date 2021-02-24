#include<iostream>
#include"../../src/include/mprpcapplication.h"
#include"../user.pb.h"
#include"../../src/include/mprpcchannel.h"

int main(int argc,char**argv)
{
  //整个程序启动后，想使用mprpc框架来享受服务调用，一定要先调用框架的初始化函数(只初始化一次)
  MprpcApplication::Init(argc,argv);

  //演示调用远程发布的rpc方法Login
  fixbug::USerServiceRpc_Stub stub(new MprpcChannel());
  fixbug::LoginRequest request;
  request.set_name("zhang san");
  request.set_pwd("123456");

  fixbug::LoginRsponse response;
  //发起rpc方法的调用，同步的rpc调用过程，MprpcChannel::callmenthod
  stub.Login(nullptr,&request,&response,nullptr); // RpcChannel->RpcChannel::callMethod 集中来做所有rpc方法调用参数序列化和网络发送
  
  //一次rpc调用完成，读调用的结果
  if(0 == response.result().errcode())
  {
    std::cout<<"rpc login response:"<<response.success()<<std::endl;
  }
  else 
  {
    std::cout<<"rpc login response error:"<<response.result().errmsg()<<std::endl;
  }

  fixbug::RegisterRequest req;
  req.set_id(2000);
  req.set_name("mprpc");
  req.set_pwd("66666");
  fixbug::RegisterResponse rsp;
  //以同步的方式发送rpc调用请求，等待返回结果
  stub.Register(nullptr,&req,&rsp,nullptr);
  
  if(0 == rsp.result().errcode())
  {
    std::cout<<"rpc Register response:"<<response.success()<<std::endl;
  }
  else 
  {
    std::cout<<"rpc Register response error:"<<response.result().errmsg()<<std::endl;
  }

  return 0;
}

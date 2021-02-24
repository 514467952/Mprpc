#pragma  once 
#include <google/protobuf/service.h>

class MprpcChannel : public google::protobuf::RpcChannel
{
public:
  //所有通过stub代理对象调用的rpc方法都走到这里了，统一做rpc方法的数据序列化和网络发送
  
  void CallMethod(const google::protobuf::MethodDescriptor*method, 
                        google::protobuf::RpcController* controller,
                        const google::protobuf::Message* request,
                        google::protobuf::Message* reponse, 
                        google::protobuf::Closure* done);

};

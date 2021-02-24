#include<iostream>
#include<string>
#include<vector>

#include "../friend.pb.h"
#include "../../src/include/mprpcapplication.h"
#include "../../src/include/rpcprovider.h"
#include "../../src/include/logger.h"

class FriendService :public fixbug::FriendServiceRpc 
{
public:
  std::vector<std::string> GetFriendsList(uint32_t userid)
  {
    std::cout<<"do GetFriendsList service"<<std::endl;
    std::vector<std::string> vec;
    vec.push_back("gao yang");
    vec.push_back("liu hong");
    vec.push_back("wang shuo");
    return vec;
  }

  void GetFriendsList(::google::protobuf::RpcController* controller,
                      const ::fixbug::GetFriendsListRequest* request,
                      ::fixbug::GetFriendsListResponse* response,
                      ::google::protobuf::Closure* done)
  {
    uint32_t userid = request->userid();

    std::vector<std::string> friendsList = GetFriendsList(userid);
    response->mutable_result()->set_errcode(0);
    response->mutable_result()->set_errmsg("");
    for(std::string &name : friendsList)
    {
      std::string *p = response->add_friends();
      *p = name;
    }
    done->Run();
  }
};

int main(int argc,char**argv)
{
  LOG_INFO("first log message!");
  LOG_ERR("%s:%s:%d",__FILE__,__FUNCTION__,__LINE__);
  MprpcApplication::Init(argc,argv);

  RpcProvider provider;
  provider.NotifyService(new FriendService());

  provider.Run();
  return 0;
}

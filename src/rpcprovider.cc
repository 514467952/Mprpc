#pragma once 
#include"include/rpcprovider.h"
#include"include/mprpcapplication.h"
#include"include/rpcheader.pb.h"
#include"include/logger.h"
#include"include/zookeeperutil.h"
/*
 * service_name =>service描述
 * service描述  => service* 记录服务对象
 * service*里有很多的方法，可以通过menthod方法得到
 */ 
void RpcProvider::NotifyService(google::protobuf::Service* service)
{
  ServiceInfo service_info;

  //获取服务对象的描述信息
  const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
  //获取服务的名字
  std::string service_name = pserviceDesc->name();
  //获取服务对象service的方法的数量
  int methodCnt = pserviceDesc->method_count();
  
  LOG_INFO("service_name:%s",service_name.c_str());
  for(int i = 0; i< methodCnt;++i)
  {
    // 获取服务对象指定下标的服务方法的描述
    const google::protobuf::MethodDescriptor* pmethodDesc = pserviceDesc->method(i);
    std::string method_name = pmethodDesc->name();
    service_info.m_methodMap.insert({method_name,pmethodDesc});
    LOG_INFO("method_name:%s",method_name.c_str());
  }
  service_info.m_service = service;
  m_serviceMap.insert({service_name,service_info});
  
}

void RpcProvider::Run()
{
  //读取配置文件的信息
  std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
  uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
  muduo::net::InetAddress address(ip,port);


  //创建TcpServer对象
  muduo::net::TcpServer server(&m_eventLoop,address,"RpcProvider");
  //绑定连接回调和消息读写回调方法,分离了网络代码和业务代码
  server.setConnectionCallback(std::bind(&RpcProvider::OnConnection,this,std::placeholders::_1));

  server.setMessageCallback(std::bind(&RpcProvider::OnMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
  //设置muduo库的线程数量
  server.setThreadNum(4);
  
  //将当前rpc结点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
  ZkClient zkCil;
  zkCil.Start();
  //service_name为永久性结点，method_name为临时性结点
  //30s  zkclient 网络IO线程专门以 1/3 * timeout时间发送ping消息
  for(auto &sp : m_serviceMap)
  {
    //service_name
    std::string service_path = "/" + sp.first;
    zkCil.Create(service_path.c_str(),nullptr,0);
    for(auto &mp:sp.second.m_methodMap)
    {
      //service_name => method_name 
      // /UserServiceRpc/Login 存储当前这个rpc服务结点主机的ip和和port
      std::string method_path = service_path + "/" + mp.first;
      char method_path_data[128] = {0};
      sprintf(method_path_data,"%s:%d",ip.c_str(),port);
      // ZOO_EPHEMERAL 表示znode是一个临时性结点
      zkCil.Create(method_path.c_str(),method_path_data,strlen(method_path_data),ZOO_EPHEMERAL);
    }

  }

  std::cout<<"RpcProvider start service at ip"<<ip<<"port"<<port<<std::endl;


  server.start();
  m_eventLoop.loop();
}
//提供setConnectionCallBack的参数
//新的socket的连接回调
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr&conn)
{
  if(!conn->connected())
  {
    //和rpc client的连接断开了
    conn->shutdown();
  }
}

/*
 * 在框架内部 , RpcProvider 和RpcConsumer协商好之间通信用的protobuf数据类型
 * service_name method_name args  定义proto的message类型 进行数据的序列化和反序列化
 *                                service_name method_name args_size
 *header_size(4个字节) 通过这个找到service_name 与menthod_name
 * 我们不知道读前面多少个字节，需要将header_size按二进制方式存入string中
 * 如果一个整数1000，按字符串方式四字节未必存的下，但是按照二进制的方式就可以
 */
//已建立连接用户的读写事件，如果远程有一个rpc服务的调用请求,那么OnMessage方法就会响应
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn,
                            muduo::net::Buffer* buffer,
                            muduo::Timestamp)
{
  //网络上接受远程rpc调用请求的字符流 Login args
  std::string recv_buf = buffer->retrieveAllAsString();

  //从字符流中读取前4个字符
  uint32_t header_size = 0;
  recv_buf.copy((char*)&header_size,4,0);
  
  //根据header_size读取数据头的原始字符流,反序列化数据得到rpc请求的详细信息
  std::string rpc_header_str = recv_buf.substr(4,header_size);
  mprpc::RpcHeader rpcHeader;

  std::string service_name;
  std::string method_name;
  uint32_t args_size;
  if(rpcHeader.ParseFromString(rpc_header_str))
  {
    //数据头反序列化成功
   service_name = rpcHeader.service_name();
   method_name = rpcHeader.method_name();
   args_size = rpcHeader.args_size();
  }
  else{
    //数据头反序列化失败
    std::cout<<"rpc_header_str"<<rpc_header_str<<"prase error"<<std::endl;
    return ;
  }
  //获取rpc方法参数的字符流数据
  std::string args_str = recv_buf.substr(4 + header_size,args_size);

  //打印调试信息
  std::cout<<"------------------------"<<std::endl;
  std::cout<<"header_size"<<header_size<<std::endl;
  std::cout<<"rpc_header_str"<<rpc_header_str<<std::endl;
  std::cout<<"service_name"<<service_name<<std::endl;
  std::cout<<"args_str"<<args_str<<std::endl;
  std::cout<<"------------------------"<<std::endl;

  //获取service对象和method对象
  auto it = m_serviceMap.find(service_name);
  if(it == m_serviceMap.end())
  {
    std::cout<<service_name<<"is not exist"<<std::endl;
    return ;
  }

  auto mit = it->second.m_methodMap.find(method_name);
  if(mit == it->second.m_methodMap.end())
  {
    std::cout<<service_name<<":"<<method_name<<"is not exist"<<std::endl;
    return ;
  }
  google::protobuf::Service *service = it->second.m_service;//获取service对象 new UserService
  const google::protobuf::MethodDescriptor *method = mit->second;// 获取method对象 Login

  //生成rpc方法调用的请求request和响应response参数
   google::protobuf::Message *request = service->GetRequestPrototype(method).New();
   if(!request->ParseFromString(args_str))
   {
     std::cout<<"request parse error"<<args_str<<std::endl;
     return;
   }
   google::protobuf::Message *response = service->GetResponsePrototype(method).New();

   //给下面method方法调用，绑定一个Closure的回调函数
   google::protobuf::Closure* done =
     google::protobuf::NewCallback<RpcProvider,
                                   const muduo::net::TcpConnectionPtr&, 
                                   google::protobuf::Message*>
                                    (this,&RpcProvider::SendRpcResponse,conn,response);
  //在框架上根据远端rpc请求，调用rpc结点发布的方法
  service->CallMethod(method,nullptr,request,response,done);
}

void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn,
                                  google::protobuf::Message* response)
{
  std::string response_str;
  if(response->SerializeToString(&response_str))
  {
    //序列化成功后，通过网络将rpc方法结果发送给rpc的调用方
    conn->send(response_str);
  }
  else 
  {
    std::cout<<"Serialize response_str error"<<std::endl;
  }
  conn->shutdown(); //模拟http短链接服务，由rpcprovider主动断开连接
}



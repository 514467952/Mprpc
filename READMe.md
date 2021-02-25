# 第三方库安装

在thirdFile文件夹中有项目所依赖的包，必须要安装才能使用此项目

## 安装protobuf

1、解压压缩包：unzip protobuf-master.zip
2、进入解压后的文件夹：cd protobuf-master
3、安装所需工具：sudo apt-get install autoconf automake libtool curl make g++ unzip
4、自动生成configure配置文件：./autogen.sh
5、配置环境：./configure
6、编译源代码(时间比较长)：make

## 安装zookeeper

1

``` bash
zookeeper-3.4.10$ cd conf    //进入文件夹
zookeeper-3.4.10/conf$  mv zoo_sample.cfg zoo.cfg //将配置文件名称进行更改
```

2、进入bin目录，启动zkServer， ./zkServer.sh start

在进行上述两布操作前需要安装jdk环境，所以必须先安装jdk，这里给出jdk的包，安装过程自行上网搜索

> 链接：https://pan.baidu.com/s/1KL0vjUt3fbNCnfojoy_N2Q 
> 提取码：gggg 
> 复制这段内容后打开百度网盘手机App，操作更方便哦

进入上面解压目录src/c下面，zookeeper已经提供了原生的C/C++和Java API开发接口，需要通过源码
编译生成，过程如下：

``` bash
~/package/zookeeper-3.4.10/src/c$ sudo ./configure
~/package/zookeeper-3.4.10/src/c$ sudo make
~/package/zookeeper-3.4.10/src/c$ sudo make install
```

## 安装muduo库

参照此链接https://blog.csdn.net/QIANGWEIYUAN/article/details/89023980

如果此链接失效，自行上网查找



# 如何使用

安装完上述三个第三方库后

需要有一个配置文件 test.conf在里面提供rpc服务的ip和port，还有服务配置中心zookeeper的ip和port

每个分布式的服务在不同机器上都有自己的ip和port，这里由于我只有一台服务器，所以只能在单机上验证

真实情况所有rpc服务的配置文件中zookeeper所在的服务器应该是一样的，因为所有服务都要往上面进行注册

```  bash
# rpc 结点的ip地址    
rpcserverip=127.0.0.1    
# rpc结点的port端口号    
rpcserverport=8001    
#zk 的ip地址    
zookeeperip=127.0.0.1    
#zk 的port端口号    
zookeeperport=2181      
```

这里利用muduo库将rpc服务启动，并用注册到ZK上面

``` c++
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
```

你可以直接在项目中example文件夹中进行编写，或者，将src文件夹拷贝到你所指定的文件夹中

服务端和调用者需要提前商议服务函数名称以及参数，并通过proto文件定义rpc服务



比方说定义一个查找好友的rpc服务，首先是定义proto文件

```protobuf
syntax = "proto3";
package fixbug;   //定义namespace
option cc_generic_services = true; //要生成rpc服务必须有这句话
message ResultCode  //定义返回值结果
{
  int32 errcode = 1;
  bytes errmsg = 2;
}
//定义请求函数，通过userid查找好友
message GetFriendsListRequest
{
  uint32 userid = 1;
}
//定义响应函数，返回结果和好友列表
message GetFriendsListResponse
{
  ResultCode result = 1;
  repeated bytes friends = 2;
}
//好友模块
//定义rpc服务
service FriendServiceRpc
{
  rpc GetFriendsList(GetFriendsListRequest) returns(GetFriendsListResponse);
}

```

当你定义proto文件后，用protoc进行生成后，h文件中就会提供所需要用的类，类里面的方法virtual 方法你需要进行重写，

``` c++
class FriendServiceRpc : public ::PROTOBUF_NAMESPACE_ID::Service {
 protected:
  // This class should be treated as an abstract interface.
  inline FriendServiceRpc() {};
 public:
  virtual ~FriendServiceRpc();

  typedef FriendServiceRpc_Stub Stub;

  static const ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor* descriptor();

  virtual void GetFriendsList(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::fixbug::GetFriendsListRequest* request,
                       ::fixbug::GetFriendsListResponse* response,
                       ::google::protobuf::Closure* done);

  // implements Service ----------------------------------------------

  const ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor* GetDescriptor();
  void CallMethod(const ::PROTOBUF_NAMESPACE_ID::MethodDescriptor* method,
                  ::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                  const ::PROTOBUF_NAMESPACE_ID::Message* request,
                  ::PROTOBUF_NAMESPACE_ID::Message* response,
                  ::google::protobuf::Closure* done);
  const ::PROTOBUF_NAMESPACE_ID::Message& GetRequestPrototype(
    const ::PROTOBUF_NAMESPACE_ID::MethodDescriptor* method) const;
  const ::PROTOBUF_NAMESPACE_ID::Message& GetResponsePrototype(
    const ::PROTOBUF_NAMESPACE_ID::MethodDescriptor* method) const;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FriendServiceRpc);
};
```

## 服务者示例

首先你需要创建个类继承自利用protobuf生成的h文件，然后定义本地对应函数的实现

最后重写基类中的虚函数

在虚函数中主要做几件事情

1. 或取请求参数
2. 调用本地对应的方法
3. 通过请求参数获取本地对应的信息
4. 封装响应消息，通过回到用返回给rpcClient端

``` c++
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
    //Closure是一个抽象类，
    done->Run();
  }
};
```

然后通过框架启动一个rpc服务

``` c++
  MprpcApplication::Init(argc,argv);
  RpcProvider provider;
  //将你需要的服务可以注册到框架上
  provider.NotifyService(new FriendService());
  provider.Run();
```

done

``` c++
 //给下面method方法调用，绑定一个Closure的回调函数
 //protobuf提供相应的Closure接口绑定回调
 //Closure提供了很多版本的NewCallback，done->run方法最后就是调用这个
   google::protobuf::Closure* done =
     google::protobuf::NewCallback<RpcProvider,
                                   const muduo::net::TcpConnectionPtr&, 
                                   google::protobuf::Message*>
                                     //conn和response都是SendRpcResponse的参数对应模板中的第二和三
                                    (this,&RpcProvider::SendRpcResponse,conn,response);
```



## 调用者示例

proto文件会自动生成服务端需要的类，也生成了客户端需要的类

```c++
class FriendServiceRpc_Stub : public FriendServiceRpc {
 public:
  FriendServiceRpc_Stub(::PROTOBUF_NAMESPACE_ID::RpcChannel* channel);
  FriendServiceRpc_Stub(::PROTOBUF_NAMESPACE_ID::RpcChannel* channel,
                   ::PROTOBUF_NAMESPACE_ID::Service::ChannelOwnership ownership);
  ~FriendServiceRpc_Stub();

  inline ::PROTOBUF_NAMESPACE_ID::RpcChannel* channel() { return channel_; }

  // implements FriendServiceRpc ------------------------------------------

  void GetFriendsList(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::fixbug::GetFriendsListRequest* request,
                       ::fixbug::GetFriendsListResponse* response,
                       ::google::protobuf::Closure* done);
 private:
  ::PROTOBUF_NAMESPACE_ID::RpcChannel* channel_;
  bool owns_channel_;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FriendServiceRpc_Stub);
};
```

调用者需要先调用框架的init方法接受参数，看需要调用哪个rpc服务

然后创建对应Rpc方法提供的stub类的类对象传入框架提供的channel方法，

闯将请求和响应对象，将请求时需要的参数通过protobuf提供的set方法写入到对象中

然后调用stub类对象的相应的方法，响应会通过respons参数传回来

``` c++
int main(int argc,char**argv)
{
  MprpcApplication::Init(argc,argv);

  fixbug::FriendServiceRpc_Stub stub(new MprpcChannel());
  fixbug::GetFriendsListRequest  request;
  request.set_userid(1000);
  fixbug::GetFriendsListResponse response;

  MprpcController controller;
  stub.GetFriendsList(&controller,&request,&response,nullptr);

  //一次rpc调用完成，读调用的结果
  //不要直接访问response
  if(controller.Failed())
  {
    std::cout<<controller.ErrorText() <<std::endl;
  }
  else 
  {
    if(0 == response.result().errcode())
    {
      std::cout<<"rpc GetFriendsList response success"<<std::endl;
      int size = response.friends_size();
      for(int i = 0; i< size;++i)
      {
        std::cout<<"index: " << (i+1) <<"name" <<response.friends(i) <<std::endl;
      }
    }
    else 
    {
      std::cout<<"rpc GetFriendsList response error:"<<response.result().errmsg()<<std::endl;
    }
  }
  return 0;
}
```








#pragma once 

#include<semaphore.h>
#include<zookeeper/zookeeper.h>
#include<string>


//封装zk客户端类
class ZkClient 
{
public:
  ZkClient();
  ~ZkClient();
  //zkclient启动连接zkserver
  void Start();
  //在zkserver上根据指定的path创建znode结点，默认0是永久性结点
  void Create(const char* path ,const char*data,int datalen,int state= 0);
  //根据参数指定的znode结点路径，获取znode结点的值
  std::string GetData(const char *path);
private:
  //zk客户端句柄
  zhandle_t *m_zhandle;
};

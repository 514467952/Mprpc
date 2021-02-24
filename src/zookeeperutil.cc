#include"include/zookeeperutil.h"
#include"include/mprpcapplication.h"
#include"include/logger.h"
#include<semaphore.h>
#include<iostream>

//全局的global_watcher
//zkserver给zkclient的通知，是异步的
void global_watcher(zhandle_t *zh,int type,
                    int state,const char*path,void *wathcherCtx)
{
  if(type == ZOO_SESSION_EVENT) //回调的消息类型和会话相关的消息类型
  {
    if(state == ZOO_CONNECTED_STATE)//zkclient和server连接成功
    {
      sem_t *sem = (sem_t*)zoo_get_context(zh);
      sem_post(sem);
    }
  }
}

ZkClient::ZkClient() : m_zhandle(nullptr)
{

}

ZkClient::~ZkClient()
{
  if(m_zhandle != nullptr)
  {
    zookeeper_close(m_zhandle);
  }
}


void ZkClient::Start()
{
  std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
  std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");

  std::string connstr = host + ":" + port;
  /*
   * zookeeper_mt 多线程版本
   * zookeeper的API客户端程序提供了三个线程
   * API调用线程
   * 网络I/O线程 pthread_create poll
   * watcher回调线程
   */ 
  //创建句柄成功，网络发送到zkserver上还没有响应
  //默认超时时间是30s
  m_zhandle = zookeeper_init(connstr.c_str(),global_watcher,30000,nullptr,nullptr,0);
  if(nullptr == m_zhandle)
  {
    std::cout<<"zookeeper_init error!"<<std::endl;
    LOG_INFO("zookeeper_init error");
    exit(EXIT_FAILURE);
  }

  sem_t sem;
  sem_init(&sem,0,0);
  zoo_set_context(m_zhandle,&sem);
  //等待watcher的回调通知
  sem_wait(&sem);
  std::cout<<"zookeeper_init success!"<<std::endl;
  LOG_INFO("zookeeper_init success!");

}


void ZkClient::Create(const char*path,const char* data,int datalen,int state)
{
  char path_buffer[128];
  int bufferlen = sizeof(path_buffer);
  int flag;

  flag = zoo_exists(m_zhandle,path,0,nullptr);
  //不要重复创建结点
  if(ZNONODE == flag)
  { 
    flag = zoo_create(m_zhandle,path,data,datalen,&ZOO_OPEN_ACL_UNSAFE,state,path_buffer,bufferlen);
    if(flag == ZOK)
    {
      std::cout<<"znode Create success ..path"<<path<<std::endl;
      LOG_INFO("znode create success ..%s",path);
    }
    else 
    {
      std::cout<<"flag:"<<flag<<std::endl;
      std::cout<<"znode Create error..path"<<path<<std::endl;
      LOG_ERR("znode create error .. %s",path);
      exit(EXIT_FAILURE);
    }
  }
}

std::string ZkClient::GetData(const char*path)
{
  char buffer[64];
  int bufferlen = sizeof(buffer);
  int flag = zoo_get(m_zhandle,path,0,buffer,&bufferlen,nullptr);
  if(flag != ZOK)
  {
    std::cout<<"get znode error ...path:"<<path<<std::endl;
    LOG_ERR("get znode error ..%s",path);
    return "";
  }
  else 
  {
    return buffer;
  }
}



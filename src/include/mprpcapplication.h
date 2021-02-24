#pragma  once 
#include"mprpcconfig.h"
#include"mprpcchannel.h"
#include"mprpccontroller.h"
//mprpc框架的初始化类
//这个类包含整个框架的共享信息：配置信息，日志信息。
class MprpcApplication
{
public:
  static void Init(int argc,char**argv);
  static MprpcApplication& GetInstance();
  static MprpcConfig& GetConfig();

private:
  static MprpcConfig m_config;
  MprpcApplication() {} ;
  MprpcApplication(const MprpcApplication&) = delete ;
  MprpcApplication(MprpcApplication&&) = delete ;
};

#ifndef AICONFIGMANAGER_H
#define AICONFIGMANAGER_H

#include <QString>
#include <QSettings>
#include <QCoreApplication>
#include <QFile>
#include <QDir>

class AIConfigManager
{
public:
    enum ServiceType
    {
        Aliyun,
        OpenAI
    };

    static AIConfigManager &instance()
    {
        static AIConfigManager instance;
        return instance;
    }

    // 获取服务类型
    ServiceType getServiceType();
    // 设置服务类型
    void setServiceType(ServiceType type);

    // 获取API密钥（根据服务类型）
    QString getApiKey();
    // 设置API密钥（根据服务类型）
    void setApiKey(const QString &key);

    // 获取模型名称（根据服务类型）
    QString getModelName();
    // 获取OpenAI端点
    QString getEndpoint();

private:
    AIConfigManager();
    QString m_configPath;

    // 将ServiceType转换为字符串
    QString serviceTypeToString(ServiceType type) const;
    // 将字符串转换为ServiceType
    ServiceType stringToServiceType(const QString &typeStr) const;
};

#endif // AICONFIGMANAGER_H

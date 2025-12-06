#include "aiconfigmanager.h"

AIConfigManager::AIConfigManager()
{
    // 尝试多个可能的配置文件路径
    QStringList configPaths;

    // 1. 项目根目录下的config.ini
    QString projectRootPath = QCoreApplication::applicationDirPath() + "/../../../../config.ini";
    configPaths << projectRootPath;

    // 2. 当前工作目录下的config.ini
    QString currentDirPath = QDir::currentPath() + "/config.ini";
    configPaths << currentDirPath;

    // 3. 可执行文件同级目录下的config.ini
    QString exeDirPath = QCoreApplication::applicationDirPath() + "/config.ini";
    configPaths << exeDirPath;

    // 选择第一个存在的配置文件
    m_configPath = exeDirPath;
    for (const QString &path : configPaths)
    {
        if (QFile::exists(path))
        {
            m_configPath = path;
            break;
        }
    }

    // 如果文件不存在，创建一个默认的
    if (!QFile::exists(m_configPath))
    {
        QSettings settings(m_configPath, QSettings::IniFormat);

        // 通用配置
        settings.setValue("General/ServiceType", "Aliyun");

        // 阿里云配置
        settings.setValue("Aliyun/ApiKey", "sk-c30504bf26dd4dfebb3342e7f2a9af4d");
        settings.setValue("Aliyun/Model", "qwen-vl-max");

        // OpenAI配置
        settings.setValue("OpenAI/ApiKey", "");
        settings.setValue("OpenAI/Model", "gpt-4-vision-preview");
        settings.setValue("OpenAI/Endpoint", "https://api.openai.com/v1/chat/completions");

        settings.sync();
    }
    else
    {
        // 兼容旧版本配置文件
        QSettings settings(m_configPath, QSettings::IniFormat);
        if (settings.contains("QianWen/ApiKey"))
        {
            // 将旧版本配置迁移到新版本格式
            settings.setValue("Aliyun/ApiKey", settings.value("QianWen/ApiKey"));
            settings.setValue("Aliyun/Model", settings.value("QianWen/Model", "qwen-vl-max"));
            settings.remove("QianWen");
            settings.sync();
        }
    }
}

QString AIConfigManager::serviceTypeToString(ServiceType type) const
{
    switch (type)
    {
    case Aliyun:
        return "Aliyun";
    case OpenAI:
        return "OpenAI";
    default:
        return "Aliyun";
    }
}

AIConfigManager::ServiceType AIConfigManager::stringToServiceType(const QString &typeStr) const
{
    if (typeStr.compare("OpenAI", Qt::CaseInsensitive) == 0)
    {
        return OpenAI;
    }
    return Aliyun;
}

AIConfigManager::ServiceType AIConfigManager::getServiceType()
{
    QSettings settings(m_configPath, QSettings::IniFormat);
    QString typeStr = settings.value("General/ServiceType", "Aliyun").toString();
    
    if (typeStr.isEmpty())
    {
        typeStr = "Aliyun";
    }
    
    return stringToServiceType(typeStr);
}

void AIConfigManager::setServiceType(ServiceType type)
{
    QSettings settings(m_configPath, QSettings::IniFormat);
    settings.setValue("General/ServiceType", serviceTypeToString(type));
    settings.sync();
}

QString AIConfigManager::getApiKey()
{
    QSettings settings(m_configPath, QSettings::IniFormat);
    ServiceType type = getServiceType();

    QString apiKey;
    if (type == OpenAI)
    {
        apiKey = settings.value("OpenAI/ApiKey", "").toString();
    }
    else
    {
        apiKey = settings.value("Aliyun/ApiKey", "").toString();
    }

    return apiKey;
}

void AIConfigManager::setApiKey(const QString &key)
{
    QSettings settings(m_configPath, QSettings::IniFormat);
    ServiceType type = getServiceType();

    if (type == OpenAI)
    {
        settings.setValue("OpenAI/ApiKey", key);
    }
    else
    {
        settings.setValue("Aliyun/ApiKey", key);
    }
    settings.sync();
}

QString AIConfigManager::getModelName()
{
    QSettings settings(m_configPath, QSettings::IniFormat);
    ServiceType type = getServiceType();

    if (type == OpenAI)
    {
        return settings.value("OpenAI/Model", "gpt-4-vision-preview").toString();
    }
    else
    {
        return settings.value("Aliyun/Model", "qwen-vl-max").toString();
    }
}

QString AIConfigManager::getEndpoint()
{
    QSettings settings(m_configPath, QSettings::IniFormat);
    return settings.value("OpenAI/Endpoint", "https://api.openai.com/v1/chat/completions").toString();
}
#include "aimanager.h"
#include "aiconfigmanager.h"

AiManager::AiManager(QObject *parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
}

void AiManager::generateImageDescription(const QImage &image)
{
    AIConfigManager::ServiceType serviceType = AIConfigManager::instance().getServiceType();
    QString apiKey = AIConfigManager::instance().getApiKey();
    
    // 验证API密钥
    bool isInvalid = false;
    if (serviceType == AIConfigManager::OpenAI) {
        if (apiKey.isEmpty()) {
            isInvalid = true;
        }
    } else {
        if (apiKey.isEmpty() || apiKey == "sk-c30504bf26dd4dfebb3342e7f2a9af4d") {
            isInvalid = true;
        }
    }
    
    if (isInvalid)
    {
        QString serviceName = (serviceType == AIConfigManager::OpenAI ? "OpenAI" : "阿里云");
        emit errorOccurred(QString("请在 config.ini 中配置正确的 %1 API Key").arg(serviceName));
        return;
    }

    QString base64Image = imageToBase64(image);

    if (serviceType == AIConfigManager::OpenAI)
    {
        // OpenAI API 调用
        QString endpoint = AIConfigManager::instance().getEndpoint();
        QUrl url(endpoint);
        QNetworkRequest request(url);

        // 设置 Header
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());

        // 构造 JSON Body (适配 OpenAI GPT-4V 格式)
        QJsonObject textContent;
        textContent["type"] = "text";
        textContent["text"] = "请详细描述这张图片中的内容，如果是文字请提取出来。";

        QJsonObject imageUrl;
        imageUrl["url"] = "data:image/png;base64," + base64Image;

        QJsonObject imageContent;
        imageContent["type"] = "image_url";
        imageContent["image_url"] = imageUrl;

        QJsonArray contentArray;
        contentArray.append(textContent);
        contentArray.append(imageContent);

        QJsonObject message;
        message["role"] = "user";
        message["content"] = contentArray;

        QJsonArray input;
        input.append(message);

        QJsonObject jsonBody;
        jsonBody["model"] = AIConfigManager::instance().getModelName();
        jsonBody["input"] = input;
        jsonBody["max_tokens"] = 300;

        // 发送 POST 请求
        QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(jsonBody).toJson());
        connect(reply, &QNetworkReply::finished, this, [this, reply]()
                { onNetworkReply(reply); });
    }
    else
    {
        // 阿里云 API 调用（原有逻辑）
        QUrl url("https://dashscope.aliyuncs.com/api/v1/services/aigc/multimodal-generation/generation");
        QNetworkRequest request(url);

        // 设置 Header
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());

        // 处理图片为 Base64
        QString base64ImageWithPrefix = "data:image/png;base64," + base64Image;

        // 构造 JSON Body (适配 Qwen-VL 格式)
        QJsonObject textContent;
        textContent["text"] = "请详细描述这张图片中的内容，如果是文字请提取出来。";

        QJsonObject imageContent;
        imageContent["image"] = base64ImageWithPrefix;

        QJsonArray contentArray;
        contentArray.append(imageContent);
        contentArray.append(textContent);

        QJsonObject message;
        message["role"] = "user";
        message["content"] = contentArray;

        QJsonArray messages;
        messages.append(message);

        QJsonObject input;
        input["messages"] = messages;

        QJsonObject jsonBody;
        jsonBody["model"] = AIConfigManager::instance().getModelName();
        jsonBody["input"] = input;

        // 发送 POST 请求
        QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(jsonBody).toJson());
        connect(reply, &QNetworkReply::finished, this, [this, reply]()
                { onNetworkReply(reply); });
    }
}

void AiManager::onNetworkReply(QNetworkReply *reply)
{
    reply->deleteLater(); // 稍后自动释放

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("网络错误: " + reply->errorString());
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    
    if (jsonDoc.isNull()) {
        emit errorOccurred("解析响应失败: 无效的JSON格式");
        return;
    }
    
    QJsonObject jsonObj = jsonDoc.object();

    AIConfigManager::ServiceType serviceType = AIConfigManager::instance().getServiceType();

    if (serviceType == AIConfigManager::OpenAI) {
        // 解析 OpenAI 响应
        if (jsonObj.contains("error")) {
            // 优先处理API错误
            QJsonObject errorObj = jsonObj["error"].toObject();
            QString errorMsg = errorObj["message"].toString();
            QString errorType = errorObj["type"].toString();
            emit errorOccurred(QString("OpenAI API 错误 (%1): %2").arg(errorType).arg(errorMsg));
            return;
        }
        
        if (jsonObj.contains("choices")) {
            QJsonArray choices = jsonObj["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject firstChoice = choices[0].toObject();
                if (firstChoice.contains("message")) {
                    QJsonObject msg = firstChoice["message"].toObject();
                    if (msg.contains("content")) {
                        QString resultText = msg["content"].toString();
                        emit descriptionGenerated(resultText);
                        return;
                    }
                }
            }
        }
        
        emit errorOccurred("解析 OpenAI 响应失败: 无效的响应格式");
    } else {
        // 解析阿里云响应（原有逻辑）
        // 检查是否有 API 错误
        if (jsonObj.contains("code") && jsonObj.contains("message")) {
            emit errorOccurred("API 错误: " + jsonObj["message"].toString());
            return;
        }

        if (jsonObj.contains("output")) {
            QJsonObject output = jsonObj["output"].toObject();
            if (output.contains("choices")) {
                QJsonArray choices = output["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject firstChoice = choices[0].toObject();
                    if (firstChoice.contains("message")) {
                        QJsonObject msg = firstChoice["message"].toObject();
                        if (msg.contains("content")) {
                            QJsonArray content = msg["content"].toArray();
                            // Qwen-VL 返回的 content 可能是数组，包含 text
                            QString resultText;
                            for (const auto &item : content) {
                                if (item.toObject().contains("text")) {
                                    resultText += item.toObject()["text"].toString();
                                }
                            }
                            emit descriptionGenerated(resultText);
                            return;
                        }
                    }
                }
            }
        }

        // 如果解析失败
        if (jsonObj.contains("message")) {
            emit errorOccurred("API 错误: " + jsonObj["message"].toString());
        } else {
            emit errorOccurred("解析响应失败");
        }
    }
}

QString AiManager::imageToBase64(const QImage &image)
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return QString::fromLatin1(byteArray.toBase64().data());
}
#include <unordered_map>

class HttpRequestParser {
public:
    HttpRequestParser() = default;


    std::string getMethod() const {
        return requestData.method;
    }
    std::string getPath() const {
        return requestData.path;
    }
    std::string getVersion() const {
        return requestData.version;
    }
    std::unordered_map<std::string, std::string> getHeaders() const {
        return requestData.headers;
    }
    std::string getBody() const {
        return requestData.body;
    }


    void parseRequest(const std::string& rawRequest) {
        std::istringstream requestStream(rawRequest);
        std::string line;

        // Parse request line
        if (std::getline(requestStream, line)) {
            std::istringstream lineStream(line);
            lineStream >> requestData.method >> requestData.path >> requestData.version;
        }

        // Parse headers
        while (std::getline(requestStream, line) && line != "\r") {
            size_t delimiterPos = line.find(": ");
            if (delimiterPos != std::string::npos) {
                std::string headerName = line.substr(0, delimiterPos);
                std::string headerValue = line.substr(delimiterPos + 2);
                // Remove trailing \r if present
                if (!headerValue.empty() && headerValue.back() == '\r') {
                    headerValue.pop_back();
                }
                requestData.headers[headerName] = headerValue;
            }
        }

        // Parse body
        std::ostringstream bodyStream;
        bodyStream << requestStream.rdbuf();
        requestData.body = bodyStream.str();
    }

private: 
    struct HttpRequest {
        std::string method;
        std::string path;
        std::string version;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
    };
    HttpRequest requestData;
};
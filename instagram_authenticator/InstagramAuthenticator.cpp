#include <memory>
#include <algorithm>
#include "InstagramAuthenticator.h"
#include "SSLQuery.h"
#include "AuthorizationUtils.h"
#include "Compression.h"

namespace
{
    bool GetCsrf(const Cookies_vt& cookies, std::string& csrf)
    {
        const auto& it = std::find_if(cookies.cbegin(), cookies.cend(), [](const Cookie& cookie) -> bool
        {
            return cookie.first == "csrftoken";
        });

        if (it != cookies.cend())
        {
            csrf = it->second;
            return true;
        }

        return false;
    }
}

InstagramAuthenticator::InstagramAuthenticator(const std::string& username, const std::string& password)
    : m_username(username)
    , m_password(password)
{
}

void InstagramAuthenticator::PerformLogin(std::string& csrfToken)
{
    std::unique_ptr<ISSLQuery> query = std::make_unique<SSLQuery>();
    ConfigureQuery(query.get());
    authorization::utils::SetBasicHeaders(query.get());
    authorization::utils::SetCSRFToken(query.get(), "1");
    query->SetPostField("username", m_username);
    query->SetPostField("password", m_password);
    query->SetPostField("queryParams", "{\"source\":\"auth_switcher\"}");

    std::string buffer;
    query->GetData("https://www.instagram.com/accounts/login/ajax/", buffer);
    CheckAuthenticantion(buffer);

    Cookies_vt cookies;
    query->GetCookies(cookies);
    GetCsrf(cookies, csrfToken);
}

void InstagramAuthenticator::ConfigureQuery(ISSLQuery* query)
{
    query->SetProxy("localhost", 8888);
    query->EnableSSLVerification(false);
    query->FollowLocation(true);
}

void InstagramAuthenticator::CheckAuthenticantion(const std::string& response)
{
    std::vector<char> jsonResponse;
    utils::compression::GZipDecompress(response.data(), response.size(), jsonResponse, 10000u);
    authorization::AuthResponse auth;
    authorization::utils::ParseAuthResponse(jsonResponse, auth);

    if (!auth.isAuthenticated)
    {
        throw std::runtime_error("Authentication error");
    }
}

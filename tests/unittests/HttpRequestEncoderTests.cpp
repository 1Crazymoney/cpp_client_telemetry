// Copyright (c) Microsoft. All rights reserved .

#include "common/Common.hpp"
#include "common/MockIHttpClient.hpp"
#include "http/HttpRequestEncoder.hpp"

using namespace testing;
using namespace ARIASDK_NS;


class HttpRequestEncoderTests : public Test {
  protected:
    MockIHttpClient    mockHttpClient;
    HttpRequestEncoder encoder;

  protected:
    HttpRequestEncoderTests()
      : encoder(mockHttpClient)
    {
        EXPECT_CALL(mockHttpClient, CreateRequest())
            .WillRepeatedly(Invoke(&HttpRequestEncoderTests::createHttpRequest));
    }

    static IHttpRequest* createHttpRequest()
    {
        return new SimpleHttpRequest("HttpRequestEncoderTests");
    }
};


TEST_F(HttpRequestEncoderTests, SetsAllParameters)
{
    EventsUploadContextPtr ctx = EventsUploadContext::create();
    ctx->compressed = false;
    ctx->body       = {1, 127, 255};
    ctx->packageIds["tenant1-token"] = 0;
    ctx->latency   = EventLatency_RealTime;

    encoder.encode(ctx);

    ASSERT_THAT(ctx->httpRequestId, Eq("HttpRequestEncoderTests"));
    SimpleHttpRequest const* req = static_cast<SimpleHttpRequest*>(ctx->httpRequest);

    EXPECT_THAT(req->m_id, Eq("HttpRequestEncoderTests"));
    EXPECT_THAT(req->m_method, Eq("POST"));
    EXPECT_THAT(req->m_url, Eq("http://collector/"));
    EXPECT_THAT(req->m_headers, Contains(Pair("Expect",       "100-continue")));
    EXPECT_THAT(req->m_headers, Contains(Pair("Client-Version",  PAL::getSdkVersion())));
    EXPECT_THAT(req->m_headers, Contains(Pair("Client-Id",    "NO_AUTH")));
    EXPECT_THAT(req->m_headers, Contains(Pair("Content-Type", "application/bond-compact-binary")));
    EXPECT_THAT(req->m_headers, Contains(Pair("APIKey",     "tenant1-token")));
    EXPECT_THAT(req->m_body,    Eq(std::vector<uint8_t>{1, 127, 255}));
    EXPECT_THAT(req->m_latency, Eq(EventLatency_RealTime));
}

TEST_F(HttpRequestEncoderTests, AddsCompressionHeader)
{
    EventsUploadContextPtr ctx = EventsUploadContext::create();

    ctx->compressed = false;
    encoder.encode(ctx);
    ASSERT_THAT(ctx->httpRequestId, Eq("HttpRequestEncoderTests"));
    SimpleHttpRequest const* req = static_cast<SimpleHttpRequest*>(ctx->httpRequest);
    EXPECT_THAT(req->m_headers, Not(Contains(Pair("Content-Encoding", "deflate"))));

    ctx->compressed = true;
    encoder.encode(ctx);
    ASSERT_THAT(ctx->httpRequestId, Eq("HttpRequestEncoderTests"));
    req = static_cast<SimpleHttpRequest*>(ctx->httpRequest);
    EXPECT_THAT(req->m_headers, Contains(Pair("Content-Encoding", "deflate")));
}

TEST_F(HttpRequestEncoderTests, BuildsApiKeyCorrectly)
{
    EventsUploadContextPtr ctx = EventsUploadContext::create();

    encoder.encode(ctx);
    ASSERT_THAT(ctx->httpRequestId, Eq("HttpRequestEncoderTests"));
    SimpleHttpRequest const* req = static_cast<SimpleHttpRequest*>(ctx->httpRequest);
    EXPECT_THAT(req->m_headers, Contains(Pair("APIKey", "")));

    ctx->packageIds["tenant1-token"] = 0;
    encoder.encode(ctx);
    ASSERT_THAT(ctx->httpRequestId, Eq("HttpRequestEncoderTests"));
    req = static_cast<SimpleHttpRequest*>(ctx->httpRequest);
    EXPECT_THAT(req->m_headers, Contains(Pair("APIKey", "tenant1-token")));

    ctx->packageIds["tenant2-token"] = 1;
    ctx->packageIds["tenant3-token"] = 2;
    encoder.encode(ctx);
    ASSERT_THAT(ctx->httpRequestId, Eq("HttpRequestEncoderTests"));
    req = static_cast<SimpleHttpRequest*>(ctx->httpRequest);
    EXPECT_THAT(req->m_headers, Contains(Pair("APIKey", "tenant1-token,tenant2-token,tenant3-token")));
}

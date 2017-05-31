// Copyright (c) Microsoft. All rights reserved.
#include "pal\PAL.hpp"

#include "HttpClient_WinRt.hpp"
#include "utils/Common.hpp"
#include <aria/Utils.hpp>
#include <algorithm>
#include <memory>
#include <sstream>
#include <vector>
#include "pplcancellation_token.h"
#include <ppltasks.h>
#include <pplawait.h>
#include "vccorlib.h"
#include <Roapi.h>
#include "WinInet.h"

using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage::Streams;
using namespace concurrency;
using namespace Platform;


namespace ARIASDK_NS_BEGIN {

	Platform::String ^to_platform_string(const std::string& s)
	{
		std::wstring wcontent(s.begin(), s.end());
		return ref new Platform::String(wcontent.data());
	}

	std::string from_platform_string(Platform::String ^ ps)
	{
		std::wstring wcontent(ps->Data());
		return std::string(wcontent.begin(), wcontent.end());
	}


		
	class WinRtRequestWrapper 
	{
	protected:
		HttpClient_WinRt*      m_parent;
		std::string            m_id;
		IHttpResponseCallback* m_appCallback;
		HttpRequestMessage^    m_httpRequestMessage;
		HttpResponseMessage^   m_httpResponseMessage;
		HANDLE                 m_hDoneEvent;
		concurrency::cancellation_token_source m_cancellationTokenSource;

	public:
		WinRtRequestWrapper(HttpClient_WinRt* parent, std::string const& id)
			: m_parent(parent),
			m_id(id),
			m_httpRequestMessage(nullptr),
			m_hDoneEvent(::CreateEvent(NULL, TRUE, FALSE, NULL))
		{
			ARIASDK_LOG_DETAIL("%p WinRtRequestWrapper()", this);
		}

		//WinRtRequestWrapper(WinRtRequestWrapper const&) = delete;
		//WinRtRequestWrapper& operator=(WinRtRequestWrapper const&) = delete;

		~WinRtRequestWrapper()
		{
			ARIASDK_LOG_DETAIL("%p ~WinRtRequestWrapper()", this);
			::CloseHandle(m_hDoneEvent);
		}

		void signalDone()
		{
			::SetEvent(m_hDoneEvent);
		}

		void cancel()
		{
			m_cancellationTokenSource.cancel();
			::WaitForSingleObject(m_hDoneEvent, INFINITE);
			if (m_httpRequestMessage) delete m_httpRequestMessage;
			m_httpRequestMessage = nullptr;
		}

		void send(SimpleHttpRequest* request, IHttpResponseCallback* callback)
		{
			m_appCallback = callback;

			{
				std::lock_guard<std::mutex> lock(m_parent->m_requestsMutex);
				m_parent->m_requests[m_id] = this;
			}

			// Convert std::string to Uri
			Uri ^ uri = ref new Uri(to_platform_string(request->m_url));
			// Create new request message
			m_httpRequestMessage = ref new HttpRequestMessage(HttpMethod::Post, uri);
								
			// Initialize the in-memory stream where data will be stored.
			DataWriter^ dataWriter = ref new DataWriter();
			dataWriter->WriteBytes((Platform::ArrayReference<unsigned char>(reinterpret_cast<unsigned char*>(request->m_body.data()), (DWORD)request->m_body.size())));
			IBuffer ^ibuffer = dataWriter->DetachBuffer();
			HttpBufferContent^ httpBufferContent = ref new HttpBufferContent(ibuffer);

			m_httpRequestMessage->Content = httpBufferContent;// ref new HttpBufferContent(ibuffer);
			
			// Populate headers based on user-supplied headers
			for (auto &kv : request->m_headers)
			{
				Platform::String^ key = to_platform_string(kv.first);
				Platform::String^ value = to_platform_string(kv.second);
				if (kv.first.compare("Client-Id") == 0 ||
					kv.first.compare("X-APIKey") == 0 ||
					kv.first.compare("SDK-Version") == 0)
				{
					m_httpRequestMessage->Headers->TryAppendWithoutValidation(key, value);
				}
				if (kv.first.compare("Expect") == 0)
				{
					m_httpRequestMessage->Headers->Expect->TryParseAdd(value);
				}
				if (kv.first.compare("Content-Encoding") == 0)
				{
					m_httpRequestMessage->Content->Headers->ContentEncoding->Append(ref new HttpContentCodingHeaderValue(value));
				}
				if (kv.first.compare("Content-Type") == 0)
				{
					m_httpRequestMessage->Content->Headers->ContentType = ref new HttpMediaTypeHeaderValue(value);
				}
			}

			SendHttpAsyncRequest(m_httpRequestMessage);

			//delete  dataWriter;
			//delete stream;

		}
		
		void SendHttpAsyncRequest(HttpRequestMessage ^req)
		{			
			IAsyncOperationWithProgress<HttpResponseMessage^, HttpProgress>^ operation = m_parent->getHttpClient()->SendRequestAsync(req, HttpCompletionOption::ResponseContentRead);
			m_cancellationTokenSource = cancellation_token_source();

			create_task(operation, m_cancellationTokenSource.get_token()).
				then([this](task<HttpResponseMessage^> responseTask)
			{
				try
				{
					// Check if any previous task threw an exception.
					m_httpResponseMessage = responseTask.get();
					//if (response->StatusCode != HttpStatusCode::None)
					{
						onRequestComplete(m_httpResponseMessage);
					}
				}
				catch (Platform::Exception ^ex) {
					printf("Connection failed!\n");
				}
				catch (...)
				{
					printf("Some other unknown exception!\n");
				}
			});
		}

		void onRequestComplete(HttpResponseMessage^ httpResponse)
		{
			std::unique_ptr<SimpleHttpResponse> response(new SimpleHttpResponse(m_id));
			if (httpResponse->IsSuccessStatusCode)
			{		
				response->m_result = HttpResult_OK;
				IMapView<String^, String^>^ mapView = httpResponse->Headers->GetView();

				//String^ temp = httpResponse->Headers->ToString();
				//std::string tempString = from_platform_string(temp);

				auto iterator = mapView->First();
				unsigned int  index = 0;
				while(index < mapView->Size)
				{
					String^ Key = iterator->Current->Key;
					String^ Value = iterator->Current->Value;

					response->m_headers.add(from_platform_string(Key), from_platform_string(Value));
					iterator->MoveNext();	
					index++;
				}
				HttpBufferContent^ content;
				auto operation = m_httpResponseMessage->Content->ReadAsBufferAsync();
				auto task = create_task(operation);
				if (task.wait() == task_status::completed)
				{
					auto buffer = task.get();
					size_t length = buffer->Length;

					if (length > 0)
					{
						response->m_body.reserve(length);
						unsigned char* storage = static_cast<unsigned char*>(response->m_body.data());
						DataReader::FromBuffer(buffer)->ReadBytes(ArrayReference<unsigned char>(storage, (DWORD)length));
						//response->m_body = std::string(reinterpret_cast<char*>(storage), length);
					}
				}
			}
			else
			{
				response->m_result = HttpResult_LocalFailure;
			}

			m_appCallback->OnHttpResponse(response.release());
			m_parent->signalDoneAndErase(m_id);
			
		}
	};

//---

unsigned HttpClient_WinRt::s_nextRequestId = 0;

HttpClient_WinRt::HttpClient_WinRt()
{
	m_httpClient = ref new HttpClient();
}

HttpClient_WinRt::~HttpClient_WinRt()
{
    bool done;

    {
		std::lock_guard<std::mutex> lock(m_requestsMutex);
        for (auto const& item : m_requests) 
		{
			WinRtRequestWrapper* temp= item.second;
			temp->cancel();
        }
        done = m_requests.empty();
    }

    while (!done) {
        PAL::sleep(100);
        {
			std::lock_guard<std::mutex> lock(m_requestsMutex);
            done = m_requests.empty();
        }
    }

    if(m_httpClient) delete m_httpClient;
}


void HttpClient_WinRt::signalDoneAndErase(std::string const& id)
{
	std::lock_guard<std::mutex> lock(m_requestsMutex);
    auto it = m_requests.find(id);
    if (it != m_requests.end())
	{
        it->second->signalDone();
		delete it->second;
        m_requests.erase(it);
    }
}

IHttpRequest* HttpClient_WinRt::CreateRequest()
{
	std::string id = "WI-" + toString(::InterlockedIncrement(&s_nextRequestId));
	return new SimpleHttpRequest(id);
}

void HttpClient_WinRt::SendRequestAsync(IHttpRequest* request, IHttpResponseCallback* callback)
{
    SimpleHttpRequest* req = static_cast<SimpleHttpRequest*>(request);
	WinRtRequestWrapper* wrapper = new WinRtRequestWrapper(this, req->m_id);
	wrapper->send(req, callback);
	delete req;
}

void HttpClient_WinRt::CancelRequestAsync(std::string const& id)
{
	std::lock_guard<std::mutex> lock(m_requestsMutex);
	auto it = m_requests.find(id);
	if (it != m_requests.end())
	{
		it->second->cancel();
	}
}


} ARIASDK_NS_END

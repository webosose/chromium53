#include "webos_url_request_context_getter.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/worker_pool.h"
#include "components/os_crypt/os_crypt.h"
#include "components/network_session_configurator/switches.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cookie_store_factory.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/cert_net/nss_ocsp.h"
#include "net/extras/sqlite/cookie_crypto_delegate.h"
#include "net/code_cache/code_cache_impl.h"
#include "net/code_cache/dummy_code_cache.h"
#include "net/cookies/cookie_store.h"
#include "net/dns/host_resolver.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_cache.h"
#include "net/http/http_network_layer.h"
#include "net/http/http_server_properties_impl.h"
#include "net/http/http_stream_factory.h"
#include "net/proxy/proxy_service.h"
#include "net/socket/next_proto.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/default_channel_id_store.h"
#include "net/ssl/ssl_config_service_defaults.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/file_protocol_handler.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_storage.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "webos/browser/net/webos_network_delegate.h"
#include "webos/browser/webos_browser_context.h"
#include "webos/browser/webos_browser_switches.h"
#include "webos/browser/webos_http_user_agent_settings.h"

using content::BrowserThread;

namespace webos {

namespace {

const char kCacheStoreFile[] = "Cache";
const char kCookieStoreFile[] = "Cookies";
const int kDefaultDiskCacheSize = 16 * 1024 * 1024;  // default size is 16MB

}  // namespace

class SecurityPolicy {
 public:
  SecurityPolicy() {
    // Hardcoded path for now
    std::string securityPolicySettings;
    if (!base::ReadFileToString(base::FilePath("/etc/wam/security_policy.conf"),
                                &securityPolicySettings))
      return;
    std::istringstream iss(securityPolicySettings);
    std::string str;
    while (std::getline(iss, str)) {
      if (str.find("Allowed") != std::string::npos)
        ParsePathsFromSettings(allowed_target_paths_, iss);

      if (str.find("Trusted") != std::string::npos)
        ParsePathsFromSettings(allowed_trusted_target_paths_, iss);
    }
  }

  virtual ~SecurityPolicy() {}

  void ParsePathsFromSettings(std::vector<std::string>& paths,
                              std::istringstream& stream) {
    std::string str;
    do {
      std::getline(stream, str);
      if (str.find("size=") != std::string::npos)
        break;
      int path_pos = str.find("=");
      if (path_pos != std::string::npos)
        paths.push_back(str.substr(path_pos + 1));
    } while (str != "");
  }

  WebOSNetworkDelegate* CreateNetworkDelegate(
      std::map<std::string, std::string> extra_websocket_headers) {
    WebOSNetworkDelegate* network_delegate = new WebOSNetworkDelegate(
        allowed_target_paths_, allowed_trusted_target_paths_,
        std::move(extra_websocket_headers));
    return network_delegate;
  }

 private:
  std::vector<std::string> allowed_target_paths_;
  std::vector<std::string> allowed_trusted_target_paths_;
};

base::LazyInstance<SecurityPolicy> g_security_policy =
    LAZY_INSTANCE_INITIALIZER;

class WebOSURLRequestContext : public net::URLRequestContext {
 public:
  WebOSURLRequestContext() : net::URLRequestContext(), storage_(this) {}
  net::URLRequestContextStorage* storage() { return &storage_; }

 private:
  net::URLRequestContextStorage storage_;

  DISALLOW_COPY_AND_ASSIGN(WebOSURLRequestContext);
};

// From src/chrome/browser/net/cookie_store_util.cc
// Use src/components/cookie_config/cookie_store_util.h once it is available
// Use the operating system's mechanisms to encrypt cookies beforethread writing
// them to persistent store.  Currently this only is done with desktop OS's
// because ChromeOS and Android already protect the entire profile contents.
//
// TODO(bcwhite): Enable on MACOSX -- requires all Cookie tests to call
// OSCrypt::UseMockKeychain or will hang waiting for user input.
class CookieOSCryptoDelegate : public net::CookieCryptoDelegate {
 public:
  bool EncryptString(const std::string& plaintext,
                     std::string* ciphertext) override;
  bool DecryptString(const std::string& ciphertext,
                     std::string* plaintext) override;
  bool ShouldEncrypt() override;
};

bool CookieOSCryptoDelegate::EncryptString(const std::string& plaintext,
                                           std::string* ciphertext) {
  return OSCrypt::EncryptString(plaintext, ciphertext);
}

bool CookieOSCryptoDelegate::DecryptString(const std::string& ciphertext,
                                           std::string* plaintext) {
  return OSCrypt::DecryptString(ciphertext, plaintext);
}

bool CookieOSCryptoDelegate::ShouldEncrypt() {
  return true;
}

// Using a LazyInstance is safe here because this class is stateless and
// requires 0 initialization.
base::LazyInstance<CookieOSCryptoDelegate> g_cookie_crypto_delegate =
    LAZY_INSTANCE_INITIALIZER;

class WebOSTransportSecurityStateRequireCTDelegate
    : public net::TransportSecurityState::RequireCTDelegate {
 public:
  ~WebOSTransportSecurityStateRequireCTDelegate() override {}
  CTRequirementLevel IsCTRequiredForHost(const std::string& hostname) override {
    return CTRequirementLevel::NOT_REQUIRED;
  }
};

WebOSRequestContextGetter::WebOSRequestContextGetter(
    WebOSBrowserContext* browser_context,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors,
    std::map<std::string, std::string> extra_websocket_headers,
    std::string proxy_rules)
    : browser_context_(browser_context),
      request_interceptors_(std::move(request_interceptors)),
      extra_websocket_headers_(std::move(extra_websocket_headers)),
      proxy_rules_(proxy_rules) {
  std::swap(protocol_handlers_, *protocol_handlers);
}

WebOSRequestContextGetter::~WebOSRequestContextGetter() {}

net::URLRequestContext* WebOSRequestContextGetter::GetURLRequestContext() {
  if (!request_context_) {
    request_context_.reset(
        CreateRequestContext(browser_context_, &protocol_handlers_,
                             std::move(request_interceptors_)));

    network_delegate_.reset(g_security_policy.Pointer()->CreateNetworkDelegate(
        std::move(extra_websocket_headers_)));
    request_context_->set_network_delegate(network_delegate_.get());

    protocol_handlers_.clear();
  }
  return request_context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner>
WebOSRequestContextGetter::GetNetworkTaskRunner() const {
  return BrowserThread::GetMessageLoopProxyForThread(BrowserThread::IO);
}

void WebOSRequestContextGetter::InitializeSystemContextDependencies() {
  host_resolver_ = net::HostResolver::CreateDefaultResolver(NULL);

  // TODO(lcwu): http://crbug.com/392352. For performance and security reasons,
  // a persistent (on-disk) HttpServerProperties and ChannelIDService might be
  // desirable in the future.
  channel_id_service_.reset(
      new net::ChannelIDService(new net::DefaultChannelIDStore(NULL),
                                base::WorkerPool::GetTaskRunner(true)));

  cert_verifier_ = net::CertVerifier::CreateDefault();
  cert_transparency_verifier_.reset(new net::MultiLogCTVerifier());
  policy_enforcer_.reset(new net::CTPolicyEnforcer);

  ssl_config_service_ = new net::SSLConfigServiceDefaults;

  transport_security_state_.reset(new net::TransportSecurityState());
  transport_security_state_->SetRequireCTDelegate(
      new WebOSTransportSecurityStateRequireCTDelegate());
  http_auth_handler_factory_ =
      net::HttpAuthHandlerFactory::CreateDefault(host_resolver_.get());

  http_server_properties_.reset(new net::HttpServerPropertiesImpl);

  proxy_service_ = proxy_rules_.empty()
                       ? net::ProxyService::CreateUsingSystemProxyResolver(
                             net::ProxyService::CreateSystemProxyConfigService(
                                 BrowserThread::GetMessageLoopProxyForThread(
                                     BrowserThread::IO),
                                 BrowserThread::GetMessageLoopProxyForThread(
                                     BrowserThread::FILE)),
                             0, NULL)
                       : net::ProxyService::CreateFixed(proxy_rules_);

  http_user_agent_settings_.reset(new WebOSHttpUserAgentSettings());
}

void WebOSRequestContextGetter::InitializeMainContextDependencies(
    net::HttpTransactionFactory* transaction_factory,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  main_transaction_factory_.reset(transaction_factory);
  std::unique_ptr<net::URLRequestJobFactoryImpl> job_factory(
      new net::URLRequestJobFactoryImpl());
  // Keep ProtocolHandlers added in sync with
  // CastContentBrowserClient::IsHandledURL().
  bool set_protocol = false;
  for (content::ProtocolHandlerMap::iterator it = protocol_handlers->begin();
       it != protocol_handlers->end(); ++it) {
    set_protocol = job_factory->SetProtocolHandler(
        it->first, base::WrapUnique(it->second.release()));
    DCHECK(set_protocol);
  }
  set_protocol = job_factory->SetProtocolHandler(
      url::kDataScheme, base::WrapUnique(new net::DataProtocolHandler));

  set_protocol = job_factory->SetProtocolHandler(
      url::kFileScheme,
      base::WrapUnique(new net::FileProtocolHandler(
          BrowserThread::GetBlockingPool()->GetTaskRunnerWithShutdownBehavior(
              base::SequencedWorkerPool::SKIP_ON_SHUTDOWN))));
  DCHECK(set_protocol);

  // Set up interceptors in the reverse order.
  std::unique_ptr<net::URLRequestJobFactory> top_job_factory =
      std::move(job_factory);
  for (content::URLRequestInterceptorScopedVector::reverse_iterator i =
           request_interceptors.rbegin();
       i != request_interceptors.rend(); ++i) {
    top_job_factory.reset(new net::URLRequestInterceptingJobFactory(
        std::move(top_job_factory), base::WrapUnique(*i)));
  }
  request_interceptors.weak_clear();

  main_job_factory_.reset(top_job_factory.release());
}

void WebOSRequestContextGetter::PopulateNetworkSessionParams(
    bool ignore_certificate_errors,
    net::HttpNetworkSession::Params* params) {
  params->host_resolver = host_resolver_.get();
  params->cert_verifier = cert_verifier_.get();
  params->cert_transparency_verifier = cert_transparency_verifier_.get();
  params->ct_policy_enforcer = policy_enforcer_.get();
  params->channel_id_service = channel_id_service_.get();
  params->ssl_config_service = ssl_config_service_.get();
  params->transport_security_state = transport_security_state_.get();
  params->http_auth_handler_factory = http_auth_handler_factory_.get();
  params->http_server_properties = http_server_properties_.get();
  params->ignore_certificate_errors = ignore_certificate_errors;
  params->proxy_service = proxy_service_.get();
}

net::URLRequestContext* WebOSRequestContextGetter::CreateRequestContext(
    WebOSBrowserContext* browser_context,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  InitializeSystemContextDependencies();

  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  WebOSURLRequestContext* main_context = new WebOSURLRequestContext();

  int disk_cache_size = kDefaultDiskCacheSize;
  if (cmd_line->HasSwitch(kDiskCacheSize))
    base::StringToInt(cmd_line->GetSwitchValueASCII(kDiskCacheSize),
                      &disk_cache_size);

  std::unique_ptr<net::HttpCache::DefaultBackend> main_backend(
      new net::HttpCache::DefaultBackend(
          net::DISK_CACHE, net::CACHE_BACKEND_SIMPLE,
          browser_context->GetPath().Append(kCacheStoreFile), disk_cache_size,
          BrowserThread::GetMessageLoopProxyForThread(BrowserThread::CACHE)));

  bool ignore_certificate_errors = false;
  if (cmd_line->HasSwitch(switches::kIgnoreCertificateErrors)) {
    ignore_certificate_errors = true;
  }
  net::HttpNetworkSession::Params network_session_params;
  PopulateNetworkSessionParams(ignore_certificate_errors,
                               &network_session_params);
  main_context->storage()->set_http_network_session(
      base::WrapUnique(new net::HttpNetworkSession(network_session_params)));
  InitializeMainContextDependencies(
      new net::HttpCache(main_context->storage()->http_network_session(),
                         std::move(main_backend), true),
      protocol_handlers, std::move(request_interceptors));

  content::CookieStoreConfig cookie_config(
      browser_context->GetPath().Append(kCookieStoreFile),
      content::CookieStoreConfig::PERSISTANT_SESSION_COOKIES, NULL, NULL);
  cookie_config.background_task_runner =
      scoped_refptr<base::SequencedTaskRunner>();
  cookie_config.crypto_delegate = g_cookie_crypto_delegate.Pointer();
  main_cookie_store_ = content::CreateCookieStore(cookie_config);
  main_context->set_cookie_store(main_cookie_store_.get());

  main_context->set_channel_id_service(channel_id_service_.get());
  main_context->set_http_user_agent_settings(http_user_agent_settings_.get());
  main_context->set_host_resolver(host_resolver_.get());
  main_context->set_cert_verifier(cert_verifier_.get());
  main_context->set_transport_security_state(transport_security_state_.get());
  main_context->set_cert_transparency_verifier(
      cert_transparency_verifier_.get());
  main_context->set_ct_policy_enforcer(policy_enforcer_.get());
  main_context->set_proxy_service(proxy_service_.get());
  main_context->set_ssl_config_service(ssl_config_service_.get());
  main_context->set_http_auth_handler_factory(http_auth_handler_factory_.get());
  main_context->set_http_server_properties(http_server_properties_.get());

  main_context->set_http_transaction_factory(main_transaction_factory_.get());
  main_context->set_job_factory(main_job_factory_.get());

  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kEnableLocalResourceCodeCache)) {
    int max_size = 5242880;
    if (command_line->HasSwitch(switches::kLocalResourceCodeCacheSize))
      base::StringToInt(command_line->GetSwitchValueASCII(
                            switches::kLocalResourceCodeCacheSize),
                        &max_size);
    code_cache_.reset(new net::CodeCacheImpl(
        browser_context->GetPath(), max_size,
        BrowserThread::GetMessageLoopProxyForThread(BrowserThread::CACHE)));
  } else {
    code_cache_.reset(new net::DummyCodeCache(
        browser_context->GetPath(), 0,
        BrowserThread::GetMessageLoopProxyForThread(BrowserThread::CACHE)));
    // Delete code cache directory in storage if it exists
    code_cache_->ClearData();
  }
    main_context->set_code_cache(code_cache_.get());

  return main_context;
}

void WebOSRequestContextGetter::AppendExtraWebSocketHeader(
    const std::string& key,
    const std::string& value) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (network_delegate_)
    network_delegate_->append_extra_socket_header(key, value);
}

}  // namespace webos
